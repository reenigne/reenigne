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

template<class T> class BrightnessSliderWindowT
  : public KnobSlider<CGA2NTSCWindow>
{
public:
    void valueSet(double value) { _host->brightnessSet(value); }
    void create()
    {
        setText("Brightness: ");
        setRange(-2, 2);
        KnobSlider<CGA2NTSCWindow>::create();
    }
};
typedef BrightnessSliderWindowT<void> BrightnessSliderWindow;

template<class T> class SaturationSliderWindowT
  : public KnobSlider<CGA2NTSCWindow>
{
public:
    void valueSet(double value) { _host->saturationSet(value); }
    void create()
    {
        setText("Saturation: ");
        setRange(0, 4);
        KnobSlider<CGA2NTSCWindow>::create();
    }
};
typedef SaturationSliderWindowT<void> SaturationSliderWindow;

template<class T> class ContrastSliderWindowT
  : public KnobSlider<CGA2NTSCWindow>
{
public:
    void valueSet(double value) { _host->contrastSet(value); }
    void create()
    {
        setText("Contrast: ");
        setRange(0, 4);
        KnobSlider<CGA2NTSCWindow>::create();
    }
};
typedef ContrastSliderWindowT<void> ContrastSliderWindow;

template<class T> class HueSliderWindowT : public KnobSlider<CGA2NTSCWindow>
{
public:
    void valueSet(double value) { _host->hueSet(value); }
    void create()
    {
        setText("Hue: ");
        setRange(-180, 180);
        KnobSlider<CGA2NTSCWindow>::create();
    }
};
typedef HueSliderWindowT<void> HueSliderWindow;

template<class T> class ChromaBandwidthSliderWindowT
  : public KnobSlider<CGA2NTSCWindow>
{
public:
    void valueSet(double value) { _host->chromaBandwidthSet(value); }
    void create()
    {
        setText("Chroma bandwidth: ");
        setRange(0, 2);
        KnobSlider<CGA2NTSCWindow>::create();
    }
};
typedef ChromaBandwidthSliderWindowT<void> ChromaBandwidthSliderWindow;

template<class T> class LumaBandwidthSliderWindowT
  : public KnobSlider<CGA2NTSCWindow>
{
public:
    void valueSet(double value) { _host->lumaBandwidthSet(value); }
    void create()
    {
        setText("Luma bandwidth: ");
        setRange(0, 2);
        KnobSlider<CGA2NTSCWindow>::create();
    }
};
typedef LumaBandwidthSliderWindowT<void> LumaBandwidthSliderWindow;

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

