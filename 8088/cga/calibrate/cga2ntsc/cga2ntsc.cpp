#include "alfe/main.h"
#include "alfe/bitmap_png.h"
#include "alfe/complex.h"
#include "alfe/space.h"
#include "alfe/set.h"
#include "alfe/config_file.h"
#include "alfe/cga.h"
#include "alfe/timer.h"
#include "alfe/ntsc_decode.h"

template<class T> class CGA2NTSCWindowT;
typedef CGA2NTSCWindowT<void> CGA2NTSCWindow;

static const SRGB rgbiPalette[16] = {
    SRGB(0x00, 0x00, 0x00), SRGB(0x00, 0x00, 0xaa),
    SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
    SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
    SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
    SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
    SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
    SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
    SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff)};

class CGAShower
{
public:
    void setInput(Bitmap<SRGB> input)
    {
        _input = input;
        _size = input.size();
        _rgbi = Bitmap<Byte>(_size + Vector(14, 0));
    }
    void convert()
    {
        // Convert to RGBI indexes and add left and right borders.
        const Byte* inputRow = _input.data();
        Byte* rgbiRow = _rgbi.data();
        int background = _palette & 15;
        if ((_mode & 0x10) != 0)
            background = 0;

        for (int y = 0; y < _size.y; ++y) {
            const SRGB* inputPixel =
                reinterpret_cast<const SRGB*>(inputRow);
            Byte* rgbiPixel = rgbiRow;
            for (int x = 0; x < 7; ++x) {
                *rgbiPixel = background;
                ++rgbiPixel;
            }
            for (int x = 0; x < _size.x; ++x) {
                SRGB s = *inputPixel;
                ++inputPixel;
                int bestDistance = 0x7fffffff;
                Byte bestRGBI = 0;
                for (int i = 0; i < 16; ++i) {
                    int distance = (Vector3Cast<int>(rgbiPalette[i]) -
                        Vector3Cast<int>(s)).modulus2();
                    if (distance < bestDistance) {
                        bestDistance = distance;
                        bestRGBI = i;
                        if (distance < 42*42)
                            break;
                    }
                }
                *rgbiPixel = bestRGBI;
                ++rgbiPixel;
            }
            for (int x = 0; x < 7; ++x) {
                *rgbiPixel = background;
                ++rgbiPixel;
            }

            inputRow += _input.stride();
            rgbiRow += _rgbi.stride();
        }

        // Find all different composite colours (sequences of 8 consecutive
        // RGBI pixels).
        rgbiRow = _rgbi.data();
        for (int y = 0; y < _size.y; ++y) {
            const Byte* rgbiPixel = rgbiRow;
            UInt32 seq = 0;
            for (int xx = 0; xx < 7; ++xx) {
                seq = (seq >> 4) | ((*rgbiPixel) << 28);
                ++rgbiPixel;
            }
            for (int xx = 0; xx < _size.x + 7; ++xx) {
                seq = (seq >> 4) | ((*rgbiPixel) << 28);
                ++rgbiPixel;
            }
            rgbiRow += _rgbi.stride();
        }
    }
    void setMode(int mode) { _mode = mode; }
    void setPalette(int palette) { _palette = palette; }
    Bitmap<Byte> getOutput() { return _rgbi; }
private:
    Vector _size;
    Bitmap<SRGB> _input;
    Bitmap<Byte> _rgbi;
    int _mode;
    int _palette;
};

