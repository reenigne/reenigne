#include "alfe/main.h"
#include "alfe/bitmap_png.h"
#include "alfe/complex.h"
#include "alfe/space.h"
#include "alfe/set.h"
#include "alfe/config_file.h"
#include "alfe/cga.h"
#include "alfe/timer.h"
#include "alfe/ntsc_decode.h"
#include "alfe/knob.h"
#include "alfe/scanlines.h"
#include "alfe/image_filter.h"

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
                        reinterpret_cast<int*>(errorRow2 + _error.stride())[
                            x + xx] += (error*_diffusionVertical/256);
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

template<class T> class CGAOutputT : public ThreadTask
{
public:
    CGAOutputT() : _window(0), _zoom(0), _aspectRatio(1) { }
    void setRGBI(Bitmap<Byte> rgbi)
    {
        _rgbi = rgbi;
        {
            Lock lock(&_mutex);
            _ntsc = Bitmap<Byte>(_rgbi.size() - Vector(1, 0));
            allocateBitmap2();
        }
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
        Bitmap<Byte> ntsc;
        Bitmap<DWORD> decoded;
        Bitmap<DWORD> bitmap;
        {
            Lock lock(&_mutex);
            ntsc = _ntsc;
            bitmap = _bitmap;
            decoded = _decoded;
        }
        const Byte* ntscRow = ntsc.data();
        Byte* decodedRow = decoded.data();
        Vector size = requiredSize();
        if (ntsc.size().x <= 0 || ntsc.size().y <= 0 || size.x <= 0 ||
            size.y <= 0)
            return;
        for (int y = 0; y < ntsc.size().y; ++y) {
            _decoder.decodeLine(ntscRow, reinterpret_cast<DWORD*>(decodedRow),
                ntsc.size().x - 6, size.x);
            decodedRow += decoded.stride();
            ntscRow += ntsc.stride();
        }

        decodedRow = decoded.data();
        Byte* outputRow = bitmap.data();
        _renderer.setOffset(0);
        _renderer.init(ntsc.size().y, size.y);
        for (int x = 0; x < decoded.size().x; ++x) {
            _renderer.renderColumn(decodedRow, decoded.stride(), outputRow,
                bitmap.stride());
            decodedRow += sizeof(DWORD);
            outputRow += sizeof(DWORD);
        }
        if (_window != 0)
            _window->draw(bitmap);
    }
    void init()
    {
        _scaler.init();
        _decoder.init();
    }
    void run2()
    {
        if (_connector == 0) {
            // Convert from RGBI to 9.7 fixed-point sRGB
            UInt16 levels[4];
            for (int i = 0; i < 4; ++i) {
                levels[i] = static_cast<int>(clamp(0.0,
                    (getBrightness() + 85*i*getContrast())*128, 32767.0));
            }
            static const int palette[3*16] = {
                0, 0, 0,  0, 0, 2,  0, 2, 0,  0, 2, 2,
                2, 0, 0,  2, 0, 2,  2, 1, 0,  2, 2, 2,
                1, 1, 1,  1, 1, 3,  1, 3, 1,  1, 3, 3,
                3, 1, 1,  3, 1, 3,  3, 3, 1,  3, 3, 3};
            static const UInt16 srgbPalette[3*16];
            for (int i = 0; i < 3*16; ++i)
                srgbPalette[i] = levels[palette[i]];
            const Byte* rgbiRow = _rgbi.data();
            Byte* srgbRow = _srgb.data();
            for (int y = 0; y < _rgbi.size().y; ++y) {
                const Byte* rgbi = rgbiRow;
                UInt16* srgb = reinterpret_cast<UInt16*>(srgbRow);
                for (int x = 0; x < _rgbi.size().x; ++x) {
                    Byte v = *rgbi;
                    ++rgbi;
                    UInt16* p = &palette[3*v];
                    srgb[0] = p[0];
                    srgb[1] = p[1];
                    srgb[2] = p[2];
                }
                rgbiRow += _rgbi.stride();
                srgbRow += _srgb.stride();
            }
        }
        else {
            // Convert from RGBI to composite
            const Byte* rgbiRow = _rgbi.data();
            Byte* ntscRow = _ntsc.data();
            for (int y = 0; y < _ntsc.size().y; ++y) {
                const Byte* rgbi = rgbiRow;
                Byte* ntsc = ntscRow;
                for (int x = 0; x < _ntsc.size().x; ++x) {
                    *ntsc =
                        _composite.simulateCGA(*rgbi, rgbi[1], (x + 1) & 3);
                    ++rgbi;
                    ++ntsc;
                }
                rgbiRow += _rgbi.stride();
                ntscRow += _ntsc.stride();
            }
            // Apply comb filter and expand to 8 channels of 16 bits per
            // channel.
            ntscRow = _ntsc.data();
            UInt16* combedRow = _combed.data();
            for (int y = 0; y < _combedSize.y; ++y) {
                const Byte* ntsc = ntscRow;
                UInt16* combed = combedRow;
                for (int x = 0; x < _combedSize.x; ++x) {
                    switch ()
                }
                ntscRow += _ntsc.stride();
                combedRow += _combed.stride();
            }

            // Decode 128 bit composite to 9.7 fixed-point sRGB
            _decoder.decode();
        }
        // Shift, clip, show clipping and linearization
        _scaler.render();
        // Delinearization and float-to-byte conversion
    }

    void save(String outputFileName)
    {
        _bitmap.subBitmap(Vector(0, 0), requiredSize()).
            save(PNGFileFormat<DWORD>(), File(outputFileName, true));

        FileStream s = File(outputFileName + ".ntsc", true).openWrite();
        s.write(_ntsc.data(), _ntsc.stride()*_ntsc.size().y);
    }

    void setConnector(int connector)
    {
        _connector = connector;
        _composite.setNewCGA(connector == 2);
        reCreateNTSC();
    }
    int getConnector() { return _connector; }
    void setBW(bool bw) { _composite.setBW(bw); reCreateNTSC(); }
    void setScanlineProfile(int profile)
    {
        _renderer.setProfile(profile);
        restart();
    }
    int getScanlineProfile() { return _renderer.getProfile(); }
    void setScanlineWidth(double width)
    {
        _renderer.setWidth(static_cast<float>(width));
        restart();
    }
    double getScanlineWidth() { return _renderer.getWidth(); }
    void setScanlineBleeding(bool bleeding)
    {
        _renderer.setBleeding(bleeding);
        restart();
    }
    bool getScanlineBleeding() { return _renderer.getBleeding(); }
    void setZoom(double zoom) { _zoom = zoom; allocateBitmap(); }
    double getZoom() { return _zoom; }
    void setAspectRatio(double ratio)
    {
        _aspectRatio = ratio;
        allocateBitmap();
    }
    double getAspectRatio() { return _aspectRatio; }
    void setCombFilter(int value) { _combFilter = value; restart(); }
    int getCombFilter() { return _combFilter; }
    ResamplingNTSCDecoder* getDecoder() { return &_decoder; }

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
    bool getShowClipping() { return _decoder.getShowClipping(); }
    void setShowClipping(bool showClipping)
    {
        _decoder.setShowClipping(showClipping);
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

    Vector requiredSize()
    {
        double y = _zoom*_ntsc.size().y;
        double x = _zoom*(_ntsc.size().x - 6)*_aspectRatio/2;
        return Vector(static_cast<int>(x), static_cast<int>(y));
    }
    void setWindow(CGA2NTSCWindow* window) { _window = window; }
private:
    void allocateBitmap()
    {
        Lock lock(&_mutex);
        allocateBitmap2();
    }
    void allocateBitmap2()
    {
        Vector size = requiredSize();
        if (size.x > _bitmap.size().x || size.y > _bitmap.size().y)
            _bitmap = Bitmap<DWORD>(size);
        if (size.x > _decoded.size().x)
            _decoded = Bitmap<DWORD>(Vector(size.x, _ntsc.size().y));
        restart();
    }

    Bitmap<DWORD> _bitmap;
    Bitmap<Byte> _rgbi;
    Bitmap<Byte> _ntsc;
    Bitmap<DWORD> _decoded;
    CGAComposite _composite;
    ResamplingNTSCDecoder _decoder;
    ScanlineRenderer _renderer;
    double _scanlineWidth;
    int _scanlineProfile;
    double _zoom;
    bool _scanlineBleeding;
    double _aspectRatio;
    int _combFilter;
    int _connector;
    CGA2NTSCWindow* _window;
    Mutex _mutex;

    FIRScanlineRenderer _scaler;
    Vector _combedSize;
    AlignedBuffer _combed;
    AlignedBuffer _srgb;
    AlignedBuffer _unscaled;
    AlignedBuffer _scaled;
};

typedef CGAOutputT<void> CGAOutput;

template<class T> class CGA2NTSCWindowT : public RootWindow
{
public:
    CGA2NTSCWindowT() : _monitor(this), _videoCard(this)
    {
        setText("CGA to NTSC");
        add(&_outputWindow);
        add(&_monitor);
        add(&_videoCard);
        add(&_knobSliders);
    }
    void create()
    {
        _monitor._connector.set(_output->getConnector());
        _monitor._colour._brightness.setValue(_output->getBrightness());
        _monitor._colour._saturation.setValue(_output->getSaturation());
        _monitor._colour._contrast.setValue(_output->getContrast());
        _monitor._colour._hue.setValue(_output->getHue());
        _monitor._colour._showClipping.setCheckState(
            _output->getShowClipping());
        _monitor._filter._chromaBandwidth.setValue(
            _output->getChromaBandwidth());
        _monitor._filter._lumaBandwidth.setValue(_output->getLumaBandwidth());
        _monitor._filter._combFilter.set(_output->getCombFilter());
        _monitor._scanlines._profile.set(_output->getScanlineProfile());
        _monitor._scanlines._width.setValue(_output->getScanlineWidth());
        _monitor._scanlines._bleeding.setCheckState(
            _output->getScanlineBleeding());
        _monitor._scaling._zoom.setValue(_output->getZoom());
        _monitor._scaling._aspectRatio.setValue(_output->getAspectRatio());
        int mode = _matcher->getMode();
        int m;
        if ((mode & 0x80) != 0)
            m = 8 + (mode & 1);
        else {
            switch (mode & 0x13) {
                case 0: m = 0; break;
                case 1: m = 1; break;
                case 2: m = 3; break;
                case 3: m = 7; break;
                case 0x10: m = 4; break;
                case 0x11: m = 5; break;
                case 0x12: m = 2; break;
                case 0x13: m = 6; break;
            }
        }
        _videoCard._registers._mode.set(m);
        _videoCard._registers._bw.setCheckState((mode & 4) != 0);
        _videoCard._registers._blink.setCheckState((mode & 0x20) != 0);
        int palette = _matcher->getPalette();
        if (palette == 0xff) {
            _paletteSelected = 0;
            _backgroundSelected = 0x10;
        }
        else {
            _paletteSelected = (palette >> 4) & 3;
            _backgroundSelected = palette & 0xf;
        }
        _videoCard._registers._palette.set(_paletteSelected);
        _videoCard._registers._background.set(_backgroundSelected);
        _videoCard._registers._scanlinesPerRow.set(
            _matcher->getScanlinesPerRow() - 1);
        _videoCard._registers._scanlinesRepeat.set(
            _matcher->getScanlinesRepeat() - 1);
        _videoCard._registers._phase.setCheckState(_matcher->getPhase() == 0);
        _videoCard._registers._interlace.set(_matcher->getInterlace());
        _videoCard._matching._matchMode.setCheckState(
            _program->getMatchMode());
        _videoCard._matching._diffusionHorizontal.setValue(
            _matcher->getDiffusionHorizontal());
        _videoCard._matching._diffusionVertical.setValue(
            _matcher->getDiffusionVertical());
        _videoCard._matching._diffusionTemporal.setValue(
            _matcher->getDiffusionTemporal());
        _videoCard._matching._quality.setValue(_matcher->getQuality());
        _videoCard._matching._characterSet.set(_matcher->getCharacterSet());
        setInnerSize(Vector(0, 0));
        _outputWindow.setInnerSize(_output->requiredSize());
        RootWindow::create();
        updateApplicableControls();
    }
    void innerSizeSet(Vector size)
    {
        if (size.x <= 0 || size.y <= 0)
            return;
        RootWindow::innerSizeSet(size);
        int owx = max(_videoCard.outerSize().x,
            size.x - (_monitor.outerSize().x + 3*pad().x));
        int owy = max(0, size.y - (_videoCard.outerSize().y + 3*pad().y));
        _outputWindow.setInnerSize(Vector(owx, owy));
        layout();
        invalidate();
    }
    void layout()
    {
        Vector pad(20, 20);
        _outputWindow.setTopLeft(pad);
        int r = _outputWindow.right();
        _videoCard.setTopLeft(_outputWindow.bottomLeft() + Vector(0, pad.y));
        r = max(r, _videoCard.right());
        _monitor.setTopLeft(_outputWindow.topRight() + Vector(pad.x, 0));
        setInnerSize(pad + Vector(_monitor.right(),
            max(_videoCard.bottom(), _monitor.bottom())));
    }
    void keyboardCharacter(int character)
    {
        if (character == VK_ESCAPE)
            remove();
    }
    void setConfig(ConfigFile* config)
    {
        _monitor._colour._brightness.setConfig(config);
        _monitor._colour._saturation.setConfig(config);
        _monitor._colour._contrast.setConfig(config);
        _monitor._colour._hue.setConfig(config);
        _monitor._filter._chromaBandwidth.setConfig(config);
        _monitor._filter._lumaBandwidth.setConfig(config);
        _monitor._scanlines._width.setConfig(config);
        _monitor._scaling._zoom.setConfig(config);
        _monitor._scaling._aspectRatio.setConfig(config);
        _videoCard._matching._diffusionHorizontal.setConfig(config);
        _videoCard._matching._diffusionVertical.setConfig(config);
        _videoCard._matching._diffusionTemporal.setConfig(config);
        _videoCard._matching._quality.setConfig(config);
    }
    void setMatcher(CGAMatcher* matcher) { _matcher = matcher; }
    void setShower(CGAShower* shower) { _shower = shower; }
    void setOutput(CGAOutput* output) { _output = output; }
    void setProgram(Program* program) { _program = program; }
    void draw(Bitmap<DWORD> bitmap) { _outputWindow.draw(bitmap); }
    void beginConvert() { _program->beginConvert(); }

    void updateApplicableControls()
    {
        bool matchMode = _program->getMatchMode();
        int mode = _matcher->getMode();
        int scanlinesPerRow = _matcher->getScanlinesPerRow();
        _videoCard._registers._blink.enableWindow((mode & 2) == 0);
        _videoCard._registers._palette.enableWindow((mode & 0x12) == 2);
        _videoCard._registers._phase.enableWindow((mode & 1) == 1);
        _videoCard._matching._quality.enableWindow(matchMode &&
            (((mode & 3) != 2 || scanlinesPerRow > 2)));
        _videoCard._matching._characterSet.enableWindow(matchMode &&
            (mode & 2) == 0);
        _videoCard._matching._diffusionHorizontal.enableWindow(matchMode);
        _videoCard._matching._diffusionVertical.enableWindow(matchMode);
        _videoCard._matching._diffusionTemporal.enableWindow(matchMode);
    }
    void modeSet(int value)
    {
        static const int modes[8] = {0, 1, 0x12, 2, 0x10, 0x11, 0x13, 3};
        int mode = modes[value] | 8 |
            (_videoCard._registers._bw.checked() ? 4 : 0) |
            (_videoCard._registers._blink.checked() ? 0x20 : 0);
        _matcher->setMode(mode);
        updateApplicableControls();
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
        updateApplicableControls();
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
    void zoomSet(double value) { _output->setZoom(value); }
    void scanlineBleedingSet(bool value)
    {
        _output->setScanlineBleeding(value);
    }
    void aspectRatioSet(double value) { _output->setAspectRatio(value); }
    void combFilterSet(int value) { _output->setCombFilter(value); }
    void bwSet(bool value)
    {
        _matcher->setMode((_matcher->getMode() & ~4) | (value ? 4 : 0));
        _output->setBW(value);
        //_composite.setBW(value);
        beginConvert();
    }
    void blinkSet(bool value)
    {
        int mode = _matcher->getMode();
        _matcher->setMode((mode & ~0x20) | (value ? 0x20 : 0));
        if ((mode & 2) == 0)
            beginConvert();
    }
    void phaseSet(bool value) { _matcher->setPhase(value ? 0 : 1); }
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
    void matchModeSet(bool value)
    {
        _program->setMatchMode(value);
        updateApplicableControls();
        beginConvert();
        reCreateNTSC();
    }

    void brightnessSet(double brightness)
    {
        _output->setBrightness(brightness);
        _matcher->setBrightness(brightness);
        beginConvert();
    }
    void saturationSet(double saturation)
    {
        _output->setSaturation(saturation);
        _matcher->setSaturation(saturation);
        beginConvert();
    }
    void contrastSet(double contrast)
    {
        _output->setContrast(contrast);
        _matcher->setContrast(contrast);
        beginConvert();
    }
    void hueSet(double hue)
    {
        _output->setHue(hue);
        _matcher->setHue(hue);
        beginConvert();
    }
    void showClippingSet(bool showClipping)
    {
        _output->setShowClipping(showClipping);
        _output->restart();
    }
    void chromaBandwidthSet(double chromaBandwidth)
    {
        _output->setChromaBandwidth(chromaBandwidth);
    }
    void lumaBandwidthSet(double lumaBandwidth)
    {
        _output->setLumaBandwidth(lumaBandwidth);
    }
    void connectorSet(int connector)
    {
        _output->setConnector(connector);
        _matcher->setNewCGA(connector == 2);
        bool composite = (connector != 0);
        _monitor._colour._saturation.enableWindow(composite);
        _monitor._colour._hue.enableWindow(composite);
        _monitor._filter.enableWindow(composite);
        beginConvert();
    }

    void reCreateNTSC()
    {
        _output->reCreateNTSC();
    }

private:
    Vector vSpace() { return Vector(0, 15); }
    Vector hSpace() { return Vector(15, 0); }
    Vector groupTL() { return Vector(15, 20); }
    Vector groupBR() { return Vector(15, 15); }
    Vector pad() { return Vector(20, 20); }
    Vector groupVSpace() { return Vector(0, 10); }

    class OutputWindow : public BitmapWindow
    {
    public:
        void draw(Bitmap<DWORD> bitmap)
        {
            _b = bitmap;
            BitmapWindow::draw();
        }
        void draw2()
        {
            // Just copy from the top left corner for now.
            Vector zero(0, 0);
            Vector size(min(_b.size().x, innerSize().x),
                min(_b.size().y, innerSize().y));
            _b.subBitmap(zero, size).copyTo(_bitmap.subBitmap(zero, size));
        }
    private:
        Bitmap<DWORD> _b;
    };
    OutputWindow _outputWindow;
    struct MonitorGroup : public GroupBox
    {
        MonitorGroup(CGA2NTSCWindow* host)
          : _host(host), _colour(host), _filter(host),
            _scanlines(host), _scaling(host)
        {
            setText("Monitor");
            _connector.setChanged(
                [&](int value) { _host->connectorSet(value); });
            _connector.setText("Connector: ");
            _connector.add("RGBI");
            _connector.add("Composite (old)");
            _connector.add("Composite (new)");
            _connector.set(1);
            add(&_connector);
            add(&_colour);
            add(&_filter);
            add(&_scanlines);
            add(&_scaling);
        }
        void layout()
        {
            Vector vSpace = _host->vSpace();
            _connector.setTopLeft(_host->groupTL());
            int r = _connector.right();
            _colour.setTopLeft(_connector.bottomLeft() + _host->groupVSpace());
            r = max(r, _colour.right());
            _filter.setTopLeft(_colour.bottomLeft() + _host->groupVSpace());
            r = max(r, _filter.right());
            _scanlines.setTopLeft(_filter.bottomLeft() + _host->groupVSpace());
            r = max(r, _scanlines.right());
            _scaling.setTopLeft(
                _scanlines.bottomLeft() + _host->groupVSpace());
            r = max(r, _scaling.right());
            setInnerSize(Vector(r, _scaling.bottom()) + _host->groupBR());
        }
        CaptionedDropDownList _connector;
        struct ColourGroup : public GroupBox
        {
            ColourGroup(CGA2NTSCWindow* host) : _host(host)
            {
                KnobSliders* sliders = &host->_knobSliders;
                setText("Colour");
                _brightness.setSliders(sliders);
                _brightness.setValueSet(
                    [&](double value) { _host->brightnessSet(value); });
                _brightness.setText("Brightness: ");
                _brightness.setRange(-2, 2);
                add(&_brightness);
                _saturation.setSliders(sliders);
                _saturation.setValueSet(
                    [&](double value) { _host->saturationSet(value); });
                _saturation.setText("Saturation: ");
                _saturation.setRange(0, 4);
                add(&_saturation);
                _contrast.setSliders(sliders);
                _contrast.setValueSet(
                    [&](double value) { _host->contrastSet(value); });
                _contrast.setText("Contrast: ");
                _contrast.setRange(0, 4);
                add(&_contrast);
                _hue.setSliders(sliders);
                _hue.setValueSet([&](double value) { _host->hueSet(value); });
                _hue.setText("Hue: ");
                _hue.setRange(-180, 180);
                add(&_hue);
                _showClipping.setClicked(
                    [&](bool value) { _host->showClippingSet(value); });
                _showClipping.setText("Show clipping");
                add(&_showClipping);
            }
            void layout()
            {
                Vector vSpace = _host->vSpace();
                _brightness.setTopLeft(_host->groupTL());
                int r = _brightness.right();
                _saturation.setTopLeft(_brightness.bottomLeft() + vSpace);
                r = max(r, _saturation.right());
                _contrast.setTopLeft(_saturation.bottomLeft() + vSpace);
                r = max(r, _contrast.right());
                _hue.setTopLeft(_contrast.bottomLeft() + vSpace);
                r = max(r, _hue.right());
                _showClipping.setTopLeft(_hue.bottomLeft() + vSpace);
                r = max(r, _showClipping.right());
                setInnerSize(
                    Vector(r, _showClipping.bottom()) + _host->groupBR());
            }
            CGA2NTSCWindow* _host;
            KnobSlider _brightness;
            KnobSlider _saturation;
            KnobSlider _contrast;
            KnobSlider _hue;
            CheckBox _showClipping;
        };
        ColourGroup _colour;
        struct FilterGroup : public GroupBox
        {
            FilterGroup(CGA2NTSCWindow* host) : _host(host)
            {
                KnobSliders* sliders = &host->_knobSliders;
                setText("Filter");
                _chromaBandwidth.setSliders(sliders);
                _chromaBandwidth.setValueSet(
                    [&](double value) { _host->chromaBandwidthSet(value); });
                _chromaBandwidth.setText("Chroma bandwidth: ");
                _chromaBandwidth.setRange(0, 2);
                add(&_chromaBandwidth);
                _lumaBandwidth.setSliders(sliders);
                _lumaBandwidth.setValueSet(
                    [&](double value) { _host->lumaBandwidthSet(value); });
                _lumaBandwidth.setText("Luma bandwidth: ");
                _lumaBandwidth.setRange(0, 2);
                add(&_lumaBandwidth);
                _combFilter.setChanged(
                    [&](int value) { _host->combFilterSet(value); });
                _combFilter.setText("Comb filter: ");
                _combFilter.add("none");
                _combFilter.add("1 line");
                _combFilter.add("2 line");
                _combFilter.add("1 frame");
                _combFilter.add("2 frame");
                add(&_combFilter);
            }
            void layout()
            {
                Vector vSpace = _host->vSpace();
                _chromaBandwidth.setTopLeft(_host->groupTL());
                int r = _chromaBandwidth.right();
                _lumaBandwidth.setTopLeft(
                    _chromaBandwidth.bottomLeft() + vSpace);
                r = max(r, _lumaBandwidth.right());
                _combFilter.setTopLeft(
                    _lumaBandwidth.bottomLeft() + vSpace);
                r = max(r, _combFilter.right());
                setInnerSize(
                    Vector(r, _combFilter.bottom()) + _host->groupBR());
            }
            CGA2NTSCWindow* _host;
            KnobSlider _chromaBandwidth;
            KnobSlider _lumaBandwidth;
            CaptionedDropDownList _combFilter;
        };
        FilterGroup _filter;
        struct ScanlinesGroup : public GroupBox
        {
            ScanlinesGroup(CGA2NTSCWindow* host) : _host(host)
            {
                setText("Scanlines");
                _profile.setChanged(
                    [&](int value) { _host->scanlineProfileSet(value); });
                _profile.setText("Profile: ");
                _profile.add("rectangle");
                _profile.add("triangle");
                _profile.add("circle");
                _profile.add("gaussian");
                add(&_profile);
                _width.setSliders(&_host->_knobSliders);
                _width.setValueSet(
                    [&](double value) { _host->scanlineWidthSet(value); });
                _width.setText("Width: ");
                _width.setRange(0, 1);
                add(&_width);
                _bleeding.setClicked(
                    [&](bool value) { _host->scanlineBleedingSet(value); });
                _bleeding.setText("Bleeding");
                add(&_bleeding);
            }
            void layout()
            {
                Vector vSpace = _host->vSpace();
                _profile.setTopLeft(_host->groupTL());
                int r = _profile.right();
                _width.setTopLeft(_profile.bottomLeft() + vSpace);
                r = max(r, _width.right());
                _bleeding.setTopLeft(_width.bottomLeft() + vSpace);
                r = max(r, _bleeding.right());
                setInnerSize(Vector(r, _bleeding.bottom()) + _host->groupBR());
            }
            CGA2NTSCWindow* _host;
            CaptionedDropDownList _profile;
            KnobSlider _width;
            CheckBox _bleeding;
        };
        ScanlinesGroup _scanlines;
        struct ScalingGroup : public GroupBox
        {
            ScalingGroup(CGA2NTSCWindow* host) : _host(host)
            {
                KnobSliders* sliders = &host->_knobSliders;
                setText("Scaling");
                _zoom.setSliders(sliders);
                _zoom.setValueSet(
                    [&](double value) { _host->zoomSet(value); });
                _zoom.setText("Zoom: ");
                _zoom.setRange(1, 10);
                _zoom.setLogarithmic(true);
                add(&_zoom);
                _aspectRatio.setSliders(sliders);
                _aspectRatio.setValueSet(
                    [&](double value) { _host->aspectRatioSet(value); });
                _aspectRatio.setText("Aspect Ratio: ");
                _aspectRatio.setRange(0.5, 2);
                _aspectRatio.setLogarithmic(true);
                add(&_aspectRatio);
            }
            void layout()
            {
                Vector vSpace = _host->vSpace();
                _zoom.setTopLeft(_host->groupTL());
                int r = _zoom.right();
                _aspectRatio.setTopLeft(_zoom.bottomLeft() + vSpace);
                r = max(r, _aspectRatio.right());
                setInnerSize(Vector(r, _aspectRatio.bottom()) +
                    _host->groupBR());
            }
            CGA2NTSCWindow* _host;
            KnobSlider _zoom;
            KnobSlider _aspectRatio;
        };
        ScalingGroup _scaling;
        CGA2NTSCWindow* _host;
    };
    MonitorGroup _monitor;
    struct VideoCardGroup : public GroupBox
    {
        VideoCardGroup(CGA2NTSCWindow* host)
          : _host(host), _registers(host), _matching(host)
        {
            setText("Video card");
            add(&_registers);
            add(&_matching);
        }
        void layout()
        {
            Vector vSpace = _host->vSpace();
            _registers.setTopLeft(_host->groupTL());
            int r = _registers.right();
            _matching.setTopLeft(
                _registers.bottomLeft() + _host->groupVSpace());
            r = max(r, _matching.right());
            setInnerSize(Vector(r, _matching.bottom()) + _host->groupBR());
        }
        struct RegistersGroup : public GroupBox
        {
            RegistersGroup(CGA2NTSCWindow* host) : _host(host)
            {
                setText("Registers");
                _mode.setChanged([&](int value) { _host->modeSet(value); });
                _mode.setText("Mode: ");
                _mode.add("low-resolution text");
                _mode.add("high-resolution text");
                _mode.add("1bpp graphics");
                _mode.add("2bpp graphics");
                _mode.add("low-res text with 1bpp");
                _mode.add("high-res text with 1bpp");
                _mode.add("high-res 1bpp graphics");
                _mode.add("high-res 2bpp graphics");
                _mode.add("Auto -HRES");
                _mode.add("Auto +HRES");
                _mode.set(2);
                add(&_mode);
                _bw.setClicked(
                    [&](bool value) { _host->bwSet(value); });
                _bw.setText("+BW");
                add(&_bw);
                _blink.setClicked(
                    [&](bool value) { _host->blinkSet(value); });
                _blink.setText("+BLINK");
                add(&_blink);
                _palette.setChanged(
                    [&](int value) { _host->paletteSet(value); });
                _palette.setText("Palette: ");
                _palette.add("2/4/6");
                _palette.add("10/12/14");
                _palette.add("3/5/7");
                _palette.add("11/13/15");
                _palette.set(3);
                add(&_palette);
                _background.setChanged(
                    [&](int value) { _host->backgroundSet(value); });
                _background.setText("Background: ");
                for (int i = 0; i < 16; ++i)
                    _background.add(decimal(i));
                _background.add("Auto");
                _background.set(15);
                add(&_background);
                _scanlinesPerRow.setChanged(
                    [&](int value) { _host->scanlinesPerRowSet(value); });
                _scanlinesPerRow.setText("Scanlines per row: ");
                for (int i = 1; i <= 32; ++i) {
                    _scanlinesPerRow.add(decimal(i));
                    _scanlinesRepeat.add(decimal(i));
                }
                _scanlinesPerRow.set(1);
                add(&_scanlinesPerRow);
                _scanlinesRepeat.setChanged(
                    [&](int value) { _host->scanlinesRepeatSet(value); });
                _scanlinesRepeat.setText("Scanlines repeat: ");
                add(&_scanlinesRepeat);
                _phase.setClicked(
                    [&](bool value) { _host->phaseSet(value); });
                _phase.setText("Phase 0");
                add(&_phase);
                _interlace.setChanged(
                    [&](int value) { _host->interlaceSet(value); });
                _interlace.setText("Interlace: ");
                _interlace.add("None");
                _interlace.add("Flicker");
                _interlace.add("Sync");
                _interlace.add("Sync and video");
                _interlace.add("Even");
                _interlace.add("Odd");
                _interlace.add("Video");
                _interlace.add("Video and flicker");
                _interlace.add("Sync flicker");
                _interlace.add("Sync video and flicker");
                _interlace.add("Even flicker");
                _interlace.add("Odd flicker");
                _interlace.add("Sync even");
                _interlace.add("Sync odd");
                _interlace.add("Sync even flicker");
                _interlace.add("Sync odd flicker");
                _interlace.add("Sync and video swapped");
                _interlace.add("Sync video and flicker swapped");
                add(&_interlace);
            }
            void layout()
            {
                Vector vSpace = _host->vSpace();
                Vector hSpace = _host->hSpace();
                _mode.setTopLeft(_host->groupTL());
                _bw.setTopLeft(_mode.topRight() + hSpace);
                _blink.setTopLeft(_bw.topRight() + hSpace);
                int r = _blink.right();
                _palette.setTopLeft(_mode.bottomLeft() + vSpace);
                _background.setTopLeft(_palette.topRight() + hSpace);
                r = max(r, _background.right());
                _scanlinesPerRow.setTopLeft(_palette.bottomLeft() + vSpace);
                _scanlinesRepeat.setTopLeft(
                    _scanlinesPerRow.topRight() + hSpace);
                r = max(r, _scanlinesRepeat.right());
                _phase.setTopLeft(_scanlinesPerRow.bottomLeft() + vSpace);
                _interlace.setTopLeft(_phase.topRight() + hSpace);
                setInnerSize(Vector(r, _interlace.bottom()) +
                    _host->groupBR());
            }
            CGA2NTSCWindow* _host;
            CaptionedDropDownList _mode;
            CheckBox _bw;
            CheckBox _blink;
            CaptionedDropDownList _palette;
            CaptionedDropDownList _background;
            CaptionedDropDownList _scanlinesPerRow;
            CaptionedDropDownList _scanlinesRepeat;
            CheckBox _phase;
            CaptionedDropDownList _interlace;
        };
        RegistersGroup _registers;
        struct MatchingGroup : public GroupBox
        {
            MatchingGroup(CGA2NTSCWindow* host) : _host(host)
            {
                KnobSliders* sliders = &host->_knobSliders;
                setText("Matching");
                _matchMode.setClicked(
                    [&](bool value) { _host->matchModeSet(value); });
                _matchMode.setText("Match");
                add(&_matchMode);
                _diffusionHorizontal.setSliders(sliders);
                _diffusionHorizontal.setValueSet(
                    [&](double value) {
                        _host->diffusionHorizontalSet(value); });
                _diffusionHorizontal.setText("Diffusion: Horizontal: ");
                _diffusionHorizontal.setRange(0, 1);
                add(&_diffusionHorizontal);
                _diffusionVertical.setSliders(sliders);
                _diffusionVertical.setValueSet(
                    [&](double value) { _host->diffusionVerticalSet(value); });
                _diffusionVertical.setText("Vertical: ");
                _diffusionVertical.setRange(0, 1);
                _diffusionVertical.setCaptionWidth(0);
                add(&_diffusionVertical);
                _diffusionTemporal.setSliders(sliders);
                _diffusionTemporal.setValueSet(
                    [&](double value) { _host->diffusionTemporalSet(value); });
                _diffusionTemporal.setText("Temporal: ");
                _diffusionTemporal.setRange(0, 1);
                _diffusionTemporal.setCaptionWidth(0);
                add(&_diffusionTemporal);
                _quality.setSliders(sliders);
                _quality.setValueSet(
                    [&](double value) { _host->qualitySet(value); });
                _quality.setText("Quality: ");
                _quality.setRange(0, 1);
                add(&_quality);
                _characterSet.setChanged(
                    [&](int value) { _host->characterSetSet(value); });
                _characterSet.setText("Character set: ");
                _characterSet.add("0xdd");
                _characterSet.add("0x13/0x55");
                _characterSet.add("1K");
                _characterSet.add("all");
                _characterSet.add("0xb1");
                _characterSet.add("0xb0/0xb1");
                _characterSet.add("ISAV");
                _characterSet.set(3);
                add(&_characterSet);
            }
            void layout()
            {
                Vector vSpace = _host->vSpace();
                Vector hSpace = _host->hSpace();
                _matchMode.setTopLeft(_host->groupTL());
                int r = _matchMode.right();
                _diffusionHorizontal.setTopLeft(
                    _matchMode.bottomLeft() + vSpace);
                _diffusionVertical.setTopLeft(
                    _diffusionHorizontal.topRight() + hSpace);
                _diffusionTemporal.setTopLeft(
                    _diffusionVertical.topRight() + hSpace);
                r = max(r, _diffusionTemporal.right());
                _quality.setTopLeft(
                    _diffusionHorizontal.bottomLeft() + vSpace);
                _characterSet.setTopLeft(_quality.bottomLeft() + vSpace);
                setInnerSize(Vector(r, _characterSet.bottom()) +
                    _host->groupBR());
            }
            CGA2NTSCWindow* _host;
            ToggleButton _matchMode;
            KnobSlider _diffusionHorizontal;
            KnobSlider _diffusionVertical;
            KnobSlider _diffusionTemporal;
            KnobSlider _quality;
            CaptionedDropDownList _characterSet;
        };
        MatchingGroup _matching;
        CGA2NTSCWindow* _host;
    };
    VideoCardGroup _videoCard;
    KnobSliders _knobSliders;

    CGAMatcher* _matcher;
    CGAShower* _shower;
    CGAOutput* _output;
    Program* _program;
    int _paletteSelected;
    int _backgroundSelected;
    ConfigFile* _config;
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
        configFile.addDefaultOption("showClipping", false);
        configFile.addDefaultOption("chromaBandwidth", 1.0);
        configFile.addDefaultOption("lumaBandwidth", 1.0);
        configFile.addDefaultOption("horizontalDiffusion", 0.5);
        configFile.addDefaultOption("verticalDiffusion", 0.5);
        configFile.addDefaultOption("temporalDiffusion", 0.0);
        configFile.addDefaultOption("quality", 0.5);
        configFile.addDefaultOption("connector", 1);
        configFile.addDefaultOption("characterSet", 3);
        configFile.addDefaultOption("cgaROM", String("5788005.u33"));
        configFile.addDefaultOption("aspectRatio", 5.0/6.0);
        configFile.addDefaultOption("scanlineWidth", 0.5);
        configFile.addDefaultOption("scanlineProfile", 0);
        configFile.addDefaultOption("scanlineBleeding", false);
        configFile.addDefaultOption("zoom", 2.0);
        configFile.addDefaultOption("phase", 1);
        configFile.addDefaultOption("interactive", true);
        configFile.addDefaultOption("combFilter", 0);
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

        CGAOutput output;
        _output = &output;
        _matcher.setProgram(this);
        _window.setConfig(&configFile);
        _window.setMatcher(&_matcher);
        _window.setShower(&_shower);
        _window.setOutput(&output);
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

        double brightness = configFile.get<double>("brightness");
        output.setBrightness(brightness);
        _matcher.setBrightness(brightness);
        double saturation = configFile.get<double>("saturation");
        output.setSaturation(saturation);
        _matcher.setSaturation(saturation);
        double hue = configFile.get<double>("hue");
        output.setHue(hue);
        _matcher.setHue(hue);
        double contrast = configFile.get<double>("contrast");
        output.setContrast(contrast);
        _matcher.setContrast(contrast);
        output.setShowClipping(configFile.get<bool>("showClipping"));
        output.setChromaBandwidth(configFile.get<double>("chromaBandwidth"));
        output.setLumaBandwidth(configFile.get<double>("lumaBandwidth"));
        int connector = configFile.get<int>("connector");
        output.setConnector(connector);
        _matcher.setNewCGA(connector == 2);
        output.setScanlineWidth(configFile.get<double>("scanlineWidth"));
        output.setScanlineProfile(configFile.get<int>("scanlineProfile"));
        output.setZoom(configFile.get<double>("zoom"));
        output.setScanlineBleeding(configFile.get<bool>("scanlineBleeding"));
        output.setAspectRatio(configFile.get<double>("aspectRatio"));
        output.setCombFilter(configFile.get<int>("combFilter"));

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
            output.setWindow(&_window);
            WindowProgram::run();
        }

        if (!_matchMode)
            _matcher.cancel();
        _matcher.join();
        output.reCreateNTSC();
        output.join();

        if (!interactive)
            timer.output("Elapsed time");

        String inputFileName = bitmapValue.name();
        int i;
        for (i = inputFileName.length() - 1; i >= 0; --i)
            if (inputFileName[i] == '.')
                break;
        if (i != -1)
            inputFileName = inputFileName.subString(0, i);

        output.save(inputFileName + "_out.png");
        _matcher.save(inputFileName + "_out.dat");
        _matcher.saveRGBI(inputFileName + "_out.rgbi");
        _matcher.savePalettes(inputFileName + "_out.palettes");
    }
    bool getMatchMode() { return _matchMode; }
    void setMatchMode(bool matchMode)
    {
        _matchMode = matchMode;
        if (!matchMode)
            _output->setRGBI(_shower.getOutput());
        else
            _output->setRGBI(_matcher.getOutput());
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
    CGAOutput* _output;
    bool _matchMode;
    bool _updateNeeded;
};