template<class T> class ModeComboT : public CaptionedComboBox<CGA2NTSCWindow>
{
public:
    void changed(int value) { _host->modeSet(value); }
    void create()
    {
        setText("Mode: ");
        CaptionedComboBox<CGA2NTSCWindow>::create();
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
};
typedef ModeComboT<void> ModeCombo;

template<class T> class BackgroundComboT
  : public CaptionedComboBox<CGA2NTSCWindow>
{
public:
    void changed(int value) { _host->backgroundSet(value); }
    void create()
    {
        setText("Background: ");
        CaptionedComboBox<CGA2NTSCWindow>::create();
        for (int i = 0; i < 16; ++i)
            add(decimal(i));
        add("Auto");
        set(15);
        autoSize();
    }
};
typedef BackgroundComboT<void> BackgroundCombo;

template<class T> class ScanlinesPerRowComboT
  : public CaptionedComboBox<CGA2NTSCWindow>
{
public:
    void changed(int value) { _host->scanlinesPerRowSet(value); }
    void create()
    {
        setText("Scanlines per row: ");
        CaptionedComboBox<CGA2NTSCWindow>::create();
        for (int i = 1; i <= 32; ++i)
            add(decimal(i));
        set(1);
        autoSize();
    }
};
typedef ScanlinesPerRowComboT<void> ScanlinesPerRowCombo;

template<class T> class ScanlinesRepeatComboT
  : public CaptionedComboBox<CGA2NTSCWindow>
{
public:
    void changed(int value) { _host->scanlinesRepeatSet(value); }
    void create()
    {
        setText("Scanlines repeat: ");
        CaptionedComboBox<CGA2NTSCWindow>::create();
        for (int i = 1; i <= 32; ++i)
            add(decimal(i));
        set(0);
        autoSize();
    }
};
typedef ScanlinesRepeatComboT<void> ScanlinesRepeatCombo;

template<class T> class PaletteComboT
  : public CaptionedComboBox<CGA2NTSCWindow>
{
public:
    void changed(int value) { _host->paletteSet(value); }
    void create()
    {
        setText("Palette: ");
        CaptionedComboBox<CGA2NTSCWindow>::create();
        add("2/4/6");
        add("10/12/14");
        add("3/5/7");
        add("11/13/15");
        set(3);
        autoSize();
    }
};
typedef PaletteComboT<void> PaletteCombo;

template<class T> class DiffusionHorizontalSliderWindowT
  : public KnobSlider<CGA2NTSCWindow>
{
public:
    void valueSet(double value) { _host->diffusionHorizontalSet(value); }
    void create()
    {
        setText("Horizontal diffusion: ");
        setRange(0, 1);
        KnobSlider<CGA2NTSCWindow>::create();
    }
};
typedef DiffusionHorizontalSliderWindowT<void> DiffusionHorizontalSliderWindow;

template<class T> class DiffusionVerticalSliderWindowT
  : public KnobSlider<CGA2NTSCWindow>
{
public:
    void valueSet(double value) { _host->diffusionVerticalSet(value); }
    void create()
    {
        setText("Vertical diffusion: ");
        setRange(0, 1);
        KnobSlider<CGA2NTSCWindow>::create();
    }
};
typedef DiffusionVerticalSliderWindowT<void> DiffusionVerticalSliderWindow;

template<class T> class DiffusionTemporalSliderWindowT
  : public KnobSlider<CGA2NTSCWindow>
{
public:
    void valueSet(double value) { _host->diffusionTemporalSet(value); }
    void create()
    {
        setText("Temporal diffusion: ");
        setRange(0, 1);
        KnobSlider<CGA2NTSCWindow>::create();
    }
};
typedef DiffusionTemporalSliderWindowT<void> DiffusionTemporalSliderWindow;

template<class T> class QualitySliderWindowT
  : public KnobSlider<CGA2NTSCWindow>
{
public:
    void valueSet(double value) { _host->qualitySet(value); }
    void create()
    {
        setText("Quality: ");
        setRange(0, 1);
        KnobSlider<CGA2NTSCWindow>::create();
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

template<class T> class InterlaceComboT
  : public CaptionedComboBox<CGA2NTSCWindow>
{
public:
    void changed(int value) { _host->interlaceSet(value); }
    void create()
    {
        setText("Interlace: ");
        CaptionedComboBox<CGA2NTSCWindow>::create();
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
};
typedef InterlaceComboT<void> InterlaceCombo;

template<class T> class CharacterSetComboT
  : public CaptionedComboBox<CGA2NTSCWindow>
{
public:
    void changed(int value) { _host->characterSetSet(value); }
    void create()
    {
        setText("Character set: ");
        CaptionedComboBox<CGA2NTSCWindow>::create();
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
};
typedef CharacterSetComboT<void> CharacterSetCombo;

template<class T> class ScanlineWidthSliderWindowT
  : public KnobSlider<CGA2NTSCWindow>
{
public:
    void valueSet(double value) { _host->scanlineWidthSet(value); }
    void create()
    {
        setText("Scanline width: ");
        setRange(0, 1);
        KnobSlider<CGA2NTSCWindow>::create();
    }
};
typedef ScanlineWidthSliderWindowT<void> ScanlineWidthSliderWindow;

template<class T> class ScanlineProfileComboT
  : public CaptionedComboBox<CGA2NTSCWindow>
{
public:
    void changed(int value) { _host->characterSetSet(value); }
    void create()
    {
        setText("Scanline profile: ");
        CaptionedComboBox<CGA2NTSCWindow>::create();
        add("rectangular");
        add("triangle");
        add("semicircle");
        add("gaussian");
        set(0);
        autoSize();
    }
};
typedef ScanlineProfileComboT<void> ScanlineProfileCombo;

template<class T> class ZoomSliderWindowT : public KnobSlider<CGA2NTSCWindow>
{
public:
    void valueSet(double value) { _host->zoomSet(value); }
    void create()
    {
        setText("Zoom: ");
        setRange(1, 10);
        KnobSlider<CGA2NTSCWindow>::create();
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

template<class T> class AspectRatioSliderWindowT
  : public KnobSlider<CGA2NTSCWindow>
{
public:
    void valueSet(double value) { _host->aspectRatioSet(value); }
    void create()
    {
        setText("Aspect Ratio: ");
        setRange(1, 2);
        KnobSlider<CGA2NTSCWindow>::create();
    }
};
typedef AspectRatioSliderWindowT<void> AspectRatioSliderWindow;

template<class T> class CombFilterVerticalComboT
  : public CaptionedComboBox<CGA2NTSCWindow>
{
public:
    void changed(int value) { _host->combFilterVerticalSet(value); }
    void create()
    {
        setText("Comb filter vertical: ");
        CaptionedComboBox<CGA2NTSCWindow>::create();
        add("none");
        add("(1, 1)");
        add("(1, 2, 1)");
        set(0);
        autoSize();
    }
};
typedef CombFilterVerticalComboT<void> CombFilterVerticalCombo;

template<class T> class CombFilterTemporalComboT
  : public CaptionedComboBox<CGA2NTSCWindow>
{
public:
    void changed(int value) { _host->combFilterTemporalSet(value); }
    void create()
    {
        setText("Comb filter temporal: ");
        CaptionedComboBox<CGA2NTSCWindow>::create();
        add("none");
        add("(1, 1)");
        add("(1, 2, 1)");
        set(0);
        autoSize();
    }
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
        if (size().x < 0 || size().x >= 0x4000 || size().y < 0 ||
            size().y >= 0x4000)
            return;
        if (!_bitmap.valid())
            _bitmap = Bitmap<DWORD>(size());
        // Just copy from the top left corner for now.
        Vector zero(0, 0);
        Vector size(min(bitmap.size().x, _bitmap.size().x),
            min(bitmap.size().y, _bitmap.size().y));
        bitmap.subBitmap(zero, size).copyTo(_bitmap.subBitmap(zero, size));
        _bitmap = setNextBitmap(_bitmap);
        invalidate();
        postMessage(WM_USER + 1);
    }
    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_USER + 1) {
            updateWindow();
            return 0;
        }
        return BitmapWindow::handleMessage(uMsg, wParam, lParam);
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
        Bitmap<DWORD> bitmap;
        {
            Lock lock(&_mutex);
            ntsc = _ntsc;
            bitmap = _bitmap;
        }
        const Byte* ntscRow = ntsc.data();
        Byte* outputRow = bitmap.data();
        for (int yy = 0; yy < ntsc.size().y; ++yy) {
            _decoder.decodeLine(ntscRow, reinterpret_cast<DWORD*>(outputRow),
                ntsc.size().x - 6, bitmap.size().x);
            memcpy(outputRow + bitmap.stride(), outputRow,
                bitmap.size().x*sizeof(DWORD));
            outputRow += bitmap.stride()*2;
            ntscRow += ntsc.stride();
        }
        if (_window != 0)
            _window->draw(bitmap);
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
        Lock lock(&_mutex);
        allocateBitmap2();
    }
    void allocateBitmap2()
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
    double _zoom;
    bool _scanlineBleeding;
    double _aspectRatio;
    int _combFilterVertical;
    int _combFilterTemporal;
    CGA2NTSCWindow* _window;
    Mutex _mutex;
};

typedef CGAOutputT<void> CGAOutput;

template<class T> class CGA2NTSCWindowT : public RootWindow
{
public:
    CGA2NTSCWindowT()
    {
        add(&_outputWindow);
        _brightness.setHost(this);
        _saturation.setHost(this);
        _contrast.setHost(this);
        _hue.setHost(this);
        _chromaBandwidth.setHost(this);
        _lumaBandwidth.setHost(this);
        add2(&_newCGA);
        add2(&_matchMode);
        _mode.setHost(this);
        _background.setHost(this);
        _palette.setHost(this);
        _scanlinesPerRow.setHost(this);
        _scanlinesRepeat.setHost(this);
        _diffusionHorizontal.setHost(this);
        _diffusionVertical.setHost(this);
        _diffusionTemporal.setHost(this);
        _quality.setHost(this);
        add2(&_bwCheckBox);
        add2(&_blinkCheckBox);
        add2(&_phaseCheckBox);
        _interlaceCombo.setHost(this);
        _characterSetCombo.setHost(this);
        _scanlineWidth.setHost(this);
        _scanlineProfile.setHost(this);
        _zoom.setHost(this);
        add2(&_scanlineBleeding);
        _aspectRatio.setHost(this);
        _combFilterVertical.setHost(this);
        _combFilterTemporal.setHost(this);
    }
    void create()
    {
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
        _brightness.setValue(_output->getBrightness());
        _saturation.setValue(_output->getSaturation());
        _hue.setValue(_output->getHue());
        _contrast.setValue(_output->getContrast());
        _chromaBandwidth.setValue(_output->getChromaBandwidth());
        _lumaBandwidth.setValue(_output->getLumaBandwidth());
        _newCGA.setCheckState(_output->getNewCGA());
        _scanlineWidth.setValue(_output->getScanlineWidth());
        _scanlineProfile.set(_output->getScanlineProfile());
        _zoom.setValue(_output->getZoom());
        _scanlineBleeding.setCheckState(_output->getScanlineBleeding());
        _aspectRatio.setValue(_output->getAspectRatio());
        _combFilterVertical.set(_output->getCombFilterVertical());
        _combFilterTemporal.set(_output->getCombFilterTemporal());
        _matchMode.setCheckState(_program->getMatchMode());
        matchModePressed();

        setText("CGA to NTSC");
        setSize(Vector(781, 830));
        RootWindow::create();
    }
    void sizeSet(Vector size)
    {
        RootWindow::sizeSet(size);

        Vector vSpace(0, 15);
        Vector hSpace(15, 0);
        Vector ks(180, 24);
        int captionWidth = 100;

        Vector pad(20, 20);

        _newCGA.setPosition(
            Vector(size.x - (_newCGA.size().x + pad.x), pad.y));
        _brightness.setPositionAndSize(
            Vector(size.x - (ks.x + pad.x), _newCGA.bottom()) + vSpace, ks,
            captionWidth);
        _saturation.setPositionAndSize(_brightness.bottomLeft() + vSpace, ks,
            captionWidth);
        _contrast.setPositionAndSize(_saturation.bottomLeft() + vSpace, ks,
            captionWidth);
        _hue.setPositionAndSize(_contrast.bottomLeft() + vSpace, ks,
            captionWidth);

        _chromaBandwidth.setPositionAndSize(_hue.bottomLeft() + vSpace, ks,
            captionWidth);
        _lumaBandwidth.setPositionAndSize(
            _chromaBandwidth.bottomLeft() + vSpace, ks, captionWidth);
        _combFilterVertical.setTopLeft(_lumaBandwidth.bottomLeft() + vSpace);
        _combFilterTemporal.setTopLeft(_combFilterVertical.bottomLeft() +
            vSpace);

        _scanlineProfile.setTopLeft(_combFilterTemporal.bottomLeft() + vSpace);
        _scanlineWidth.setPositionAndSize(
            _scanlineProfile.bottomLeft() + vSpace, ks, captionWidth);
        _scanlineBleeding.setPosition(_scanlineWidth.bottomLeft() + vSpace);

        _zoom.setPositionAndSize(_scanlineBleeding.bottomLeft() + vSpace, ks,
            captionWidth);
        _aspectRatio.setPositionAndSize(_zoom.bottomLeft() + vSpace, ks,
            captionWidth);


        _characterSetCombo.setTopLeft(
            Vector(pad.x, size.y - (_characterSetCombo.size().y + pad.y)));
        _quality.setPositionAndSize(_characterSetCombo.topLeft()
            - (Vector(0, _quality.size().y) + vSpace), ks, captionWidth);
        _diffusionTemporal.setPositionAndSize(_quality.topLeft()
            - (Vector(0, _diffusionTemporal.size().y) + vSpace), ks,
            captionWidth);
        _diffusionVertical.setPositionAndSize(_diffusionTemporal.topLeft()
            - (Vector(0, _diffusionVertical.size().y) + vSpace), ks,
            captionWidth);
        _diffusionHorizontal.setPositionAndSize(_diffusionVertical.topLeft()
            - (Vector(0, _diffusionHorizontal.size().y) + vSpace), ks,
            captionWidth);

        _phaseCheckBox.setPosition(_diffusionHorizontal.topLeft() -
            (Vector(0, _phaseCheckBox.size().y) + vSpace));
        _interlaceCombo.setTopLeft(_phaseCheckBox.topRight() + hSpace);
        _scanlinesPerRow.setTopLeft(_phaseCheckBox.topLeft() -
            (Vector(0, _scanlinesPerRow.size().y) + vSpace));
        _scanlinesRepeat.setTopLeft(_scanlinesPerRow.topRight() + hSpace);
        _palette.setTopLeft(_scanlinesPerRow.topLeft() -
            (Vector(0, _palette.size().y) + vSpace));
        _background.setTopLeft(_palette.topRight() + hSpace);

        _mode.setTopLeft(_palette.topLeft() -
            (Vector(0, _mode.size().y) + vSpace));
        _bwCheckBox.setPosition(_mode.topRight() + hSpace);
        _blinkCheckBox.setPosition(_bwCheckBox.topRight() + hSpace);

        _matchMode.setPosition(_mode.topLeft() -
            (Vector(0, _matchMode.size().y) + vSpace));


        _outputWindow.setPosition(pad);
        _outputWindow.setSize(Vector(_brightness.left(), _matchMode.top())
            - 2*pad);
        //console.write(format("%i %i\n", _outputWindow.size().x, _outputWindow.size().y));
    }
    void keyboardCharacter(int character)
    {
        if (character == VK_ESCAPE)
            remove();
    }
    void setConfig(ConfigFile* config)
    {
        _brightness.setConfig(config);
        _saturation.setConfig(config);
        _contrast.setConfig(config);
        _hue.setConfig(config);
        _chromaBandwidth.setConfig(config);
        _lumaBandwidth.setConfig(config);
        _diffusionHorizontal.setConfig(config);
        _diffusionVertical.setConfig(config);
        _diffusionTemporal.setConfig(config);
        _quality.setConfig(config);
        _scanlineWidth.setConfig(config);
        _zoom.setConfig(config);
        _aspectRatio.setConfig(config);
    }
    void setMatcher(CGAMatcher* matcher) { _matcher = matcher; }
    void setShower(CGAShower* shower) { _shower = shower; }
    void setOutput(CGAOutput* output) { _output = output; }
    void setProgram(Program* program) { _program = program; }
    void draw(Bitmap<DWORD> bitmap) { _outputWindow.draw(bitmap); }
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
    void chromaBandwidthSet(double chromaBandwidth)
    {
        _output->setChromaBandwidth(chromaBandwidth);
    }
    void lumaBandwidthSet(double lumaBandwidth)
    {
        _output->setLumaBandwidth(lumaBandwidth);
    }
    void newCGAPressed()
    {
        bool newCGA = _newCGA.checked();
        _output->setNewCGA(newCGA);
        _matcher->setNewCGA(newCGA);
        beginConvert();
    }

    void reCreateNTSC()
    {
        _output->reCreateNTSC();
    }

private:
    template<class T> void add2(T* p)
    {
        add(p);
        p->setHost(this);
    }

    OutputWindow _outputWindow;
    BrightnessSliderWindow _brightness;
    SaturationSliderWindow _saturation;
    ContrastSliderWindow _contrast;
    HueSliderWindow _hue;
    ChromaBandwidthSliderWindow _chromaBandwidth;
    LumaBandwidthSliderWindow _lumaBandwidth;
    NewCGAButtonWindow _newCGA;
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
    ZoomSliderWindow _zoom;
    ScanlineBleedingCheckBox _scanlineBleeding;
    AspectRatioSliderWindow _aspectRatio;
    CombFilterVerticalCombo _combFilterVertical;
    CombFilterTemporalCombo _combFilterTemporal;
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
        configFile.addDefaultOption("chromaBandwidth", 1.0);
        configFile.addDefaultOption("lumaBandwidth", 1.0);
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
        _window.setConfig(&configFile);
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