template<class T> class CGAMatcherT : public ThreadTask
{
public:
    CGAMatcherT() : _skip(256)
    {
        _patterns.allocate(0x10000*8*17 + 0x100*80*5);
    }
    void setInput(Bitmap<SRGB> input)
    {
        _input = input;
        _size = input.size();
        _input2 = Bitmap<SRGB>(_size + Vector(11, 0));
        _input2.fill(SRGB(0, 0, 0));
        _input2.subBitmap(Vector(5, 0), _size).copyFrom(_input);
        _configs.allocate(_size.y);
        _rgbi = Bitmap<Byte>(_size + Vector(14, 0));
    }
    Bitmap<Byte> getOutput() { return _rgbi; }
    void setProgram(Program* program) { _program = program; }
    static void filterHF(const Byte* input, SInt16* output, int n)
    {
        for (int x = 0; x < n; ++x)
            output[x] = (-input[x] + input[x+1]*2 + input[x+2]*6 + input[x+3]*2
                -input[x+4]);
    }
    void run()
    {
        _composite.initChroma();
        Byte burst[4];
        for (int i = 0; i < 4; ++i)
            burst[i] = _composite.simulateCGA(6, 6, i);
        _decoder.calculateBurst(burst);
        _block.y = _scanlinesPerRow * _scanlinesRepeat;
        if ((_mode & 2) != 0) {
            // In graphics modes, the data for the second scanline of the row
            // is independent of the data for the first scanline, so we can
            // pretend there's one scanline per row for matching purposes.
            if (_scanlinesPerRow == 2)
                _block.y = _scanlinesRepeat;
            for (int i = 0; i < 256; ++i)
                _skip[i] = false;
        }
        else {
            auto cgaROM = _sequencer.romData();
            int lines = _scanlinesPerRow;
            for (int i = 0; i < 256; ++i) {
                _skip[i] = false;
                if (_characterSet == 0) {
                    _skip[i] = (i != 0xdd);
                    continue;
                }
                if (_characterSet == 1) {
                    _skip[i] = (i != 0x13 && i != 0x55);
                    continue;
                }
                if (_characterSet == 2) {
                    _skip[i] =
                        (i != 0x13 && i != 0x55 && i != 0xb0 && i != 0xb1);
                    continue;
                }
                if (_characterSet == 4) {
                    _skip[i] = (i != 0xb1);
                    continue;
                }
                if (_characterSet == 5) {
                    _skip[i] = (i != 0xb0 && i != 0xb1);
                    continue;
                }
                if (_characterSet == 6) {
                    _skip[i] = (i != 0x06 && i != 0x13 && i != 0x19 &&
                        i != 0x22 && i != 0x27 && i != 0x55 && i != 0x57 &&
                        i != 0x60 && i != 0xb6 && i != 0xdd);
                }
                bool isBackground = true;
                bool isForeground = true;
                for (int y = 0; y < lines; ++y) {
                    Byte b = cgaROM[i*8 + y];
                    if (b != 0x00)
                        isBackground = false;
                    if (b != 0xff)
                        isForeground = false;
                }
                if (isBackground || isForeground) {
                    _skip[i] = true;
                    continue;
                }
                int j;
                for (j = 0; j < i; ++j) {
                    int y;
                    for (y = 0; y < lines; ++y)
                        if (cgaROM[i*8 + y] != cgaROM[j*8 + y])
                            break;
                    if (y == lines)
                        break;
                }
                if (j != i)
                    _skip[i] = true;
                for (j = 0; j < i; ++j) {
                    int y;
                    for (y = 0; y < lines; ++y)
                        if (cgaROM[i*8 + y] != (cgaROM[j*8 + y]^0xff))
                            break;
                    if (y == lines)
                        break;
                }
                if (j != i)
                    _skip[i] = true;
            }
        }

        _ntscInput = Bitmap<SInt16>(_size + Vector(1, 0));
        Byte* ntscRow = _ntscInput.data();
        const Byte* srgbRow = _input2.data();
        Array<Byte> ntscTemp(_size.x + 5);
        for (int y = 0; y < _size.y; ++y) {
            _decoder.encodeLine(&ntscTemp[0],
                reinterpret_cast<const SRGB*>(srgbRow), _size.x + 5, 2);
            filterHF(&ntscTemp[0], reinterpret_cast<SInt16*>(ntscRow),
                _size.x + 1);
            ntscRow += _ntscInput.stride();
            srgbRow += _input2.stride();
        }

        // Convert from _mode/_palette to config
        switch (_mode & 0x13) {
            case 0:
                _startConfig = 0x50;
                break;
            case 1:
                _startConfig = 0xd0;
                break;
            case 0x12:
                _startConfig = 0x40 + (_palette & 0x0f);
                break;
            case 2:
                _startConfig = _palette;
                break;
            case 0x10:
                _startConfig = 0x51;
                break;
            case 0x11:
                _startConfig = 0xd1;
                break;
            case 0x13:
                _startConfig = 0xc0 + (_palette & 0x0f);
                break;
            case 3:
                _startConfig = 0x80 + _palette;
                break;
        }
        _endConfig = _startConfig + 1;
        if (_palette == 0xff) {
            switch (_mode & 0x13) {
                case 0x12:
                    _startConfig = 0x40;
                    _endConfig = 0x50;
                    break;
                case 2:
                    _startConfig = 0x00;
                    _endConfig = 0x40;
                    break;
                case 0x13:
                    _startConfig = 0xc0;
                    _endConfig = 0xd0;
                    break;
                case 3:
                    _startConfig = 0x80;
                    _endConfig = 0xc0;
                    break;
            }
        }
        if ((_mode & 0x80) != 0) {
            _startConfig = (_mode & 1) == 0 ? 0 : 0x80;
            _endConfig = _startConfig + 0x51;
        }

        for (_config = _startConfig; _config < _endConfig; ++_config) {
            SInt16* p = &_patterns[(_config & 0x7f)*5*256];
            config();
            Array<Byte> rgbi(_block.x + 6);
            ntscTemp.allocate(_block.x + 5);
            int w = _block.x + 1;
            for (int pattern = 0; pattern < _patternCount; ++pattern) {
                for (int line = 0; line < _block.y; ++line) {
                    plotPattern(&rgbi[3], pattern, line);
                    rgbi[0] = rgbi[_block.x];
                    rgbi[1] = rgbi[1 + _block.x];
                    rgbi[2] = rgbi[2 + _block.x];
                    rgbi[3 + _block.x] = rgbi[3];
                    rgbi[4 + _block.x] = rgbi[4];
                    rgbi[5 + _block.x] = rgbi[5];
                    _composite.simulateLine(&rgbi[0], &ntscTemp[0],
                        _block.x + 5, 0);
                    filterHF(&ntscTemp[0], &p[(pattern*_block.y + line)*w], w);
                }
            }
        }

        int overscan = (_mode & 0x10) == 0 ? _palette & 0x0f : 0;
        _data.allocate((_size.y/_block.y)*(_size.x/_hdots));
        int y = 0;
        _rgbiRow = _rgbi.data() + 7;
        _inputRow = _ntscInput.data();
        _error = Bitmap<int>(_size + Vector(4, 1));
        _error.fill(0);
        _errorRow = _error.data();
        _testError = Bitmap<int>(_block + Vector(4, 1));
        //_window->resetColours();
        _config = _startConfig;
        _testConfig = (_startConfig + 1 != _endConfig);
        _configScore = 0x7fffffffffffffffUL;
        while (true) {
            int w = _block.x + 1;
            Vector errorLineSize(_size.x + 4, 1);
            Bitmap<int> savedError(errorLineSize);
            if (_testConfig)
                savedError.copyFrom(_error.subBitmap(Vector(0, y),
                    errorLineSize));
            config();
            SInt16* p = &_patterns[(_config & 0x7f)*5*256];
            UInt64 lineScore = 0;

            _rgbi.subBitmap(Vector(0, y), Vector(7, _block.y)).fill(overscan);
            _rgbi.subBitmap(Vector(7 + (_size.x & -_hdots), y),
                Vector(7 + _size.x - (_size.x & -_hdots), _block.y))
                .fill(overscan);
            for (int x = 0; x < (_size.x & -_hdots); x += _block.x) {
                int bestPattern = 0;
                int bestScore = 0x7fffffff;
                int skipSolidColour = 0xf00;
                for (int pattern = 0; pattern < _patternCount; ++pattern) {
                    if ((_mode & 2) == 0) {
                        if (_skip[pattern & 0xff])
                            continue;
                        if ((pattern & 0x0f00) == ((pattern >> 4) & 0x0f00)) {
                            if ((pattern & 0xf00) == skipSolidColour)
                                continue;
                            skipSolidColour = (pattern & 0xf00);
                        }
                    }
                    int score = 0;
                    const Byte* inputRow2 = _inputRow;
                    Byte* errorRow2 = _errorRow;
                    _testError.fill(0);
                    for (int yy = 0; yy < _block.y; ++yy) {
                        const SInt16* inputPixel =
                            reinterpret_cast<const SInt16*>(inputRow2) + x;
                        const int* errorPixel =
                            reinterpret_cast<const int*>(errorRow2) + x;
                        for (int xx = 0; xx < w; ++xx) {
                            int test = p[(pattern*_block.y + yy)*w + xx];
                            Vector p(xx, yy);
                            int target = inputPixel[xx] +
                                (errorPixel[xx] + _testError[p])/4;
                            int d = target - test;
                            int weight = (xx == 0 || xx == _block.x ? 1 : 2);
                            score += weight*d*d;
                            int error = weight*d;
                            _testError[p + Vector(4, 0)] +=
                                (error*_diffusionHorizontal)/256;
                            _testError[p + Vector(0, 1)] +=
                                (error*_diffusionVertical)/256;
                        }
                        inputRow2 += _ntscInput.stride();
                        errorRow2 += _error.stride();
                    }
                    if (score < bestScore) {
                        bestScore = score;
                        bestPattern = pattern;
                    }
                }
                for (int yy = 0; yy < _block.y; ++yy) {
                    Byte* rgbiPixel = _rgbiRow + yy*_rgbi.stride() + x;
                    plotPattern(rgbiPixel, bestPattern, yy);
                    UInt32 seq = 0;
                    rgbiPixel -= 7;
                    for (int xx = 0; xx < 7; ++xx) {
                        seq = (seq >> 4) | ((*rgbiPixel) << 28);
                        ++rgbiPixel;
                    }
                    for (int xx = 0; xx < _block.x; ++xx) {
                        seq = (seq >> 4) | ((*rgbiPixel) << 28);
                        ++rgbiPixel;
                        //_window->addColour(static_cast<UInt64>(seq) |
                        //    (static_cast<UInt64>(xx & 3) << 32));
                    }
                }

                int address = (y/_block.y)*(_size.x/_hdots) + x/_hdots;
                if ((_mode & 2) == 0)
                    _data[address] = bestPattern;
                else {
                    int bit = (x & 12) ^ 4;
                    _data[address] =
                        (_data[address] & ~(15 << bit)) | (bestPattern << bit);
                }

                const Byte* inputRow2 = _inputRow;
                Byte* errorRow2 = _errorRow;
                for (int yy = 0; yy < _block.y; ++yy) {
                    const SInt16* inputPixel =
                        reinterpret_cast<const SInt16*>(inputRow2) + x;
                    int* errorPixel = reinterpret_cast<int*>(errorRow2) + x;
                    for (int xx = 0; xx < w; ++xx) {
                        int test = p[(bestPattern*_block.y + yy)*w + xx];
                        int target = inputPixel[xx] + errorPixel[xx]/4;
                        int d = target - test;
                        int weight = (xx == 0 || xx == _block.x ? 1 : 2);
                        lineScore += weight*d*d;
                        int error = weight*d;
                        errorPixel[xx + 4] += (error*_diffusionHorizontal)/256;
                        reinterpret_cast<int*>(errorRow2 + _error.stride())[x + xx]
                            += (error*_diffusionVertical/256);
                    }
                    inputRow2 += _ntscInput.stride();
                    errorRow2 += _error.stride();
                }
            }
            _program->updateOutput();
            bool advance = false;
            if (_testConfig) {
                if (lineScore < _configScore) {
                    _configScore = lineScore;
                    _bestConfig = _config;
                }
                ++_config;
                if (_config == _endConfig) {
                    _config = _bestConfig;
                    _configs[y] = _bestConfig;
                    _testConfig = false;
                    _configScore = 0x7fffffffffffffffUL;
                }
                else {
                    savedError.copyTo(_error.subBitmap(Vector(0, y),
                        errorLineSize));
                    _error.subBitmap(Vector(0, y + 1), errorLineSize).fill(0);
                }
            }
            else {
                advance = true;
                _testConfig = (_startConfig + 1 != _endConfig);
                _config = _startConfig;
            }
            if (advance) {
                _inputRow += _ntscInput.stride() * _block.y;
                _errorRow += _error.stride() * _block.y;
                _rgbiRow += _rgbi.stride() * _block.y;
                y += _block.y;
                if (y >= _size.y + 1 - _block.y)
                    return;
            }
            if (cancelling())
                return;
        }
    }
    void config()
    {
        switch (_config) {
            case 0x50:
            case 0x51:
                _block.x = 16;
                _patternCount = 0x10000;
                break;
            case 0xd0:
            case 0xd1:
                _block.x = 8;
                _patternCount = 0x10000;
                break;
            default:
                _block.x = 4;
                _patternCount = 16;
                break;
        }
        if ((_config & 0x80) == 0)
            _hdots = 16;
        else
            _hdots = 8;
    }
    void save(String outputFileName)
    {
        File(outputFileName, true).
            save(reinterpret_cast<const Byte*>(&_data[0]), _data.count()*2);
    }
    void saveRGBI(String outputFileName)
    {
        FileStream stream = File(outputFileName, true).openWrite();
        const Byte* rgbiRow = _rgbi.data() + 7;
        for (int y = 0; y < _size.y; ++y) {
            stream.write(reinterpret_cast<const void*>(rgbiRow), _size.x);
            rgbiRow += _rgbi.stride();
        }
    }
    void savePalettes(String outputFileName)
    {
        if (_startConfig + 1 == _endConfig)
            return;
        FileStream stream = File(outputFileName, true).openWrite();
        for (int y = 0; y < _size.y; ++y) {
            int c = _configs[y];
            if ((_mode & 0x80) != 0)
                stream.write<Byte>(c == 80 ? 0x08 : (c < 64 ? 0x0a : 0x1a));
            if (c == 80)
                stream.write<Byte>(0);
            else
                if (c >= 64)
                    stream.write<Byte>(c & 0x0f);
                else
                    if (c >= 16 && c < 48)
                        stream.write<Byte>(c ^ 0x30);
                    else
                        stream.write<Byte>(c);
        }
    }
    void plotPattern(Byte* rgbi, int pattern, int line)
    {
        int modeAndPalette = modeAndPaletteFromConfig(_config);
        UInt8 latch = 0;
        if ((modeAndPalette & 3) == 2)
            pattern <<= 4;
        UInt64 r = _sequencer.process(pattern, modeAndPalette & 0xff,
            modeAndPalette >> 8, line / _scanlinesRepeat, false, 0, &latch);
        int hdots = 8;
        if ((modeAndPalette & 3) == 0) {
            // For -HRES-GRPH need 16 hdots
            hdots = 16;
        }
        for (int x = 0; x < hdots; ++x)
            rgbi[x] = (r >> (x * 4)) & 0x0f;
    }
    int modeAndPaletteFromConfig(int config)
    {
        int b = _mode & 0x24;
        if (config < 0x40)
            return (config << 8) | 0x0a | b;
        if (config < 0x50)
            return ((config & 0x0f) << 8) | 0x1a | b;
        if (config == 0x50)
            return 0x08 | b;
        if (config == 0x51)
            return 0x18 | b;
        if (config < 0xc0)
            return ((config & 0x3f) << 8) | 0x0b | b;
        if (config < 0xd0)
            return ((config & 0x0f) << 8) | 0x1b | b;
        if (config == 0xd0)
            return 0x09 | b;
        return 0x19 | b;
    }

    void setDiffusionHorizontal(double diffusionHorizontal)
    {
        _diffusionHorizontal = static_cast<int>(diffusionHorizontal*256);
    }
    double getDiffusionHorizontal() { return _diffusionHorizontal/256.0; }
    void setDiffusionVertical(double diffusionVertical)
    {
        _diffusionVertical = static_cast<int>(diffusionVertical*256);
    }
    double getDiffusionVertical() { return _diffusionVertical/256.0; }
    void setDiffusionTemporal(double diffusionTemporal)
    {
        _diffusionTemporal = static_cast<int>(diffusionTemporal*256);
    }
    double getDiffusionTemporal() { return _diffusionTemporal/256.0; }
    void setMode(int mode) { _mode = mode; }
    int getMode() { return _mode; }
    void setPalette(int palette) { _palette = palette; }
    int getPalette() { return _palette; }
    void setScanlinesPerRow(int v) { _scanlinesPerRow = v; }
    int getScanlinesPerRow() { return _scanlinesPerRow; }
    void setScanlinesRepeat(int v) { _scanlinesRepeat = v; }
    int getScanlinesRepeat() { return _scanlinesRepeat; }
    void setROM(File rom) { _sequencer.setROM(rom); }
    void setNewCGA(bool newCGA) { _composite.setNewCGA(newCGA); }
    void setPhase(int phase) { _phase = phase; }
    int getPhase() { return _phase; }
    void setInterlace(int interlace) { _interlace = interlace; }
    int getInterlace() { return _interlace; }
    void setQuality(double quality) { _quality = quality; }
    double getQuality() { return _quality; }
    void setCharacterSet(int characterSet) { _characterSet = characterSet; }
    int getCharacterSet() { return _characterSet; }

    bool getNTSCPrimaries() { return _decoder.getNTSCPrimaries(); }
    void setNTSCPrimaries(bool ntscPrimaries)
    {
        _decoder.setNTSCPrimaries(ntscPrimaries);
    }
    double getHue() { return _decoder.getHue(); }
    void setHue(double hue) { _decoder.setHue(hue); }
    double getSaturation() { return _decoder.getSaturation(); }
    void setSaturation(double saturation)
    {
        _decoder.setSaturation(saturation);
    }
    double getContrast() { return _decoder.getContrast(); }
    void setContrast(double contrast) { _decoder.setContrast(contrast); }
    double getBrightness() { return _decoder.getBrightness(); }
    void setBrightness(double brightness)
    {
        _decoder.setBrightness(brightness);
    }

private:
    int _phase;
    int _mode;
    int _palette;
    int _scanlinesPerRow;
    int _scanlinesRepeat;
    Vector _size;
    Bitmap<SRGB> _input;
    Bitmap<Byte> _rgbi;
    CGAComposite _composite;
    NTSCDecoder _decoder;
    Program* _program;
    CGASequencer _sequencer;
    Array<SInt16> _patterns;
    Bitmap<SRGB> _input2;
    const Byte* _inputRow;
    Byte* _rgbiRow;
    Byte* _errorRow;
    int _y;
    Array<Word> _data;
    Bitmap<SInt16> _ntscInput;
    int _patternCount;
    Bitmap<int> _error;
    Bitmap<int> _testError;
    int _hdots;
    Vector _block;
    int _diffusionHorizontal;
    int _diffusionVertical;
    int _diffusionTemporal;
    int _interlace;
    double _quality;
    int _characterSet;
    UInt64 _configScore;
    Array<bool> _skip;

    // a config is a mode/palette combination suitable for auto testing
    // The configs are:
    //   0x00..0x3f = 2bpp (background in low 4 bits)
    //   0x40..0x4f = 1bpp
    //   0x50       = 40-column text
    //   0x51       = 40-column text with 1bpp graphics
    //   0x80..0xbf = high-res 2bpp
    //   0xc0..0xcf = 1bpp odd bits ignored
    //   0xd0       = 80-column text
    //   0xd1       = 80-column text with 1bpp graphics
    Array<int> _configs;
    int _startConfig;
    int _endConfig;
    int _config;
    bool _testConfig;
    int _bestConfig;
};

typedef CGAMatcherT<void> CGAMatcher;

//class Particle
//{
//public:
//    void plot(Bitmap<DWORD> bitmap, Vector rPosition)
//    {
//        Vector size = bitmap.size();
//        Byte* buffer = bitmap.data();
//        int byteWidth = bitmap.stride();
//        double zOffset = rPosition.x*0.01;
//        double scale = rPosition.y*0.01;
//        double x = _position.x/(_position.z + zOffset)*scale;
//        double y = _position.y/(_position.z + zOffset)*scale;
//        int x0 = static_cast<int>(size.x*x/5.0 + size.x/2);
//        int y0 = static_cast<int>(size.x*y/5.0 + size.y/2);
//        int r = byteClamp(_colour.x);
//        int g = byteClamp(_colour.y);
//        int b = byteClamp(_colour.z);
//        DWord c = (r << 16) | (g << 8) | b;
//        plot(bitmap, Vector(x0,     y0    ), c);
//        if (!_big) {
//            if (r < 16 && g < 16 && b < 16) {
//                c = 0xffffff;
//                plot(bitmap, Vector(x0 - 1, y0 - 1), c);
//                plot(bitmap, Vector(x0 + 1, y0 - 1), c);
//                plot(bitmap, Vector(x0 - 1, y0 + 1), c);
//                plot(bitmap, Vector(x0 + 1, y0 + 1), c);
//            }
//            return;
//        }
//        plot(bitmap, Vector(x0 - 1, y0 - 2), c);
//        plot(bitmap, Vector(x0,     y0 - 2), c);
//        plot(bitmap, Vector(x0 + 1, y0 - 2), c);
//        plot(bitmap, Vector(x0 - 2, y0 - 1), c);
//        plot(bitmap, Vector(x0 - 1, y0 - 1), c);
//        plot(bitmap, Vector(x0,     y0 - 1), c);
//        plot(bitmap, Vector(x0 + 1, y0 - 1), c);
//        plot(bitmap, Vector(x0 + 2, y0 - 1), c);
//        plot(bitmap, Vector(x0 - 2, y0    ), c);
//        plot(bitmap, Vector(x0 - 1, y0    ), c);
//        plot(bitmap, Vector(x0 + 1, y0    ), c);
//        plot(bitmap, Vector(x0 + 2, y0    ), c);
//        plot(bitmap, Vector(x0 - 2, y0 + 1), c);
//        plot(bitmap, Vector(x0 - 1, y0 + 1), c);
//        plot(bitmap, Vector(x0,     y0 + 1), c);
//        plot(bitmap, Vector(x0 + 1, y0 + 1), c);
//        plot(bitmap, Vector(x0 + 2, y0 + 1), c);
//        plot(bitmap, Vector(x0 - 1, y0 + 2), c);
//        plot(bitmap, Vector(x0,     y0 + 2), c);
//        plot(bitmap, Vector(x0 + 1, y0 + 2), c);
//        if (r < 16 && g < 16 && b < 16) {
//            c = 0xffffff;
//            plot(bitmap, Vector(x0 - 1, y0 - 3), c);
//            plot(bitmap, Vector(x0,     y0 - 3), c);
//            plot(bitmap, Vector(x0 + 1, y0 - 3), c);
//            plot(bitmap, Vector(x0 - 1, y0 + 3), c);
//            plot(bitmap, Vector(x0,     y0 + 3), c);
//            plot(bitmap, Vector(x0 + 1, y0 + 3), c);
//            plot(bitmap, Vector(x0 - 2, y0 - 2), c);
//            plot(bitmap, Vector(x0 + 2, y0 - 2), c);
//            plot(bitmap, Vector(x0 - 3, y0 - 1), c);
//            plot(bitmap, Vector(x0 + 3, y0 - 1), c);
//            plot(bitmap, Vector(x0 - 3, y0    ), c);
//            plot(bitmap, Vector(x0 + 3, y0    ), c);
//            plot(bitmap, Vector(x0 - 3, y0 + 1), c);
//            plot(bitmap, Vector(x0 + 3, y0 + 1), c);
//            plot(bitmap, Vector(x0 - 2, y0 + 2), c);
//            plot(bitmap, Vector(x0 + 2, y0 + 2), c);
//        }
//    }
//    void plot(Bitmap<DWORD> bitmap, Vector p, DWord c)
//    {
//        if (p.inside(bitmap.size()))
//            bitmap[p] = c;
//    }
//    bool operator<(const Particle& other) const { return _position.z > other._position.z; }
//    void transform(double* matrix)
//    {
//        Vector3<double> c = (_colour - Vector3<double>(128.0, 128.0, 128.0))/128.0;
//        _position.x = c.x*matrix[0] + c.y*matrix[1] + c.z*matrix[2];
//        _position.y = c.x*matrix[3] + c.y*matrix[4] + c.z*matrix[5];
//        _position.z = c.x*matrix[6] + c.y*matrix[7] + c.z*matrix[8];
//    }
//
//    Colour _colour;
//    Vector3<double> _position;
//    bool _big;
//};
//
//class GamutWindow : public BitmapWindow
//{
//public:
//    GamutWindow()
//      : _lButton(false), _rButton(false), _rPosition(1000, 1000), _particle(0),
//        _delta(0, 0)
//    {
//        _matrix[0] = 1; _matrix[1] = 0; _matrix[2] = 0;
//        _matrix[3] = 0; _matrix[4] = 1; _matrix[5] = 0;
//        _matrix[6] = 0; _matrix[7] = 0; _matrix[8] = 1;
//        setSize(Vector(640, 480));
//    }
//    void create()
//    {
//        BitmapWindow::create();
//        reset();
//        draw();
//        invalidate();
//    }
//    void setAnimated(AnimatedWindow* animated) { _animated = animated; }
//    void paint()
//    {
//        _animated->restart();
//    }
//    void draw()
//    {
//        if (_delta.modulus2() >= 0.000001)
//            animate();
//        else
//            _animated->stop();
//        if (!_bitmap.valid())
//            _bitmap = Bitmap<DWORD>(Vector(640, 480));
//        _bitmap.fill(0);
//        for (int i = 0; i < _particle; ++i)
//            _particles[i].transform(_matrix);
//        std::sort(&_particles[0], &_particles[_particle]);
//        for (int i = 0; i < _particle; ++i)
//            _particles[i].plot(_bitmap, _rPosition);
//        _bitmap = setNextBitmap(_bitmap);
//    }
//    void line(Colour c1, Colour c2)
//    {
//        for (int i = 0; i < 101; ++i)
//            add(c1*(i*0.01) + c2*((100-i)*0.01), false);
//    }
//    void reset()
//    {
//        _particle = 0;
//        line(Colour(0, 0, 0), Colour(255, 0, 0));
//        line(Colour(0, 0, 0), Colour(0, 255, 0));
//        line(Colour(0, 0, 0), Colour(0, 0, 255));
//        line(Colour(255, 0, 0), Colour(255, 255, 0));
//        line(Colour(255, 0, 0), Colour(255, 0, 255));
//        line(Colour(0, 255, 0), Colour(255, 255, 0));
//        line(Colour(0, 255, 0), Colour(0, 255, 255));
//        line(Colour(0, 0, 255), Colour(0, 255, 255));
//        line(Colour(0, 0, 255), Colour(255, 0, 255));
//        line(Colour(255, 255, 0), Colour(255, 255, 255));
//        line(Colour(255, 0, 255), Colour(255, 255, 255));
//        line(Colour(0, 255, 255), Colour(255, 255, 255));
//    }
//    void add(Colour c, bool big = true)
//    {
//        Particle p;
//        p._colour = c;
//        p._big = big;
//        if (_particle >= _particles.count())
//            _particles.append(p);
//        else
//            _particles[_particle] = p;
//        ++_particle;
//    }
//    bool mouseInput(Vector position, int buttons)
//    {
//        bool oldLButton = _lButton;
//        bool mouseDown = false;
//        if ((buttons & MK_LBUTTON) != 0 && !_lButton) {
//            _lButton = true;
//            mouseDown = true;
//            _lastPosition = position;
//        }
//        if ((buttons & MK_LBUTTON) == 0)
//            _lButton = false;
//        if ((buttons & MK_RBUTTON) != 0 && !_rButton) {
//            _rButton = true;
//            mouseDown = true;
//            _lastPosition = position;
//        }
//        if ((buttons & MK_RBUTTON) == 0)
//            _rButton = false;
//        if (_lButton) {
//            _delta = Vector2Cast<double>(position - _lastPosition)*0.01;
//            if (position != _lastPosition)
//                update();
//            _lastPosition = position;
//        }
//        else
//            if (oldLButton && _delta.modulus2() >= 0.000001)
//                _animated->start();
//        if (_rButton && position != _lastPosition) {
//            _rPosition += (position - _lastPosition);
//            _lastPosition = position;
//        }
//
//        return mouseDown;
//    }
//    void animate()
//    {
//        _rotor =
//            Rotor3<double>::yz(-_delta.y)*Rotor3<double>::zx(_delta.x)*_rotor;
//        _rotor.toMatrix(_matrix);
//        _delta *= 0.95;
//    }
//    void update()
//    {
//        animate();
//        invalidate();
//    }
//
//    Rotor3<double> _rotor;
//    double _matrix[9];
//    AppendableArray<Particle> _particles;
//    Vector _lastPosition;
//    Vector _rPosition;
//    bool _lButton;
//    bool _rButton;
//    Vector2<double> _delta;
//    AnimatedWindow* _animated;
//    Bitmap<DWORD> _bitmap;
//
//    int _particle;
//};
//
template<class T> class NumericSliderWindowT
{
public:
    void setText(String text) { _caption.setText(text); }
    void setHost(CGA2NTSCWindow* host)
    {
        _host = host;
        host->add(&_slider);
        host->add(&_caption);
        host->add(&_text);
        _slider.setHost(this);
    }
    void setPositionAndSize(Vector position, Vector size)
    {
        _slider.setSize(size);
        _slider.setPosition(position);
        _caption.setPosition(_slider.bottomLeft() + Vector(0, 15));
        _text.setPosition(_caption.topRight());
    }
    Vector bottomLeft() { return _caption.bottomLeft(); }
    int right() const { return _slider.right(); }
    void setRange(double low, double high)
    {
        _slider.setRange(positionFromValue(low), positionFromValue(high));
    }
    void setValue(double value) { _slider.setValue(positionFromValue(value)); }
    double getValue() { return valueFromPosition(_slider.getValue()); }

protected:
    virtual void create() { }
    virtual void valueSet(double value) { }
    virtual double positionFromValue(double value) { return value; }
    virtual double valueFromPosition(double position) { return position; }

    CGA2NTSCWindow* _host;
private:
    void valueSet1(double value)
    {
        double v = valueFromPosition(value);
        _text.setText(format("%f", v));
        _text.size();
        valueSet(v);
    }

    class NumericSlider : public Slider
    {
    public:
        void setHost(NumericSliderWindowT* host) { _host = host; }
        void valueSet(double value) { _host->valueSet1(value); }
        void create() { _host->create(); Slider::create(); }
    private:
        NumericSliderWindowT* _host;
    };
    NumericSlider _slider;
    TextWindow _caption;
    TextWindow _text;
    friend class NumericSlider;
};
typedef NumericSliderWindowT<void> NumericSliderWindow;

template<class T> class KnobSliderT
{
public:
    KnobSliderT() : _knob(this), _sliding(false) { }
    void create()
    {
        _caption.size();
        _popup.create();
    }
    void setText(String text) { _caption.setText(text); }
    void setHost(CGA2NTSCWindow* host)
    {
        _host = host;
        host->add(&_caption);
        host->add(&_knob);
        host->add(&_edit);
    }
    void setPositionAndSize(Vector position, Vector size)
    {
        _caption.setPosition(position);
        _knob.setSize(Vector(size.y, size.y));
        _knob.setPosition(Vector(_caption.right(), position.y));
        _edit.setPosition(_knob.topRight());
    }
    Vector bottomLeft() { return _caption.bottomLeft(); }
    int right() const { return _edit.right(); }
    void setRange(double low, double high)
    {
        _slider.setRange(positionFromValue(low), positionFromValue(high));
    }

protected:
    CGA2NTSCWindow* _host;
private:
    void knobEvent(Vector position, bool buttonDown)
    {
        if (buttonDown && !_sliding) {
            _popup.show(SW_SHOW);
        }
        if (!buttonDown && _sliding) {
            _popup.show(SW_HIDE);
        }
        _sliding = buttonDown;
    }

    class KnobWindow : public BitmapWindow
    {
    public:
        KnobWindow(KnobSliderT* host) : _host(host) { }
        bool mouseInput(Vector position, int buttons)
        {
            bool lButton = (buttons & MK_LBUTTON) != 0;
            _host->knobEvent(position, lButton);
            return lButton;  // Capture when button is down
        }
    private:
        KnobSliderT* _host;
    };
    class PopupWindow : public BitmapWindow
    {
    public:
        PopupWindow()
        {
            HDC hdcScreen = GetDC(NULL);
            _hdcBackBuffer = CreateCompatibleDC(hdcScreen);
            _hbmBackBuffer = CreateCompatibleBitmap(hdcScreen, 100, 100);
            _hbmOld = SelectObject(_hdcBackBuffer, _hbmBackBuffer);
        }
        ~PopupWindow()
        {
            SelectObject(_hdcBackBuffer, _hbmOld);

        }

        void create()
        {
            setStyle(WS_VISIBLE | SS_OWNERDRAW | WS_EX_LAYERED);
            setSize(100, 100);
        }
        virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            switch (uMsg) {
                case WM_PAINT:
                {
                    PaintHandle paintHandle(this);
                    if (paintHandle.zeroArea() || !_bitmap.valid())
                        return 0;
                    Vector ptl = paintHandle.topLeft();
                    Vector pbr = paintHandle.bottomRight();
                    Vector br = size();
                    pbr = Vector(min(pbr.x, br.x), min(pbr.y, br.y));
                    Vector ps = pbr - ptl;
                    if (ps.x <= 0 || ps.y <= 0)
                        return 0;
                    Vector s = size();
                    _bmi.bmiHeader.biWidth = s.x;
                    _bmi.bmiHeader.biHeight = -s.y;
                    paint();
                    IF_ZERO_THROW(SetDIBitsToDevice(
                        _hdcBackBuffer,
                        ptl.x,
                        ptl.y,
                        ps.x,
                        ps.y,
                        ptl.x,
                        s.y - pbr.y,
                        0,
                        s.y,
                        _bitmap.data(),
                        &_bmi,
                        DIB_RGB_COLORS));

                    POINT ptSrc;
                    ptSrc.x = 0;
                    ptSrc.y = 0;
                    BOOL r = UpdateLayeredWindow(_hWnd,)
                    return 0;
                }
            }
            return BitmapWindow::handleMessage(uMsg, wParam, lParam);
        }
    private:
        HDC _hdcBackBuffer;
        HBITMAP _hbmBackBuffer;
        HGDIOBJ _hbmOld;
    };

    TextWindow _caption;
    KnobWindow _knob;
    EditWindow _edit;
    PopupWindow _popup;
    bool _sliding;

    friend class KnobWindow;
};
typedef KnobSliderT<void> KnobSlider;

template<class T> class BrightnessSliderWindowT : public KnobSlider
{
public:
    void valueSet(double value) { _host->brightnessSet(value); }
    void create()
    {
        setText("Brightness: ");
        setRange(-2, 2);
    }
};
typedef BrightnessSliderWindowT<void> BrightnessSliderWindow;

template<class T> class SaturationSliderWindowT : public NumericSliderWindow
{
public:
    void valueSet(double value) { _host->saturationSet(value); }
    void create()
    {
        setText("Saturation: ");
        setRange(0, 4);
    }
};
typedef SaturationSliderWindowT<void> SaturationSliderWindow;

template<class T> class ContrastSliderWindowT: public NumericSliderWindow
{
public:
    void valueSet(double value) { _host->contrastSet(value); }
    void create()
    {
        setText("Contrast: ");
        setRange(0, 4);
    }
};
typedef ContrastSliderWindowT<void> ContrastSliderWindow;

template<class T> class HueSliderWindowT : public NumericSliderWindow
{
public:
    void valueSet(double value) { _host->hueSet(value); }
    void create()
    {
        setText("Hue: ");
        setRange(-180, 180);
    }
};
typedef HueSliderWindowT<void> HueSliderWindow;

template<class T> class ChromaBandwidthSliderWindowT
    : public NumericSliderWindow
{
public:
    void valueSet(double value) { _host->chromaBandwidthSet(value); }
    void create()
    {
        setText("Chroma bandwidth: ");
        setRange(0, 2);
    }
};
typedef ChromaBandwidthSliderWindowT<void> ChromaBandwidthSliderWindow;

template<class T> class LumaBandwidthSliderWindowT : public NumericSliderWindow
{
public:
    void valueSet(double value) { _host->lumaBandwidthSet(value); }
    void create()
    {
        setText("Luma bandwidth: ");
        setRange(0, 2);
    }
};
typedef LumaBandwidthSliderWindowT<void> LumaBandwidthSliderWindow;

//template<class T> class AutoBrightnessButtonWindowT
//  : public ToggleButton
//{
//public:
//    void setHost(CGA2NTSCWindow* host) { _host = host; }
//    void clicked() { _host->autoBrightnessPressed(); }
//    void create()
//    {
//        setText("Auto");
//        ToggleButton::create();
//    }
//private:
//    CGA2NTSCWindow* _host;
//};
//typedef AutoBrightnessButtonWindowT<void> AutoBrightnessButtonWindow;
//
//template<class T> class AutoContrastClipButtonWindowT
//  : public ToggleButton
//{
//public:
//    void setHost(CGA2NTSCWindow* host) { _host = host; }
//    void clicked() { _host->autoContrastClipPressed(); }
//    void create()
//    {
//        setText("No clipping");
//        ToggleButton::create();
//    }
//private:
//    CGA2NTSCWindow* _host;
//};
//typedef AutoContrastClipButtonWindowT<void>
//    AutoContrastClipButtonWindow;
//
//template<class T> class AutoSaturationButtonWindowT
//  : public ToggleButton
//{
//public:
//    void setHost(CGA2NTSCWindow* host) { _host = host; }
//    void clicked() { _host->autoSaturationPressed(); }
//    void create()
//    {
//        setText("Auto");
//        ToggleButton::create();
//    }
//private:
//    CGA2NTSCWindow* _host;
//};
//typedef AutoSaturationButtonWindowT<void> AutoSaturationButtonWindow;
//
//template<class T> class AutoContrastMonoButtonWindowT
//  : public ToggleButton
//{
//public:
//    void setHost(CGA2NTSCWindow* host) { _host = host; }
//    void clicked() { _host->autoContrastMonoPressed(); }
//    void create()
//    {
//        setText("Fix black and white");
//        ToggleButton::create();
//    }
//private:
//    CGA2NTSCWindow* _host;
//};
//typedef AutoContrastMonoButtonWindowT<void>
//    AutoContrastMonoButtonWindow;
//
template<class T> class NewCGAButtonWindowT : public ToggleButton
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void clicked() { _host->newCGAPressed(); }
    void create()
    {
        setText("New CGA");
        ToggleButton::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef NewCGAButtonWindowT<void> NewCGAButtonWindow;

template<class T> class NTSCPrimariesButtonWindowT : public ToggleButton
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void clicked() { _host->ntscPrimariesPressed(); }
    void create()
    {
        setText("NTSC Primaries");
        ToggleButton::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef NTSCPrimariesButtonWindowT<void> NTSCPrimariesButtonWindow;

template<class T> class MatchModeButtonT : public ToggleButton
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void clicked() { _host->matchModePressed(); }
    void create()
    {
        setText("Match");
        ToggleButton::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef MatchModeButtonT<void> MatchModeButton;

template<class T> class ModeComboT : public ComboBox
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void changed(int value) { _host->modeSet(value); }
    void create()
    {
        ComboBox::create();
        add("40 column text");
        add("80 column text");
        add("1bpp graphics");
        add("2bpp graphics");
        add("40 column text with 1bpp graphics");
        add("80 column text with 1bpp graphics");
        add("1bpp graphics, odd bits ignored");
        add("high-res 2bpp graphics");
        add("Auto -HRES");
        add("Auto +HRES");
        set(2);
        autoSize();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef ModeComboT<void> ModeCombo;

template<class T> class BackgroundComboT : public ComboBox
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void changed(int value) { _host->backgroundSet(value); }
    void create()
    {
        ComboBox::create();
        for (int i = 0; i < 16; ++i)
            add(decimal(i));
        add("Auto");
        set(15);
        autoSize();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef BackgroundComboT<void> BackgroundCombo;

template<class T> class ScanlinesPerRowComboT : public ComboBox
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void changed(int value) { _host->scanlinesPerRowSet(value); }
    void create()
    {
        ComboBox::create();
        for (int i = 1; i <= 32; ++i)
            add(decimal(i));
        set(1);
        autoSize();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef ScanlinesPerRowComboT<void> ScanlinesPerRowCombo;

template<class T> class ScanlinesRepeatComboT : public ComboBox
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void changed(int value) { _host->scanlinesRepeatSet(value); }
    void create()
    {
        ComboBox::create();
        for (int i = 1; i <= 32; ++i)
            add(decimal(i));
        set(0);
        autoSize();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef ScanlinesRepeatComboT<void> ScanlinesRepeatCombo;

template<class T> class PaletteComboT : public ComboBox
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void changed(int value) { _host->paletteSet(value); }
    void create()
    {
        ComboBox::create();
        add("2/4/6");
        add("10/12/14");
        add("3/5/7");
        add("11/13/15");
        set(3);
        autoSize();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef PaletteComboT<void> PaletteCombo;

template<class T> class DiffusionHorizontalSliderWindowT
  : public NumericSliderWindow
{
public:
    void valueSet(double value) { _host->diffusionHorizontalSet(value); }
    void create()
    {
        setText("Horizontal diffusion: ");
        setRange(0, 1);
    }
};
typedef DiffusionHorizontalSliderWindowT<void> DiffusionHorizontalSliderWindow;

template<class T> class DiffusionVerticalSliderWindowT
  : public NumericSliderWindow
{
public:
    void valueSet(double value) { _host->diffusionVerticalSet(value); }
    void create()
    {
        setText("Vertical diffusion: ");
        setRange(0, 1);
    }
};
typedef DiffusionVerticalSliderWindowT<void> DiffusionVerticalSliderWindow;

template<class T> class DiffusionTemporalSliderWindowT
  : public NumericSliderWindow
{
public:
    void valueSet(double value) { _host->diffusionTemporalSet(value); }
    void create()
    {
        setText("Temporal diffusion: ");
        setRange(0, 1);
    }
};
typedef DiffusionTemporalSliderWindowT<void> DiffusionTemporalSliderWindow;

template<class T> class QualitySliderWindowT : public NumericSliderWindow
{
public:
    void valueSet(double value) { _host->qualitySet(value); }
    void create()
    {
        setText("Quality: ");
        setRange(0, 1);
    }
};
typedef QualitySliderWindowT<void> QualitySliderWindow;

template<class T> class BWCheckBoxT : public CheckBox
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void clicked() { _host->bwPressed(); }
    void create()
    {
        setText("+BW");
        CheckBox::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef BWCheckBoxT<void> BWCheckBox;

template<class T> class BlinkCheckBoxT : public CheckBox
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void clicked() { _host->blinkPressed(); }
    void create()
    {
        setText("+BLINK");
        CheckBox::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef BlinkCheckBoxT<void> BlinkCheckBox;

template<class T> class PhaseCheckBoxT : public CheckBox
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void clicked() { _host->phasePressed(); }
    void create()
    {
        setText("Phase 0");
        CheckBox::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef PhaseCheckBoxT<void> PhaseCheckBox;

template<class T> class InterlaceComboT : public ComboBox
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void changed(int value) { _host->interlaceSet(value); }
    void create()
    {
        ComboBox::create();
        add("None");
        add("Flicker");
        add("Sync");
        add("Sync and video");
        add("Even");
        add("Odd");
        add("Video");
        add("Video and flicker");
        add("Sync flicker");
        add("Sync video and flicker");
        add("Even flicker");
        add("Odd flicker");
        add("Sync even");
        add("Sync odd");
        add("Sync even flicker");
        add("Sync odd flicker");
        add("Sync and video swapped");
        add("Sync video and flicker swapped");
        set(0);
        autoSize();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef InterlaceComboT<void> InterlaceCombo;

template<class T> class CharacterSetComboT : public ComboBox
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void changed(int value) { _host->characterSetSet(value); }
    void create()
    {
        ComboBox::create();
        add("0xdd");
        add("0x13/0x55");
        add("1K");
        add("all");
        add("0xb1");
        add("0xb0/0xb1");
        add("ISAV");
        set(3);
        autoSize();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef CharacterSetComboT<void> CharacterSetCombo;

template<class T> class ScanlineWidthSliderWindowT : public NumericSliderWindow
{
public:
    void valueSet(double value) { _host->scanlineWidthSet(value); }
    void create()
    {
        setText("Scanline width: ");
        setRange(0, 1);
    }
};
typedef ScanlineWidthSliderWindowT<void> ScanlineWidthSliderWindow;

template<class T> class ScanlineProfileComboT : public ComboBox
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void changed(int value) { _host->characterSetSet(value); }
    void create()
    {
        ComboBox::create();
        add("rectangular");
        add("triangle");
        add("semicircle");
        add("gaussian");
        set(0);
        autoSize();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef ScanlineProfileComboT<void> ScanlineProfileCombo;

template<class T> class ScanlineOffsetSliderWindowT
  : public NumericSliderWindow
{
public:
    void valueSet(double value) { _host->scanlineOffsetSet(value); }
    void create()
    {
        setText("Scanline offset: ");
        setRange(0, 1);
    }
};
typedef ScanlineOffsetSliderWindowT<void> ScanlineOffsetSliderWindow;

template<class T> class ZoomSliderWindowT : public NumericSliderWindow
{
public:
    void valueSet(double value) { _host->scanlineOffsetSet(value); }
    void create()
    {
        setText("Zoom: ");
        setRange(1, 10);
    }
    double positionFromValue(double value) { return log(value); }
    double valueFromPosition(double position) { return exp(position); }
};
typedef ZoomSliderWindowT<void> ZoomSliderWindow;

template<class T> class ScanlineBleedingCheckBoxT : public CheckBox
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void clicked() { _host->scanlineBleedingPressed(); }
    void create()
    {
        setText("Scanline bleeding");
        CheckBox::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef ScanlineBleedingCheckBoxT<void> ScanlineBleedingCheckBox;

template<class T> class AspectRatioSliderWindowT : public NumericSliderWindow
{
public:
    void valueSet(double value) { _host->aspectRatioSet(value); }
    void create()
    {
        setText("Aspect Ratio: ");
        setRange(1, 2);
    }
};
typedef AspectRatioSliderWindowT<void> AspectRatioSliderWindow;

template<class T> class CombFilterVerticalComboT : public ComboBox
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void changed(int value) { _host->combFilterVerticalSet(value); }
    void create()
    {
        ComboBox::create();
        add("none");
        add("(1, 1)");
        add("(1, 2, 1)");
        set(0);
        autoSize();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef CombFilterVerticalComboT<void> CombFilterVerticalCombo;

template<class T> class CombFilterTemporalComboT : public ComboBox
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void changed(int value) { _host->combFilterTemporalSet(value); }
    void create()
    {
        ComboBox::create();
        add("none");
        add("(1, 1)");
        add("(1, 2, 1)");
        set(0);
        autoSize();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef CombFilterTemporalComboT<void> CombFilterTemporalCombo;

class OutputWindow : public BitmapWindow
{
public:
    void create()
    {
        Vector size(648, 400);
        setSize(size);
        BitmapWindow::create();
        _bitmap = Bitmap<DWORD>(size);
        setNextBitmap(_bitmap);
    }
    void draw(Bitmap<DWORD> bitmap)
    {
        if (!_bitmap.valid())
            _bitmap = Bitmap<DWORD>(size());
        // Just copy from the top left corner for now.
        Vector zero(0, 0);
        Vector size(min(bitmap.size().x, _bitmap.size().x),
            min(bitmap.size().y, _bitmap.size().y));
        bitmap.subBitmap(zero, size).copyTo(_bitmap.subBitmap(zero, size));
        _bitmap = setNextBitmap(_bitmap);
        invalidate();
    }
private:
    Bitmap<DWORD> _bitmap;
};

template<class T> class CGAOutputT : public ThreadTask
{
public:
    CGAOutputT() : _window(0), _zoom(0), _aspectRatio(1) { }
    void setRGBI(Bitmap<Byte> rgbi)
    {
        _rgbi = rgbi;
        _ntsc = Bitmap<Byte>(_rgbi.size() - Vector(1, 0));
        allocateBitmap();
        reCreateNTSC();
    }
    void reCreateNTSC()
    {
        _composite.initChroma();
        Byte burst[4];
        for (int i = 0; i < 4; ++i)
            burst[i] = _composite.simulateCGA(6, 6, i);
        _decoder.calculateBurst(burst);
        // Convert to raw NTSC
        const Byte* rgbiRow = _rgbi.data();
        Byte* ntscRow = _ntsc.data();
        Vector size = _rgbi.size() - Vector(14, 0);
        for (int y = 0; y < size.y; ++y) {
            const Byte* rgbiPixel = rgbiRow;
            Byte* ntscPixel = ntscRow;
            for (int x = 0; x < size.x + 13; ++x) {
                int left = *rgbiPixel;
                ++rgbiPixel;
                int right = *rgbiPixel;
                *ntscPixel = _composite.simulateCGA(left, right, (x + 1) & 3);
                ++ntscPixel;
            }
            rgbiRow += _rgbi.stride();
            ntscRow += _ntsc.stride();
        }
        restart();
    }
    void run()
    {
        //Timer timer;
        const Byte* ntscRow = _ntsc.data();
        Byte* outputRow = _bitmap.data();
        for (int yy = 0; yy < _ntsc.size().y; ++yy) {
            _decoder.decodeLine(ntscRow, reinterpret_cast<DWORD*>(outputRow),
                _ntsc.size().x - 6, _bitmap.size().x);
            memcpy(outputRow + _bitmap.stride(), outputRow,
                _bitmap.size().x*sizeof(DWORD));
            outputRow += _bitmap.stride()*2;
            ntscRow += _ntsc.stride();
        }
        //timer.output("resampling ");
        if (_window != 0)
            _window->draw(_bitmap);
    }
    void save(String outputFileName)
    {
        _bitmap.subBitmap(Vector(0, 0), requiredSize()).
            save(PNGFileFormat<DWORD>(), File(outputFileName, true));

        FileStream s = File(outputFileName + ".ntsc", true).openWrite();
        s.write(_ntsc.data(), _ntsc.stride()*_ntsc.size().y);
    }

    void setNewCGA(bool newCGA)
    {
        _composite.setNewCGA(newCGA);
        reCreateNTSC();
    }
    bool getNewCGA() { return _composite.getNewCGA(); }
    void setBW(bool bw) { _composite.setBW(bw); reCreateNTSC(); }
    void setScanlineWidth(double width) { _scanlineWidth = width; restart(); }
    double getScanlineWidth() { return _scanlineWidth; }
    void setScanlineProfile(int profile)
    {
        _scanlineProfile = profile;
        restart();
    }
    int getScanlineProfile() { return _scanlineProfile; }
    void setScanlineOffset(double offset)
    {
        _scanlineOffset = offset;
        restart();
    }
    double getScanlineOffset() { return _scanlineOffset; }
    void setZoom(double zoom) { _zoom = zoom; allocateBitmap(); }
    double getZoom() { return _zoom; }
    void setScanlineBleeding(bool bleeding)
    {
        _scanlineBleeding = bleeding;
        restart();
    }
    bool getScanlineBleeding() { return _scanlineBleeding; }
    void setAspectRatio(double ratio)
    {
        _aspectRatio = ratio;
        allocateBitmap();
    }
    double getAspectRatio() { return _aspectRatio; }
    void setCombFilterVertical(int value)
    {
        _combFilterVertical = value;
        restart();
    }
    int getCombFilterVertical() { return _combFilterVertical; }
    void setCombFilterTemporal(int value)
    {
        _combFilterTemporal = value;
        restart();
    }
    int getCombFilterTemporal() { return _combFilterTemporal; }
    ResamplingNTSCDecoder* getDecoder() { return &_decoder; }

    bool getNTSCPrimaries() { return _decoder.getNTSCPrimaries(); }
    void setNTSCPrimaries(bool ntscPrimaries)
    {
        _decoder.setNTSCPrimaries(ntscPrimaries);
        restart();
    }
    double getHue() { return _decoder.getHue(); }
    void setHue(double hue) { _decoder.setHue(hue); restart(); }
    double getSaturation() { return _decoder.getSaturation(); }
    void setSaturation(double saturation)
    {
        _decoder.setSaturation(saturation);
        restart();
    }
    double getContrast() { return _decoder.getContrast(); }
    void setContrast(double contrast)
    {
        _decoder.setContrast(contrast);
        restart();
    }
    double getBrightness() { return _decoder.getBrightness(); }
    void setBrightness(double brightness)
    {
        _decoder.setBrightness(brightness);
        restart();
    }
    double getChromaBandwidth() { return _decoder.getChromaBandwidth(); }
    void setChromaBandwidth(double chromaBandwidth)
    {
        _decoder.setChromaBandwidth(chromaBandwidth);
        restart();
    }
    double getLumaBandwidth() { return _decoder.getLumaBandwidth(); }
    void setLumaBandwidth(double lumaBandwidth)
    {
        _decoder.setLumaBandwidth(lumaBandwidth);
        restart();
    }

    void setWindow(CGA2NTSCWindow* window) { _window = window; }
private:
    void allocateBitmap()
    {
        Vector size = requiredSize();
        if (size.x > _bitmap.size().x || size.y > _bitmap.size().y)
            _bitmap = Bitmap<DWORD>(size);
        restart();
    }
    Vector requiredSize()
    {
        double y = _zoom*_ntsc.size().y;
        double x = _zoom*(_ntsc.size().x - 6)*_aspectRatio/2;
        return Vector(static_cast<int>(x), static_cast<int>(y));
    }

    Bitmap<DWORD> _bitmap;
    Bitmap<Byte> _rgbi;
    Bitmap<Byte> _ntsc;
    CGAComposite _composite;
    ResamplingNTSCDecoder _decoder;
    double _scanlineWidth;
    int _scanlineProfile;
    double _scanlineOffset;
    double _zoom;
    bool _scanlineBleeding;
    double _aspectRatio;
    int _combFilterVertical;
    int _combFilterTemporal;
    CGA2NTSCWindow* _window;
};

typedef CGAOutputT<void> CGAOutput;

template<class T> class CGA2NTSCWindowT : public RootWindow
{
public:
    CGA2NTSCWindowT()
      //: _autoBrightnessFlag(false), _autoSaturationFlag(false),
      //  _autoContrastClipFlag(false), _autoContrastMonoFlag(false),
      //  _updating(true)
    {
        add(&_outputWindow);
        _brightness.setHost(this);
        //add2(&_autoBrightness);
        _saturation.setHost(this);
        //add2(&_autoSaturation);
        _contrast.setHost(this);
        //add2(&_autoContrastClip);
        //add2(&_autoContrastMono);
        _hue.setHost(this);
        _chromaBandwidth.setHost(this);
        _lumaBandwidth.setHost(this);
        //add(&_blackText);
        //add(&_whiteText);
        //add(&_mostSaturatedText);
        //add(&_clippedColoursText);
        //add(&_gamut);
        add2(&_newCGA);
        add2(&_ntscPrimaries);
        //add(&_animated);
        add2(&_matchMode);
        add2(&_mode);
        add2(&_background);
        add2(&_palette);
        add2(&_scanlinesPerRow);
        add2(&_scanlinesRepeat);
        _diffusionHorizontal.setHost(this);
        _diffusionVertical.setHost(this);
        _diffusionTemporal.setHost(this);
        _quality.setHost(this);
        add2(&_bwCheckBox);
        add2(&_blinkCheckBox);
        add2(&_phaseCheckBox);
        add2(&_interlaceCombo);
        add2(&_characterSetCombo);
        _scanlineWidth.setHost(this);
        add2(&_scanlineProfile);
        _scanlineOffset.setHost(this);
        _zoom.setHost(this);
        add2(&_scanlineBleeding);
        _aspectRatio.setHost(this);
        add2(&_combFilterVertical);
        add2(&_combFilterTemporal);
        //_decoder = _output->getDecoder();
    }
    void create()
    {
        //_animated.setDrawWindow(&_gamut);
        //_gamut.setAnimated(&_animated);

        int mode = _matcher->getMode();
        if ((mode & 0x80) != 0)
            _mode.set(8 + (mode & 1));
        else {
            switch (mode & 0x13) {
                case 0: _mode.set(0); break;
                case 1: _mode.set(1); break;
                case 2: _mode.set(3); break;
                case 3: _mode.set(7); break;
                case 0x10: _mode.set(4); break;
                case 0x11: _mode.set(5); break;
                case 0x12: _mode.set(2); break;
                case 0x13: _mode.set(6); break;
            }
        }
        _bwCheckBox.setCheckState((mode & 4) != 0);
        _blinkCheckBox.setCheckState((mode & 0x20) != 0);
        _phaseCheckBox.setCheckState(_matcher->getPhase() == 0);
        _interlaceCombo.set(_matcher->getInterlace());
        int palette = _matcher->getPalette();
        if (palette == 0xff) {
            _paletteSelected = 0;
            _backgroundSelected = 0x10;
        }
        else {
            _paletteSelected = (palette >> 4) & 3;
            _backgroundSelected = palette & 0xf;
        }
        _palette.set(_paletteSelected);
        _background.set(_backgroundSelected);
        _diffusionHorizontal.setValue(_matcher->getDiffusionHorizontal());
        _diffusionVertical.setValue(_matcher->getDiffusionVertical());
        _diffusionTemporal.setValue(_matcher->getDiffusionTemporal());
        _quality.setValue(_matcher->getQuality());
        _characterSetCombo.set(_matcher->getCharacterSet());
        _scanlinesPerRow.set(_matcher->getScanlinesPerRow() - 1);
        _scanlinesRepeat.set(_matcher->getScanlinesRepeat() - 1);
        _ntscPrimaries.setCheckState(_output->getNTSCPrimaries());
        _brightness.setValue(_output->getBrightness());
        _saturation.setValue(_output->getSaturation());
        _hue.setValue(_output->getHue());
        _contrast.setValue(_output->getContrast());
        _chromaBandwidth.setValue(_output->getChromaBandwidth());
        _lumaBandwidth.setValue(_output->getLumaBandwidth());
        _newCGA.setCheckState(_output->getNewCGA());
        _scanlineWidth.setValue(_output->getScanlineWidth());
        _scanlineProfile.set(_output->getScanlineProfile());
        _scanlineOffset.setValue(_output->getScanlineOffset());
        _zoom.setValue(_output->getZoom());
        _scanlineBleeding.setCheckState(_output->getScanlineBleeding());
        _aspectRatio.setValue(_output->getAspectRatio());
        _combFilterVertical.set(_output->getCombFilterVertical());
        _combFilterTemporal.set(_output->getCombFilterTemporal());
        _matchMode.setCheckState(_program->getMatchMode());
        matchModePressed();

        setText("CGA to NTSC");
        setSize(Vector(640, 480));

        RootWindow::create();

        sizeSet(size());
        setSize(Vector(_brightness.right() + 20, _outputWindow.bottom() + 20));
        //setSize(Vector(_brightness.right() + 20, _gamut.bottom() + 20));

        //_updating = false;
        //update();
        //uiUpdate();
    }
    void sizeSet(Vector size)
    {
        _outputWindow.setPosition(Vector(20, 20));
        //int w = max(_outputWindow.right(), _gamut.right()) + 20;
        int w = _outputWindow.right() + 20;

        //_gamut.setPosition(Vector(20, _outputWindow.bottom() + 20));

        Vector vSpace(0, 15);

        _brightness.setPositionAndSize(Vector(w, 20), Vector(301, 24));
        //_autoBrightness.setPosition(_brightness.bottomLeft() + vSpace);

        _saturation.setPositionAndSize(_brightness.bottomLeft() + 2*vSpace,
            Vector(301, 24));
        //_saturation.setPositionAndSize(_autoBrightness.bottomLeft() + 2*vSpace,
        //    Vector(301, 24));
        //_autoSaturation.setPosition(_saturation.bottomLeft() + vSpace);

        _contrast.setPositionAndSize(_saturation.bottomLeft() + 2*vSpace,
            Vector(301, 24));
        //_contrast.setPositionAndSize(_autoSaturation.bottomLeft() + 2*vSpace,
        //    Vector(301, 24));
        //_autoContrastClip.setPosition(_contrast.bottomLeft() + vSpace);
        //_autoContrastMono.setPosition(_autoContrastClip.topRight() +
        //    Vector(20, 0));

        _hue.setPositionAndSize(_contrast.bottomLeft() + 2*vSpace,
            Vector(301, 24));
        //_hue.setPositionAndSize(_autoContrastClip.bottomLeft() + 2*vSpace,
        //    Vector(301, 24));

        _chromaBandwidth.setPositionAndSize(_hue.bottomLeft() + 2*vSpace,
            Vector(301, 24));
        _lumaBandwidth.setPositionAndSize(
            _chromaBandwidth.bottomLeft() + 2*vSpace, Vector(301, 24));

        _newCGA.setPosition(_lumaBandwidth.bottomLeft() + 2*vSpace);
        _ntscPrimaries.setPosition(_newCGA.topRight() + Vector(20, 0));

        //_blackText.setPosition(_newCGA.bottomLeft() + 2*vSpace);
        //_whiteText.setPosition(_blackText.bottomLeft());
        //_mostSaturatedText.setPosition(_whiteText.bottomLeft());
        //_clippedColoursText.setPosition(_mostSaturatedText.bottomLeft());

        _matchMode.setPosition(_ntscPrimaries.bottomLeft() + 2*vSpace);
        //_matchMode.setPosition(_clippedColoursText.bottomLeft() + 2*vSpace);
        _mode.setPosition(_matchMode.bottomLeft() + vSpace);
        _background.setPosition(_mode.topRight());
        _palette.setPosition(_background.topRight());
        _characterSetCombo.setPosition(_palette.topLeft());
        _scanlinesPerRow.setPosition(_palette.topRight());
        _scanlinesRepeat.setPosition(_scanlinesPerRow.topRight());

        _bwCheckBox.setPosition(_matchMode.bottomLeft() + vSpace);
        _blinkCheckBox.setPosition(_bwCheckBox.topRight());
        _phaseCheckBox.setPosition(_blinkCheckBox.topRight());
        _interlaceCombo.setPosition(_phaseCheckBox.topRight());

        _diffusionHorizontal.setPositionAndSize(
            _matchMode.bottomLeft() + 3*vSpace, Vector(301, 24));

        _diffusionVertical.setPositionAndSize(
            _diffusionHorizontal.bottomLeft() + vSpace, Vector(301, 24));

        _diffusionTemporal.setPositionAndSize(
            _diffusionVertical.bottomLeft() + vSpace, Vector(301, 24));

        _quality.setPositionAndSize(
            _diffusionTemporal.bottomLeft() + vSpace, Vector(301, 24));

        _scanlineWidth.setPositionAndSize(
            _quality.bottomLeft() + vSpace, Vector(301, 24));

        _scanlineProfile.setPosition(_scanlineWidth.bottomLeft());

        _scanlineOffset.setPositionAndSize(
            _scanlineProfile.bottomLeft() + vSpace, Vector(301, 24));

        _zoom.setPositionAndSize(
            _scanlineOffset.bottomLeft() + vSpace, Vector(301, 24));

        _scanlineBleeding.setPosition(_zoom.bottomLeft());

        _aspectRatio.setPositionAndSize(
            _scanlineBleeding.bottomLeft() + vSpace, Vector(301, 24));

        _combFilterVertical.setPosition(_aspectRatio.bottomLeft());
        _combFilterTemporal.setPosition(_combFilterVertical.topRight());
    }
    void keyboardCharacter(int character)
    {
        if (character == VK_ESCAPE)
            remove();
    }
    void setMatcher(CGAMatcher* matcher) { _matcher = matcher; }
    void setShower(CGAShower* shower) { _shower = shower; }
    void setOutput(CGAOutput* output) { _output = output; }
    void setProgram(Program* program) { _program = program; }
    void draw(Bitmap<DWORD> bitmap) { _outputWindow.draw(bitmap); }
    //void uiUpdate()
    //{
    //    _updating = true;
    //    _whiteText.setText(format("White level: %f", _white));
    //    _whiteText.size();
    //    _blackText.setText(format("Black level: %f", _black));
    //    _blackText.size();
    //    _mostSaturatedText.setText(
    //        format("Most saturated: %f", _maxSaturation));
    //    _mostSaturatedText.size();
    //    _clippedColoursText.setText(format("%i colours clipped", _clips));
    //    _clippedColoursText.size();
    //    _outputWindow.draw();
    //    _outputWindow.invalidate();
    //    _gamut.invalidate();
    //    _updating = false;
    //}
    //Colour colourFromSeq(UInt64 seq)
    //{
    //    Byte ntsc[7];
    //    int phase = (seq >> 32) & 3;
    //    for (int x = 0; x < 7; ++x) {
    //        ntsc[x] = _composite.simulateCGA(seq & 15, (seq >> 4) & 15,
    //            (x + phase) & 3);
    //        seq >>= 4;
    //    }
    //    return _decoder->decode(ntsc, phase);
    //}
    //void update()
    //{
    //    if (_updating)
    //        return;
    //    _composite.initChroma();
    //    Byte burst[4];
    //    for (int i = 0; i < 4; ++i)
    //        burst[i] = _composite.simulateCGA(6, 6, i);
    //    _decoder->calculateBurst(burst);
    //    int s[4];
    //    _composite.decode(0, s);
    //    Colour black = _decoder->decode(s);
    //    _black = 0.299*black.x + 0.587*black.y + 0.114*black.z;
    //    _composite.decode(0xffff, s);
    //    Colour white = _decoder->decode(s);
    //    _white = 0.299*white.x + 0.587*white.y + 0.114*white.z;
    //    _clips = 0;
    //    _maxSaturation = 0;
    //    _gamut.reset();
    //    for (auto i : _colours) {
    //        Colour c = colourFromSeq(i);
    //        double r = c.x;
    //        double g = c.y;
    //        double b = c.z;
    //        if (r < 0 || r >= 256 || g < 0 || g >= 256 || b < 0 || b >= 256) {
    //            ++_clips;
    //            _clipped = i;
    //        }
    //        double y = 0.299*r + 0.587*g + 0.114*b;
    //        _maxSaturation =
    //            max(_maxSaturation, (c - Colour(y, y, y)).modulus());
    //        _gamut.add(c);
    //    }
    //    _gamut.draw();
    //    _gamut.invalidate();
    //}
    void beginConvert() { _program->beginConvert(); }

    void modeSet(int value)
    {
        static const int modes[8] = {0, 1, 0x12, 2, 0x10, 0x11, 0x13, 3};
        int mode = modes[value] | 8 | (_bwCheckBox.checked() ? 4 : 0) |
            (_blinkCheckBox.checked() ? 0x20 : 0);
        _matcher->setMode(mode);
        beginConvert();
    }
    void backgroundSet(int value)
    {
        _backgroundSelected = value;
        setPaletteAndBackground();
    }
    void paletteSet(int value)
    {
        _paletteSelected = value;
        setPaletteAndBackground();
    }
    void setPaletteAndBackground()
    {
        if (_backgroundSelected == 0x10)
            _matcher->setPalette(0xff);
        else {
            _matcher->setPalette(
                _backgroundSelected + (_paletteSelected << 4));
        }
        beginConvert();
    }
    void scanlinesPerRowSet(int value)
    {
        _matcher->setScanlinesPerRow(value + 1);
        beginConvert();
    }
    void scanlinesRepeatSet(int value)
    {
        _matcher->setScanlinesRepeat(value + 1);
        beginConvert();
    }
    void diffusionHorizontalSet(double value)
    {
        _matcher->setDiffusionHorizontal(value);
        beginConvert();
    }
    void diffusionVerticalSet(double value)
    {
        _matcher->setDiffusionVertical(value);
        beginConvert();
    }
    void diffusionTemporalSet(double value)
    {
        _matcher->setDiffusionTemporal(value);
        beginConvert();
    }
    void qualitySet(double value)
    {
        _matcher->setQuality(value);
        beginConvert();
    }
    void scanlineWidthSet(double value) { _output->setScanlineWidth(value); }
    void scanlineProfileSet(int value) { _output->setScanlineProfile(value); }
    void scanlineOffsetSet(double value) { _output->setScanlineOffset(value); }
    void zoomSet(double value) { _output->setZoom(value); }
    void scanlineBleedingPressed()
    {
        _output->setScanlineBleeding(_scanlineBleeding.checked() ? 0 : 1);
    }
    void aspectRatioSet(double value) { _output->setAspectRatio(value); }
    void combFilterVerticalSet(int value)
    {
        _output->setCombFilterVertical(value);
    }
    void combFilterTemporalSet(int value)
    {
        _output->setCombFilterTemporal(value);
    }
    void bwPressed()
    {
        bool bw = _bwCheckBox.checked();
        _matcher->setMode((_matcher->getMode() & ~4) | (bw ? 4 : 0));
        _output->setBW(bw);
        //_composite.setBW(bw);
        beginConvert();
    }
    void blinkPressed()
    {
        bool blink = _blinkCheckBox.checked();
        int mode = _matcher->getMode();
        _matcher->setMode((mode & ~0x20) | (blink ? 0x20 : 0));
        if ((mode & 2) == 0)
            beginConvert();
    }
    void phasePressed()
    {
        _matcher->setPhase(_phaseCheckBox.checked() ? 0 : 1);
    }
    void interlaceSet(int value)
    {
        _matcher->setInterlace(value);
        beginConvert();
    }
    void characterSetSet(int value)
    {
        _matcher->setCharacterSet(value);
        beginConvert();
    }
    void matchModePressed()
    {
        _program->setMatchMode(_matchMode.checked());
        beginConvert();
        reCreateNTSC();
    }

    void brightnessSet(double brightness)
    {
        _output->setBrightness(brightness);
        _matcher->setBrightness(brightness);
        beginConvert();
        //if (!_updating) {
        //    update();
        //    uiUpdate();
        //}
    }
    void saturationSet(double saturation)
    {
        _output->setSaturation(saturation);
        _matcher->setSaturation(saturation);
        beginConvert();
        //if (!_updating) {
        //    update();
        //    autoContrastClip();
        //    uiUpdate();
        //}
    }
    void contrastSet(double contrast)
    {
        _output->setContrast(contrast);
        _matcher->setContrast(contrast);
        beginConvert();
        //if (!_updating) {
        //    update();
        //    autoBrightness();
        //    autoSaturation();
        //    uiUpdate();
        //}
    }
    void hueSet(double hue)
    {
        _output->setHue(hue);
        _matcher->setHue(hue);
        beginConvert();
        //if (!_updating) {
        //    update();
        //    allAutos();
        //    uiUpdate();
        //}
    }
    void chromaBandwidthSet(double chromaBandwidth)
    {
        _output->setChromaBandwidth(chromaBandwidth);
        //if (!_updating) {
        //    update();
        //    uiUpdate();
        //}
    }
    void lumaBandwidthSet(double lumaBandwidth)
    {
        _output->setLumaBandwidth(lumaBandwidth);
        //if (!_updating) {
        //    update();
        //    uiUpdate();
        //}
    }
    void newCGAPressed()
    {
        bool newCGA = _newCGA.checked();
        _output->setNewCGA(newCGA);
        _matcher->setNewCGA(newCGA);
        //_composite.setNewCGA(newCGA);
        beginConvert();
    }
    void ntscPrimariesPressed()
    {
        bool ntscPrimaries = _ntscPrimaries.checked();
        _output->setNTSCPrimaries(ntscPrimaries);
        _matcher->setNTSCPrimaries(ntscPrimaries);
        //update();
        //allAutos();
        //uiUpdate();
    }

    //void autoContrastClipPressed()
    //{
    //    _autoContrastClipFlag = _autoContrastClip.checked();
    //    if (_autoContrastClipFlag) {
    //        _autoContrastMono.uncheck();
    //        _autoContrastMonoFlag = false;
    //    }
    //    autoContrastClip();
    //    uiUpdate();
    //}
    //void autoContrastMonoPressed()
    //{
    //    _autoContrastMonoFlag = _autoContrastMono.checked();
    //    if (_autoContrastMonoFlag) {
    //        _autoContrastClip.uncheck();
    //        _autoContrastClipFlag = false;
    //    }
    //    autoContrastMono();
    //    autoBrightness();
    //    uiUpdate();
    //}
    //void autoBrightnessPressed()
    //{
    //    _autoBrightnessFlag = _autoBrightness.checked();
    //    autoContrastMono();
    //    autoBrightness();
    //    uiUpdate();
    //}
    //void autoSaturationPressed()
    //{
    //    _autoSaturationFlag = _autoSaturation.checked();
    //    autoSaturation();
    //    uiUpdate();
    //}
    void reCreateNTSC()
    {
        _output->reCreateNTSC();
        //update();
        //allAutos();
        //uiUpdate();
    }

    //void allAutos()
    //{
    //    autoSaturation();
    //    autoContrastClip();
    //    autoContrastMono();
    //    autoBrightness();
    //}
    //void autoBrightness(bool force = false)
    //{
    //    if (!false && !_autoBrightnessFlag)
    //        return;
    //    _brightness.setValue(_brightness.getValue() +
    //        (256 - (_black + _white))/512);
    //    update();
    //}
    //void autoSaturation()
    //{
    //    if (!_autoSaturationFlag)
    //        return;
    //    _saturation.setValue(_output->getSaturation()*sqrt(3.0)*
    //        (_white - _black)/(2*_maxSaturation));
    //    update();
    //}
    //void autoContrastClip()
    //{
    //    if (!_autoContrastClipFlag)
    //        return;
    //    double minContrast = 0;
    //    double maxContrast = 2;
    //    double contrast;
    //    do {
    //        contrast = (maxContrast + minContrast)/2;
    //        setContrast(contrast);
    //        update();
    //        autoBrightness();
    //        if (_clips == 1 || (maxContrast - minContrast) < 0.000001)
    //            break;
    //        else
    //            if (_clips == 0)
    //                minContrast = contrast;
    //            else
    //                maxContrast = contrast;
    //    } while (true);
    //    double midPoint = (_white + _black)/2;
    //    double fudge = 0.99999;
    //    for (int i = 0; i < 3; ++i) {
    //        Colour c = colourFromSeq(_clipped);
    //        double r = c.x;
    //        double g = c.y;
    //        double b = c.z;
    //        bool found = false;
    //        if (r < 0) {
    //            contrast *= fudge*midPoint/(midPoint - r);
    //            found = true;
    //        }
    //        if (!found && r >= 256) {
    //            contrast *= fudge*midPoint/(r - midPoint);
    //            found = true;
    //        }
    //        if (!found && g < 0) {
    //            contrast *= fudge*midPoint/(midPoint - g);
    //            found = true;
    //        }
    //        if (!found && g >= 256) {
    //            contrast *= fudge*midPoint/(g - midPoint);
    //            found = true;
    //        }
    //        if (!found && b < 0) {
    //            contrast *= fudge*midPoint/(midPoint - b);
    //            found = true;
    //        }
    //        if (!found && b >= 256)
    //            contrast *= fudge*midPoint/(b - midPoint);
    //        setContrast(contrast);
    //        update();
    //        autoBrightness();
    //        autoSaturation();
    //        if (_clips == 0)
    //            break;
    //    }
    //}
    //void autoContrastMono()
    //{
    //    if (!_autoContrastMonoFlag)
    //        return;
    //    _contrast.setValue(_output->getContrast() * 256/(_white - _black));
    //    update();
    //}
    //void resetColours() { _colours = Set<UInt64>(); }
    //void addColour(UInt64 seq) { _colours.add(seq); }

private:
    template<class T> void add2(T* p)
    {
        add(p);
        p->setHost(this);
    }

    //AnimatedWindow _animated;
    OutputWindow _outputWindow;
    BrightnessSliderWindow _brightness;
    //AutoBrightnessButtonWindow _autoBrightness;
    SaturationSliderWindow _saturation;
    //AutoSaturationButtonWindow _autoSaturation;
    ContrastSliderWindow _contrast;
    //AutoContrastClipButtonWindow _autoContrastClip;
    //AutoContrastMonoButtonWindow _autoContrastMono;
    HueSliderWindow _hue;
    ChromaBandwidthSliderWindow _chromaBandwidth;
    LumaBandwidthSliderWindow _lumaBandwidth;
    //TextWindow _blackText;
    //TextWindow _whiteText;
    //TextWindow _mostSaturatedText;
    //TextWindow _clippedColoursText;
    //GamutWindow _gamut;
    NewCGAButtonWindow _newCGA;
    NTSCPrimariesButtonWindow _ntscPrimaries;
    MatchModeButton _matchMode;
    ModeCombo _mode;
    BackgroundCombo _background;
    PaletteCombo _palette;
    ScanlinesPerRowCombo _scanlinesPerRow;
    ScanlinesRepeatCombo _scanlinesRepeat;
    DiffusionHorizontalSliderWindow _diffusionHorizontal;
    DiffusionVerticalSliderWindow _diffusionVertical;
    DiffusionTemporalSliderWindow _diffusionTemporal;
    QualitySliderWindow _quality;
    BWCheckBox _bwCheckBox;
    BlinkCheckBox _blinkCheckBox;
    PhaseCheckBox _phaseCheckBox;
    InterlaceCombo _interlaceCombo;
    CharacterSetCombo _characterSetCombo;
    ScanlineWidthSliderWindow _scanlineWidth;
    ScanlineProfileCombo _scanlineProfile;
    ScanlineOffsetSliderWindow _scanlineOffset;
    ZoomSliderWindow _zoom;
    ScanlineBleedingCheckBox _scanlineBleeding;
    AspectRatioSliderWindow _aspectRatio;
    CombFilterVerticalCombo _combFilterVertical;
    CombFilterTemporalCombo _combFilterTemporal;
    CGAMatcher* _matcher;
    CGAShower* _shower;
    CGAOutput* _output;
    Program* _program;
    //CGAComposite _composite;
    //double _black;
    //double _white;
    //Set<UInt64> _colours;
    //int _clips;
    //double _maxSaturation;
    //UInt64 _clipped;
    //bool _autoBrightnessFlag;
    //bool _autoSaturationFlag;
    //bool _autoContrastClipFlag;
    //bool _autoContrastMonoFlag;
    //bool _updating;
    int _paletteSelected;
    int _backgroundSelected;
};

class BitmapValue : public Structure
{
public:
    void load(String filename)
    {
        _name = filename;
        // We parse the filename relative to the current directory here instead
        // of relative to the config file path because the filename usually
        // comes from the command line.
        _bitmap = PNGFileFormat<SRGB>().load(File(filename, true));
        Vector size = _bitmap.size();
        _size.set("x", size.x, Span());
        _size.set("y", size.y, Span());
    }
    Bitmap<SRGB> bitmap() { return _bitmap; }
    Value getValue(Identifier identifier) const
    {
        if (identifier == Identifier("size"))
            return Value(VectorType(), &_size, Span());
        return Structure::getValue(identifier);
    }
    String name() { return _name; }
    bool operator==(const BitmapValue& other) const
    {
        return _name == other._name;
    }
private:
    Structure _size;
    String _name;
    Bitmap<SRGB> _bitmap;
};

class BitmapType : public StructuredType
{
public:
    BitmapType(BitmapValue* bitmapValue)
      : StructuredType(create<Body>(bitmapValue)) { }
    static String name() { return "Bitmap"; }
    class Body : public StructuredType::Body
    {
    public:
        Body(BitmapValue* bitmapValue)
          : StructuredType::Body("Bitmap", members()),
            _bitmapValue(bitmapValue)
        { }
        List<StructuredType::Member> members()
        {
            List<StructuredType::Member> vectorMembers;
            vectorMembers.add(StructuredType::member<Vector>("size"));
            return vectorMembers;
        }
        bool canConvertFrom(const Type& from, String* reason) const
        {
            return from == StringType();
        }
        Value convert(const Value& value) const
        {
            _bitmapValue->load(value.value<String>());
            return Value(type(), static_cast<Structure*>(_bitmapValue),
                value.span());
        }
        Value defaultValue() const
        {
            return Value(type(), static_cast<Structure*>(_bitmapValue),
                Span());
        }
    private:
        BitmapValue* _bitmapValue;
    };
};

class BitmapIsRGBIFunction : public Function
{
public:
    BitmapIsRGBIFunction(BitmapType bitmapType)
      : Function(create<Body>(bitmapType)) { }
    class Body : public Function::Body
    {
    public:
        Body(BitmapType bitmapType) : _bitmapType(bitmapType) { }
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto bitmap = static_cast<BitmapValue*>(
                arguments.begin()->value<Structure*>())->bitmap();
            Vector size = bitmap.size();

            int maxDistance = 0;
            const Byte* inputRow = bitmap.data();

            for (int y = 0; y < size.y; ++y) {
                const SRGB* inputPixel =
                    reinterpret_cast<const SRGB*>(inputRow);
                for (int x = 0; x < size.x; ++x) {
                    SRGB s = *inputPixel;
                    ++inputPixel;
                    int bestDistance = 0x7fffffff;
                    Byte bestRGBI = 0;
                    for (int i = 0; i < 16; ++i) {
                        int distance = (Vector3Cast<int>(rgbiPalette[i]) -
                            Vector3Cast<int>(s)).modulus2();
                        if (distance < bestDistance) {
                            bestDistance = distance;
                            if (distance < 42*42)
                                break;
                        }
                    }
                    maxDistance = max(bestDistance, maxDistance);
                }
                inputRow += bitmap.stride();
            }

            return Value(maxDistance < 15*15*3);
        }
        Identifier identifier() const { return "bitmapIsRGBI"; }
        FunctionType type() const
        {
            return FunctionType(BooleanType(), _bitmapType);
        }
    private:
        BitmapType _bitmapType;
    };
};

class Program : public WindowProgram<CGA2NTSCWindow>
{
public:
    void run()
    {
        BitmapValue bitmapValue;
        BitmapType bitmapType(&bitmapValue);

        ConfigFile configFile;
        configFile.addOption("inputPicture", bitmapType);
        configFile.addDefaultOption("mode", 0x1a);
        configFile.addDefaultOption("palette", 0x0f);
        configFile.addDefaultOption("interlaceMode", 0);
        configFile.addDefaultOption("scanlinesPerRow", 2);
        configFile.addDefaultOption("scanlinesRepeat", 1);
        configFile.addDefaultOption("matchMode", true);
        configFile.addDefaultOption("contrast", 1.0);
        configFile.addDefaultOption("brightness", 0.0);
        configFile.addDefaultOption("saturation", 1.0);
        configFile.addDefaultOption("hue", 0.0);
        configFile.addDefaultOption("chromaBandwidth", 1.0);
        configFile.addDefaultOption("lumaBandwidth", 1.0);
        configFile.addDefaultOption("ntscPrimaries", false);
        configFile.addDefaultOption("horizontalDiffusion", 0.5);
        configFile.addDefaultOption("verticalDiffusion", 0.5);
        configFile.addDefaultOption("temporalDiffusion", 0.0);
        configFile.addDefaultOption("quality", 0.5);
        configFile.addDefaultOption("newCGA", false);
        configFile.addDefaultOption("characterSet", 3);
        configFile.addDefaultOption("cgaROM", String("5788005.u33"));
        configFile.addDefaultOption("aspectRatio", 5.0/6.0);
        configFile.addDefaultOption("scanlineWidth", 0.5);
        configFile.addDefaultOption("scanlineProfile", 0);
        configFile.addDefaultOption("scanlineBleeding", false);
        configFile.addDefaultOption("scanlineOffset", 0.0);
        configFile.addDefaultOption("zoom", 2.0);
        configFile.addDefaultOption("phase", 1);
        configFile.addDefaultOption("interactive", true);
        configFile.addDefaultOption("combFilterVertical", 0);
        configFile.addDefaultOption("combFilterTemporal", 0);
        configFile.addDefaultOption("fftWisdom", String("wisdom"));
        configFile.addDefaultOption("doubleWidth", false);
        configFile.addDefaultOption("doubleHeight", false);

        configFile.addFunco(BitmapIsRGBIFunction(bitmapType));

        List<Value> arguments;

        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] +
                " <input file name>(.png|.config)\n");
            return;
        }
        String configPath = _arguments[1];
        int n = configPath.length();
        bool isPng = false;
        if (n > 4) {
            auto p = &configPath[n - 4];
            if (p[0] == '.' && (p[1] == 'p' || p[1] == 'P') &&
                (p[2] == 'n' || p[2] == 'N') && (p[3] == 'g' || p[3] == 'G'))
                isPng = true;
        }
        if (isPng) {
            configPath = "default.config";
            arguments.add(_arguments[0] + " " + configPath);
            for (int i = 1; i < _arguments.count(); ++i)
                arguments.add(_arguments[i]);
        }
        else {
            arguments.add(_arguments[0] + " " + configPath);
            for (int i = 1; i < _arguments.count() - 1; ++i)
                arguments.add(_arguments[i + 1]);
        }

        configFile.addDefaultOption("arguments",
            ArrayType(StringType(), IntegerType()), arguments);

        File config(configPath, true);
        configFile.load(config);

        FFTWWisdom<float> wisdom(
            File(configFile.get<String>("fftWisdom"), config.parent()));

        _matcher.setProgram(this);
        _window.setMatcher(&_matcher);
        _window.setShower(&_shower);
        _window.setOutput(&_output);
        _window.setProgram(this);

        _matcher.setDiffusionHorizontal(
            configFile.get<double>("horizontalDiffusion"));
        _matcher.setDiffusionVertical(
            configFile.get<double>("verticalDiffusion"));
        _matcher.setDiffusionTemporal(
            configFile.get<double>("temporalDiffusion"));
        _matcher.setQuality(configFile.get<double>("quality"));
        _matcher.setInterlace(configFile.get<int>("interlaceMode"));
        _matcher.setCharacterSet(configFile.get<int>("characterSet"));
        _matcher.setMode(configFile.get<int>("mode"));
        _matcher.setPalette(configFile.get<int>("palette"));
        _matcher.setScanlinesPerRow(configFile.get<int>("scanlinesPerRow"));
        _matcher.setScanlinesRepeat(configFile.get<int>("scanlinesRepeat"));
        _matcher.setROM(
            File(configFile.get<String>("cgaROM"), config.parent()));

        bool ntscPrimaries = configFile.get<bool>("ntscPrimaries");
        _output.setNTSCPrimaries(ntscPrimaries);
        _matcher.setNTSCPrimaries(ntscPrimaries);
        double brightness = configFile.get<double>("brightness");
        _output.setBrightness(brightness);
        _matcher.setBrightness(brightness);
        double saturation = configFile.get<double>("saturation");
        _output.setSaturation(saturation);
        _matcher.setSaturation(saturation);
        double hue = configFile.get<double>("hue");
        _output.setHue(hue);
        _matcher.setHue(hue);
        double contrast = configFile.get<double>("contrast");
        _output.setContrast(contrast);
        _matcher.setContrast(contrast);
        _output.setChromaBandwidth(configFile.get<double>("chromaBandwidth"));
        _output.setLumaBandwidth(configFile.get<double>("lumaBandwidth"));
        bool newCGA = configFile.get<bool>("newCGA");
        _output.setNewCGA(newCGA);
        _matcher.setNewCGA(newCGA);
        _output.setScanlineWidth(configFile.get<double>("scanlineWidth"));
        _output.setScanlineProfile(configFile.get<int>("scanlineProfile"));
        _output.setScanlineOffset(configFile.get<double>("scanlineOffset"));
        _output.setZoom(configFile.get<double>("zoom"));
        _output.setScanlineBleeding(configFile.get<bool>("scanlineBleeding"));
        _output.setAspectRatio(configFile.get<double>("aspectRatio"));
        _output.setCombFilterVertical(
            configFile.get<int>("combFilterVertical"));
        _output.setCombFilterTemporal(
            configFile.get<int>("combFilterTemporal"));

        Bitmap<SRGB> input = bitmapValue.bitmap();
        Bitmap<SRGB> input2 = input;
        Vector size = input.size();
        if (!configFile.get<bool>("doubleHeight")) {
            // Vertically shrink tall images by a factor of two, to handle the
            // normal case of a DOSBox screenshot.
            input2 = Bitmap<SRGB>(Vector(size.x, size.y/2));
            const Byte* inputRow = input.data();
            Byte* outputRow = input2.data();
            for (int y = 0; y < size.y/2; ++y) {
                const SRGB* inputPixelTop =
                    reinterpret_cast<const SRGB*>(inputRow);
                const SRGB* inputPixelBottom =
                    reinterpret_cast<const SRGB*>(inputRow +input.stride());
                SRGB* outputPixel = reinterpret_cast<SRGB*>(outputRow);
                for (int x = 0; x < size.x; ++x) {
                    Vector3<int> top = Vector3Cast<int>(*inputPixelTop);
                    ++inputPixelTop;
                    Vector3<int> bottom = Vector3Cast<int>(*inputPixelBottom);
                    ++inputPixelBottom;
                    *outputPixel = Vector3Cast<UInt8>((top + bottom)/2);
                    ++outputPixel;
                }
                inputRow += input.stride()*2;
                outputRow += input2.stride();
            }
            input = input2;
            size = input.size();
        }
        if (configFile.get<bool>("doubleWidth")) {
            // Image is most likely 2bpp or LRES text mode with 1 pixel per
            // ldot. Rescale it to 1 pixel per hdot.
            input2 = Bitmap<SRGB>(Vector(size.x*2, size.y));
            const Byte* inputRow = input.data();
            Byte* outputRow = input2.data();
            for (int y = 0; y < size.y; ++y) {
                SRGB* outputPixel = reinterpret_cast<SRGB*>(outputRow);
                const SRGB* inputPixel =
                    reinterpret_cast<const SRGB*>(inputRow);
                for (int x = 0; x < size.x; ++x) {
                    SRGB s = *inputPixel;
                    ++inputPixel;
                    *outputPixel = s;
                    ++outputPixel;
                    *outputPixel = s;
                    ++outputPixel;
                }
                inputRow += input.stride();
                outputRow += input2.stride();
            }
            input = input2;
            size = input.size();
        }

        _matcher.setInput(input);
        _shower.setInput(input);
        setMatchMode(configFile.get<bool>("matchMode"));

        Timer timer;

        beginConvert();

        bool interactive = configFile.get<bool>("interactive");
        if (interactive) {
            _output.setWindow(&_window);
            WindowProgram::run();
        }

        if (!_matchMode)
            _matcher.cancel();
        _matcher.join();
        _output.reCreateNTSC();
        _output.join();

        if (!interactive)
            timer.output("Elapsed time");

        String inputFileName = bitmapValue.name();
        int i;
        for (i = inputFileName.length() - 1; i >= 0; --i)
            if (inputFileName[i] == '.')
                break;
        if (i != -1)
            inputFileName = inputFileName.subString(0, i);

        _output.save(inputFileName + "_out.png");
        _matcher.save(inputFileName + "_out.dat");
        _matcher.saveRGBI(inputFileName + "_out.rgbi");
        _matcher.savePalettes(inputFileName + "_out.palettes");
    }
    bool getMatchMode() { return _matchMode; }
    void setMatchMode(bool matchMode)
    {
        _matchMode = matchMode;
        if (!matchMode)
            _output.setRGBI(_shower.getOutput());
        else
            _output.setRGBI(_matcher.getOutput());
    }
    void updateOutput()
    {
        _updateNeeded = true;
        _interruptMessageLoop.signal();
    }
    bool idle()
    {
        if (_updateNeeded) {
            _updateNeeded = false;
            _window.reCreateNTSC();
        }
        return false;
    }
    void beginConvert()
    {
        if (!_matchMode)
            _shower.convert();
        else
            _matcher.restart();
    }
private:
    CGAMatcher _matcher;
    CGAShower _shower;
    CGAOutput _output;
    bool _matchMode;
    bool _updateNeeded;
};
