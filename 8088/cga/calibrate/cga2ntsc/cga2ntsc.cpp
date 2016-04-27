#include "alfe/main.h"
#include "alfe/bitmap_png.h"
#include "alfe/complex.h"
#include "alfe/space.h"
#include "alfe/set.h"
#include "alfe/config_file.h"
#include "alfe/cga.h"

class NTSCDecoder
{
public:
    void calculateBurst(Byte* burst)
    {
        Complex<double> iq;
        iq.x = burst[0] - burst[2];
        iq.y = burst[1] - burst[3];
        _iqAdjust =
            -iq.conjugate()*unit((33 + 90 + _hue)/360.0)*_saturation*_contrast/
            (iq.modulus()*16);
        _contrast2 = _contrast/32;
        _brightness2 = _brightness*256.0;
    }
    Colour decode(int* s)
    {
        int dc = (s[0] + s[1] + s[2] + s[3])*8;
        Complex<int> iq;
        iq.x = (s[0] - s[2])*8;
        iq.y = (s[1] - s[3])*8;
        return decode(dc, iq);
    }
    Colour decode(const Byte* n, int phase)
    {
        // Filter kernel must be divisible by (1,1,1,1) so that all phases
        // contribute equally.
        int y = n[0] +n[1]*4 +n[2]*7 +n[3]*8 +n[4]*7 +n[5]*4 +n[6];
        Complex<int> iq;
        switch (phase) {
            case 0:
                iq.x =  n[0]   -n[2]*7 +n[4]*7 -n[6];
                iq.y =  n[1]*4 -n[3]*8 +n[5]*4;
                break;
            case 1:
                iq.x = -n[1]*4 +n[3]*8 -n[5]*4;
                iq.y =  n[0]   -n[2]*7 +n[4]*7 -n[6];
                break;
            case 2:
                iq.x = -n[0]   +n[2]*7 -n[4]*7 +n[6];
                iq.y = -n[1]*4 +n[3]*8 -n[5]*4;
                break;
            case 3:
                iq.x = +n[1]*4 -n[3]*8 +n[5]*4;
                iq.y = -n[0]   +n[2]*7 -n[4]*7 +n[6];
                break;
        }
        return decode(y, iq);
    }
    void decodeLine(const Byte* ntsc, SRGB* srgb, int length, int phase)
    {
        for (int x = 0; x < length; ++x) {
            phase = (phase + 1) & 3;
            Colour s = decode(ntsc, phase);
            ++ntsc;
            *srgb = SRGB(byteClamp(s.x), byteClamp(s.y), byteClamp(s.z));
            ++srgb;
        }
    }
    void encodeLine(Byte* ntsc, const SRGB* srgb, int length, int phase)
    {
        phase = (phase + 3) & 3;
        for (int x = 0; x < length; ++x) {
            Vector3<int> mix = Vector3Cast<int>(srgb[0]) +
                4*Vector3Cast<int>(srgb[1]) + 7*Vector3Cast<int>(srgb[2]) +
                8*Vector3Cast<int>(srgb[3]) + 7*Vector3Cast<int>(srgb[4]) +
                4*Vector3Cast<int>(srgb[5]) + Vector3Cast<int>(srgb[6]);
            ++srgb;
            Colour c;
            if (_fixPrimaries) {
                c.x = (0.6689*mix.x + 0.2679*mix.y + 0.0323*mix.z);
                c.y = (0.0185*mix.x + 1.0743*mix.y - 0.0603*mix.z);
                c.z = (0.0162*mix.x + 0.0431*mix.y + 0.8551*mix.z);
            }
            else
                c = Colour(mix.x, mix.y, mix.z);
            Complex<double> iq;
            double y = 0.299*c.x + 0.587*c.y + 0.114*c.z;
            iq.x = 0.596*c.x - 0.275*c.y - 0.321*c.z;
            iq.y = 0.212*c.x - 0.528*c.y + 0.311*c.z;
            iq /= (_iqAdjust*512);
            y = (y/32 - _brightness2)/(_contrast2*32);
            switch (phase) {
                case 0:
                    *ntsc = byteClamp(y + iq.x);
                    break;
                case 1:
                    *ntsc = byteClamp(y + iq.y);
                    break;
                case 2:
                    *ntsc = byteClamp(y - iq.x);
                    break;
                case 3:
                    *ntsc = byteClamp(y - iq.y);
                    break;
            }
            ++ntsc;
            phase = (phase + 1) & 3;
        }
    }

    bool _fixPrimaries;
    double _hue;
    double _saturation;
    double _contrast;
    double _brightness;
    double _sharpness;
private:
    Colour decode(int y, Complex<int> iq)
    {
        double y2 = y*_contrast2 + _brightness2;
        Complex<double> iq2 = Complex<double>(iq)*_iqAdjust;
        double r = y2 + 0.9563*iq2.x + 0.6210*iq2.y;
        double g = y2 - 0.2721*iq2.x - 0.6474*iq2.y;
        double b = y2 - 1.1069*iq2.x + 1.7046*iq2.y;
        if (_fixPrimaries)
            return Colour(
                 1.5073*r -0.3725*g -0.0832*b,
                -0.0275*r +0.9350*g +0.0670*b,
                -0.0272*r -0.0401*g +1.1677*b);
        return Colour(r, g, b);
    }

    Complex<double> _iqAdjust;
    double _contrast2;
    double _brightness2;
};

class CGA2NTSCWindow;

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
private:
    Vector _size;
    Bitmap<SRGB> _input;
    Bitmap<Byte> _rgbi;
    int _mode;
    int _palette;
};

template<class T> class CGAMatcherT
{
public:
    CGAMatcherT() : _skip(256), _converting(false)
    {
        _patterns.allocate(0x10000*8*17 + 0x100*80*5);
    }
    void setInput(Bitmap<SRGB> input)
    {
        _input = input;
        _size = input.size();
        _rgbi = Bitmap<Byte>(_size + Vector(14, 0));
        _input2 = Bitmap<SRGB>(_size + Vector(11, 0));
        _input2.fill(SRGB(0, 0, 0));
        _input2.subBitmap(Vector(5, 0), _size).copyFrom(_input);
        _configs.allocate(_size.y);
    }
    void setWindow(CGA2NTSCWindow* window) { _window = window; }
    static void filterHF(const Byte* input, SInt16* output, int n)
    {
        for (int x = 0; x < n; ++x)
            output[x] = (-input[x] + input[x+1]*2 + input[x+2]*6 + input[x+3]*2
                -input[x+4]);
    }
    void convert()
    {
        _block.y = _scanlinesPerRow * _scanlinesRepeat;
        if ((_mode & 2) != 0) {
            // In graphics modes, the data for the second scanline of the row
            // is independent of the data for the first scanline, so we can
            // pretend there's one scanline per rof ro matching purposes.
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
            _decoder->encodeLine(&ntscTemp[0],
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

        _rgbi.fill((_mode & 0x10) == 0 ? _palette & 0x0f : 0);
        _data.allocate((_size.y/_block.y)*(_size.x/_hdots));
        _converting = true;
        _y = 0;
        _rgbiRow = _rgbi.data() + 7;
        _inputRow = _ntscInput.data();
        _error = Bitmap<int>(_size + Vector(4, 1));
        _error.fill(0);
        _errorRow = _error.data();
        _testError = Bitmap<int>(_block + Vector(4, 1));
        _window->resetColours();
        _config = _startConfig;
        _testConfig = (_startConfig + 1 != _endConfig);
        _configScore = 0x7fffffffffffffffUL;
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
    bool idle()
    {
        if (!_converting)
            return false;
        int w = _block.x + 1;
        Vector errorLineSize(_size.x + 4, 1);
        Bitmap<int> savedError(errorLineSize);
        if (_testConfig)
            savedError.copyFrom(_error.subBitmap(Vector(0, _y),
                errorLineSize));
        config();
        SInt16* p = &_patterns[(_config & 0x7f)*5*256];
        UInt64 lineScore = 0;
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
                            (errorPixel[xx] + _testError[p])/8;
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
                    _window->addColour(static_cast<UInt64>(seq) |
                        (static_cast<UInt64>(xx & 3) << 32));
                }
            }

            int address = (_y/_block.y)*(_size.x/_hdots) + x/_hdots;
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
                    int target = inputPixel[xx] + errorPixel[xx]/8;
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
        _window->reCreateNTSC();
        bool advance = false;
        if (_testConfig) {
            if (lineScore < _configScore) {
                _configScore = lineScore;
                _bestConfig = _config;
            }
            ++_config;
            if (_config == _endConfig) {
                _config = _bestConfig;
                _configs[_y] = _bestConfig;
                _testConfig = false;
                _configScore = 0x7fffffffffffffffUL;
            }
            else {
                savedError.copyTo(_error.subBitmap(Vector(0, _y),
                    errorLineSize));
                _error.subBitmap(Vector(0, _y + 1), errorLineSize).fill(0);
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
            _y += _block.y;
            if (_y >= _size.y + 1 - _block.y)
                _converting = false;
        }
        return _converting;
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

    void setDecoder(NTSCDecoder* decoder) { _decoder = decoder; }

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
    int getInterlace() { _interlace; }
    void setQuality(double quality) { _quality = quality; }
    double getQuality() { return _quality; }
    void setCharacterSet(int characterSet) { _characterSet = characterSet; }
    int getCharacterSet() { return _characterSet; }

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
    NTSCDecoder* _decoder;
    CGA2NTSCWindow* _window;
    CGASequencer _sequencer;
    bool _converting;
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

class Particle
{
public:
    void plot(Bitmap<DWORD> bitmap, Vector rPosition)
    {
        Vector size = bitmap.size();
        Byte* buffer = bitmap.data();
        int byteWidth = bitmap.stride();
        double zOffset = rPosition.x*0.01;
        double scale = rPosition.y*0.01;
        double x = _position.x/(_position.z + zOffset)*scale;
        double y = _position.y/(_position.z + zOffset)*scale;
        int x0 = static_cast<int>(size.x*x/5.0 + size.x/2);
        int y0 = static_cast<int>(size.x*y/5.0 + size.y/2);
        int r = byteClamp(_colour.x);
        int g = byteClamp(_colour.y);
        int b = byteClamp(_colour.z);
        DWord c = (r << 16) | (g << 8) | b;
        plot(bitmap, Vector(x0,     y0    ), c);
        if (!_big) {
            if (r < 16 && g < 16 && b < 16) {
                c = 0xffffff;
                plot(bitmap, Vector(x0 - 1, y0 - 1), c);
                plot(bitmap, Vector(x0 + 1, y0 - 1), c);
                plot(bitmap, Vector(x0 - 1, y0 + 1), c);
                plot(bitmap, Vector(x0 + 1, y0 + 1), c);
            }
            return;
        }
        plot(bitmap, Vector(x0 - 1, y0 - 2), c);
        plot(bitmap, Vector(x0,     y0 - 2), c);
        plot(bitmap, Vector(x0 + 1, y0 - 2), c);
        plot(bitmap, Vector(x0 - 2, y0 - 1), c);
        plot(bitmap, Vector(x0 - 1, y0 - 1), c);
        plot(bitmap, Vector(x0,     y0 - 1), c);
        plot(bitmap, Vector(x0 + 1, y0 - 1), c);
        plot(bitmap, Vector(x0 + 2, y0 - 1), c);
        plot(bitmap, Vector(x0 - 2, y0    ), c);
        plot(bitmap, Vector(x0 - 1, y0    ), c);
        plot(bitmap, Vector(x0 + 1, y0    ), c);
        plot(bitmap, Vector(x0 + 2, y0    ), c);
        plot(bitmap, Vector(x0 - 2, y0 + 1), c);
        plot(bitmap, Vector(x0 - 1, y0 + 1), c);
        plot(bitmap, Vector(x0,     y0 + 1), c);
        plot(bitmap, Vector(x0 + 1, y0 + 1), c);
        plot(bitmap, Vector(x0 + 2, y0 + 1), c);
        plot(bitmap, Vector(x0 - 1, y0 + 2), c);
        plot(bitmap, Vector(x0,     y0 + 2), c);
        plot(bitmap, Vector(x0 + 1, y0 + 2), c);
        if (r < 16 && g < 16 && b < 16) {
            c = 0xffffff;
            plot(bitmap, Vector(x0 - 1, y0 - 3), c);
            plot(bitmap, Vector(x0,     y0 - 3), c);
            plot(bitmap, Vector(x0 + 1, y0 - 3), c);
            plot(bitmap, Vector(x0 - 1, y0 + 3), c);
            plot(bitmap, Vector(x0,     y0 + 3), c);
            plot(bitmap, Vector(x0 + 1, y0 + 3), c);
            plot(bitmap, Vector(x0 - 2, y0 - 2), c);
            plot(bitmap, Vector(x0 + 2, y0 - 2), c);
            plot(bitmap, Vector(x0 - 3, y0 - 1), c);
            plot(bitmap, Vector(x0 + 3, y0 - 1), c);
            plot(bitmap, Vector(x0 - 3, y0    ), c);
            plot(bitmap, Vector(x0 + 3, y0    ), c);
            plot(bitmap, Vector(x0 - 3, y0 + 1), c);
            plot(bitmap, Vector(x0 + 3, y0 + 1), c);
            plot(bitmap, Vector(x0 - 2, y0 + 2), c);
            plot(bitmap, Vector(x0 + 2, y0 + 2), c);
        }
    }
    void plot(Bitmap<DWORD> bitmap, Vector p, DWord c)
    {
        if (p.inside(bitmap.size()))
            bitmap[p] = c;
    }
    bool operator<(const Particle& other) const { return _position.z > other._position.z; }
    void transform(double* matrix)
    {
        Vector3<double> c = (_colour - Vector3<double>(128.0, 128.0, 128.0))/128.0;
        _position.x = c.x*matrix[0] + c.y*matrix[1] + c.z*matrix[2];
        _position.y = c.x*matrix[3] + c.y*matrix[4] + c.z*matrix[5];
        _position.z = c.x*matrix[6] + c.y*matrix[7] + c.z*matrix[8];
    }

    Colour _colour;
    Vector3<double> _position;
    bool _big;
};

class GamutWindow : public BitmapWindow
{
public:
    GamutWindow()
      : _lButton(false), _rButton(false), _rPosition(1000, 1000), _particle(0),
        _delta(0, 0)
    {
        _matrix[0] = 1; _matrix[1] = 0; _matrix[2] = 0;
        _matrix[3] = 0; _matrix[4] = 1; _matrix[5] = 0;
        _matrix[6] = 0; _matrix[7] = 0; _matrix[8] = 1;
        setSize(Vector(640, 480));
    }
    void create()
    {
        BitmapWindow::create();
        reset();
        draw();
        invalidate();
    }
    void setDecoder(NTSCDecoder* decoder) { _decoder = decoder; }
    void setAnimated(AnimatedWindow* animated) { _animated = animated; }
    void paint()
    {
        _animated->restart();
    }
    void draw()
    {
        if (_delta.modulus2() >= 0.000001)
            animate();
        else
            _animated->stop();
        if (!_bitmap.valid())
            _bitmap = Bitmap<DWORD>(Vector(640, 480));
        _bitmap.fill(0);
        for (int i = 0; i < _particle; ++i)
            _particles[i].transform(_matrix);
        std::sort(&_particles[0], &_particles[_particle]);
        for (int i = 0; i < _particle; ++i)
            _particles[i].plot(_bitmap, _rPosition);
        _bitmap = setNextBitmap(_bitmap);
    }
    void line(Colour c1, Colour c2)
    {
        for (int i = 0; i < 101; ++i)
            add(c1*(i*0.01) + c2*((100-i)*0.01), false);
    }
    void reset()
    {
        _particle = 0;
        line(Colour(0, 0, 0), Colour(255, 0, 0));
        line(Colour(0, 0, 0), Colour(0, 255, 0));
        line(Colour(0, 0, 0), Colour(0, 0, 255));
        line(Colour(255, 0, 0), Colour(255, 255, 0));
        line(Colour(255, 0, 0), Colour(255, 0, 255));
        line(Colour(0, 255, 0), Colour(255, 255, 0));
        line(Colour(0, 255, 0), Colour(0, 255, 255));
        line(Colour(0, 0, 255), Colour(0, 255, 255));
        line(Colour(0, 0, 255), Colour(255, 0, 255));
        line(Colour(255, 255, 0), Colour(255, 255, 255));
        line(Colour(255, 0, 255), Colour(255, 255, 255));
        line(Colour(0, 255, 255), Colour(255, 255, 255));
    }
    void add(Colour c, bool big = true)
    {
        Particle p;
        p._colour = c;
        p._big = big;
        if (_particle >= _particles.count())
            _particles.append(p);
        else
            _particles[_particle] = p;
        ++_particle;
    }
    bool mouseInput(Vector position, int buttons)
    {
        bool oldLButton = _lButton;
        bool mouseDown = false;
        if ((buttons & MK_LBUTTON) != 0 && !_lButton) {
            _lButton = true;
            mouseDown = true;
            _lastPosition = position;
        }
        if ((buttons & MK_LBUTTON) == 0)
            _lButton = false;
        if ((buttons & MK_RBUTTON) != 0 && !_rButton) {
            _rButton = true;
            mouseDown = true;
            _lastPosition = position;
        }
        if ((buttons & MK_RBUTTON) == 0)
            _rButton = false;
        if (_lButton) {
            _delta = Vector2Cast<double>(position - _lastPosition)*0.01;
            if (position != _lastPosition)
                update();
            _lastPosition = position;
        }
        else
            if (oldLButton && _delta.modulus2() >= 0.000001)
                _animated->start();
        if (_rButton && position != _lastPosition) {
            _rPosition += (position - _lastPosition);
            _lastPosition = position;
        }

        return mouseDown;
    }
    void animate()
    {
        _rotor =
            Rotor3<double>::yz(-_delta.y)*Rotor3<double>::zx(_delta.x)*_rotor;
        _rotor.toMatrix(_matrix);
        _delta *= 0.95;
    }
    void update()
    {
        animate();
        invalidate();
    }

    Rotor3<double> _rotor;
    double _matrix[9];
    AppendableArray<Particle> _particles;
    Vector _lastPosition;
    Vector _rPosition;
    bool _lButton;
    bool _rButton;
    NTSCDecoder* _decoder;
    Vector2<double> _delta;
    AnimatedWindow* _animated;
    Bitmap<DWORD> _bitmap;

    int _particle;
};

class CGA2NTSCWindow;

template<class T> class NumericSliderWindowT : public Slider
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void setPositionAndSize(Vector position, Vector size)
    {
        Slider::setSize(size);
        Slider::setPosition(position);
        _caption.setPosition(bottomLeft() + Vector(0, 15));
        _text.setPosition(_caption.topRight());
    }
    Vector bottomLeft() { return _caption.bottomLeft(); }
    void valueSet(double value)
    {
        double v = valueFromPosition(value);
        _text.setText(format("%f", v));
        _text.size();
        valueSet2(v);
    }
    void setRange(double low, double high)
    {
        Slider::setRange(positionFromValue(low), positionFromValue(high));
    }
    void setValue(double value) { Slider::setValue(positionFromValue(value)); }
    double getValue() { return valueFromPosition(Slider::getValue()); }

protected:
    virtual void valueSet2(double value) { }
    virtual double positionFromValue(double value) { return value; }
    virtual double valueFromPosition(double position) { return position; }

    CGA2NTSCWindow* _host;
    TextWindow _caption;
private:
    TextWindow _text;
};
typedef NumericSliderWindowT<void> NumericSliderWindow;

template<class T> class BrightnessSliderWindowT : public NumericSliderWindow
{
public:
    void valueSet2(double value) { _host->brightnessSet(value); }
    void create()
    {
        _caption.setText("Brightness: ");
        setRange(-2, 2);
        NumericSliderWindow::create();
    }
};
typedef BrightnessSliderWindowT<void> BrightnessSliderWindow;

template<class T> class SaturationSliderWindowT : public NumericSliderWindow
{
public:
    void valueSet2(double value) { _host->saturationSet(value); }
    void create()
    {
        _caption.setText("Saturation: ");
        setRange(0, 4);
        NumericSliderWindow::create();
    }
};
typedef SaturationSliderWindowT<void> SaturationSliderWindow;

template<class T> class ContrastSliderWindowT: public NumericSliderWindow
{
public:
    void valueSet2(double value) { _host->contrastSet(value); }
    void create()
    {
        _caption.setText("Contrast: ");
        setRange(0, 4);
        NumericSliderWindow::create();
    }
};
typedef ContrastSliderWindowT<void> ContrastSliderWindow;

template<class T> class HueSliderWindowT : public NumericSliderWindow
{
public:
    void valueSet2(double value) { _host->hueSet(value); }
    void create()
    {
        _caption.setText("Hue: ");
        setRange(-180, 180);
        NumericSliderWindow::create();
    }
};
typedef HueSliderWindowT<void> HueSliderWindow;

template<class T> class SharpnessSliderWindowT : public NumericSliderWindow
{
public:
    void valueSet2(double value) { _host->sharpnessSet(value); }
    void create()
    {
        _caption.setText("Sharpness: ");
        setRange(0, 1);
        NumericSliderWindow::create();
    }
};
typedef SharpnessSliderWindowT<void> SharpnessSliderWindow;

template<class T> class AutoBrightnessButtonWindowT
  : public ToggleButton
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void clicked() { _host->autoBrightnessPressed(); }
    void create()
    {
        setText("Auto");
        ToggleButton::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef AutoBrightnessButtonWindowT<void> AutoBrightnessButtonWindow;

template<class T> class AutoContrastClipButtonWindowT
  : public ToggleButton
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void clicked() { _host->autoContrastClipPressed(); }
    void create()
    {
        setText("No clipping");
        ToggleButton::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef AutoContrastClipButtonWindowT<void>
    AutoContrastClipButtonWindow;

template<class T> class AutoSaturationButtonWindowT
  : public ToggleButton
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void clicked() { _host->autoSaturationPressed(); }
    void create()
    {
        setText("Auto");
        ToggleButton::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef AutoSaturationButtonWindowT<void> AutoSaturationButtonWindow;

template<class T> class AutoContrastMonoButtonWindowT
  : public ToggleButton
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void clicked() { _host->autoContrastMonoPressed(); }
    void create()
    {
        setText("Fix black and white");
        ToggleButton::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef AutoContrastMonoButtonWindowT<void>
    AutoContrastMonoButtonWindow;

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

template<class T> class FixPrimariesButtonWindowT : public ToggleButton
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void clicked() { _host->fixPrimariesPressed(); }
    void create()
    {
        setText("Fix Primaries");
        ToggleButton::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef FixPrimariesButtonWindowT<void> FixPrimariesButtonWindow;

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
    void valueSet2(double value) { _host->diffusionHorizontalSet(value); }
    void create()
    {
        _caption.setText("Horizontal diffusion: ");
        setRange(0, 1);
        NumericSliderWindow::create();
    }
};
typedef DiffusionHorizontalSliderWindowT<void> DiffusionHorizontalSliderWindow;

template<class T> class DiffusionVerticalSliderWindowT
  : public NumericSliderWindow
{
public:
    void valueSet2(double value) { _host->diffusionVerticalSet(value); }
    void create()
    {
        _caption.setText("Vertical diffusion: ");
        setRange(0, 1);
        NumericSliderWindow::create();
    }
};
typedef DiffusionVerticalSliderWindowT<void> DiffusionVerticalSliderWindow;

template<class T> class DiffusionTemporalSliderWindowT
  : public NumericSliderWindow
{
public:
    void valueSet2(double value) { _host->diffusionTemporalSet(value); }
    void create()
    {
        _caption.setText("Temporal diffusion: ");
        setRange(0, 1);
        NumericSliderWindow::create();
    }
};
typedef DiffusionTemporalSliderWindowT<void> DiffusionTemporalSliderWindow;

template<class T> class QualitySliderWindowT : public NumericSliderWindow
{
public:
    void valueSet2(double value) { _host->qualitySet(value); }
    void create()
    {
        _caption.setText("Quality: ");
        setRange(0, 1);
        NumericSliderWindow::create();
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
    void valueSet2(double value) { _host->scanlineWidthSet(value); }
    void create()
    {
        _caption.setText("Scanline width: ");
        setRange(0, 1);
        NumericSliderWindow::create();
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
    void valueSet2(double value) { _host->scanlineOffsetSet(value); }
    void create()
    {
        _caption.setText("Scanline offset: ");
        setRange(0, 1);
        NumericSliderWindow::create();
    }
};
typedef ScanlineOffsetSliderWindowT<void> ScanlineOffsetSliderWindow;

template<class T> class ZoomSliderWindowT : public NumericSliderWindow
{
public:
    void valueSet2(double value) { _host->scanlineOffsetSet(value); }
    void create()
    {
        _caption.setText("Zoom: ");
        setRange(1, 10);
        NumericSliderWindow::create();
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
    void valueSet2(double value) { _host->aspectRatioSet(value); }
    void create()
    {
        _caption.setText("Aspect Ratio: ");
        setRange(1, 2);
        NumericSliderWindow::create();
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
    void setDecoder(NTSCDecoder* decoder) { _decoder = decoder; }
    void setRGBI(Bitmap<Byte> rgbi)
    {
        _rgbi = rgbi;
        _ntsc = Bitmap<Byte>(_rgbi.size() - Vector(1, 0));
        setSize(Vector(_ntsc.size().x - 6, _ntsc.size().y*2));
        reCreateNTSC();
    }
    void reCreateNTSC()
    {
        _composite.initChroma();
        Byte burst[4];
        for (int i = 0; i < 4; ++i)
            burst[i] = _composite.simulateCGA(6, 6, i);
        _decoder->calculateBurst(burst);
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
    }
    void draw()
    {
        if (!_bitmap.valid())
            _bitmap =
                Bitmap<DWORD>(Vector(_ntsc.size().x - 6, _ntsc.size().y*2));
        const Byte* ntscRow = _ntsc.data();
        Byte* outputRow = _bitmap.data();
        for (int yy = 0; yy < _ntsc.size().y; ++yy) {
            const Byte* n = ntscRow;
            DWORD* outputPixel = reinterpret_cast<DWORD*>(outputRow);
            DWORD* outputPixel2 =
                reinterpret_cast<DWORD*>(outputRow + _bitmap.stride());
            for (int x = 0; x < _ntsc.size().x - 6; ++x) {
                Colour s = _decoder->decode(n, (x + 1) & 3);
                ++n;
                DWORD d = (byteClamp(s.x) << 16) | (byteClamp(s.y) << 8) |
                    byteClamp(s.z);
                *outputPixel = d;
                ++outputPixel;
                *outputPixel2 = d;
                ++outputPixel2;
            }
            outputRow += _bitmap.stride()*2;
            ntscRow += _ntsc.stride();
        }
        _bitmap = setNextBitmap(_bitmap);
    }
    void save(String outputFileName)
    {
        _bitmap.save(PNGFileFormat<DWORD>(), File(outputFileName, true));

        FileStream s = File(outputFileName + ".ntsc", true).openWrite();
        s.write(_ntsc.data(), _ntsc.stride()*_ntsc.size().y);
    }
    void setNewCGA(bool newCGA) { _composite.setNewCGA(newCGA); }
    void setBW(bool bw) { _composite.setBW(bw); }
    void setScanlineWidth(double width) { _scanlineWidth = width; }
    void setScanlineProfile(int profile) { _scanlineProfile = profile; }
    void setScanlineOffset(double offset) { _scanlineOffset = offset; }
    void setZoom(double zoom) { _zoom = zoom; }
    void setScanlineBleeding(bool bleeding) { _scanlineBleeding = bleeding; }
    void setAspectRatio(double ratio) { _aspectRatio = ratio; }
    void setCombFilterVertical(int value) { _combFilterVertical = value; }
    void setCombFilterTemporal(int value) { _combFilterTemporal = value; }

private:
    Bitmap<DWORD> _bitmap;
    Bitmap<Byte> _rgbi;
    Bitmap<Byte> _ntsc;
    CGAComposite _composite;
    NTSCDecoder* _decoder;
    double _scanlineWidth;
    int _scanlineProfile;
    double _scanlineOffset;
    double _zoom;
    bool _scanlineBleeding;
    double _aspectRatio;
    int _combFilterVertical;
    int _combFilterTemporal;
};

class CGA2NTSCWindow : public RootWindow
{
public:
    CGA2NTSCWindow()
      : _autoBrightnessFlag(false), _autoSaturationFlag(false),
        _autoContrastClipFlag(false), _autoContrastMonoFlag(false),
        _updating(false) { }
    void setWindows(Windows* windows)
    {
        add(&_output);
        add2(&_brightness);
        add2(&_autoBrightness);
        add2(&_saturation);
        add2(&_autoSaturation);
        add2(&_contrast);
        add2(&_autoContrastClip);
        add2(&_autoContrastMono);
        add2(&_hue);
        add2(&_sharpness);
        add(&_blackText);
        add(&_whiteText);
        add(&_mostSaturatedText);
        add(&_clippedColoursText);
        add(&_gamut);
        add2(&_newCGA);
        add2(&_fixPrimaries);
        add(&_animated);
        add2(&_matchMode);
        add2(&_mode);
        add2(&_background);
        add2(&_palette);
        add2(&_scanlinesPerRow);
        add2(&_scanlinesRepeat);
        add2(&_diffusionHorizontal);
        add2(&_diffusionVertical);
        add2(&_diffusionTemporal);
        add2(&_quality);
        add2(&_bwCheckBox);
        add2(&_blinkCheckBox);
        add2(&_phaseCheckBox);
        add2(&_interlaceCombo);
        add2(&_characterSetCombo);
        add2(&_scanlineWidth);
        add2(&_scanlineProfile);
        add2(&_scanlineOffset);
        add2(&_zoom);
        add2(&_scanlineBleeding);
        add2(&_aspectRatio);
        add2(&_combFilterVertical);
        add2(&_combFilterTemporal);
        RootWindow::setWindows(windows);
    }
    void create()
    {
        _animated.setDrawWindow(&_gamut);
        _gamut.setAnimated(&_animated);

        setText("CGA to NTSC");
        setSize(Vector(640, 480));

        RootWindow::create();

        sizeSet(size());
        setSize(Vector(_brightness.right() + 20, _gamut.bottom() + 20));

        setBrightness(_decoder->_brightness);
        setSaturation(_decoder->_saturation);
        setHue(_decoder->_hue);
        setContrast(_decoder->_contrast);
        setSharpness(_decoder->_sharpness);
        int mode = _matcher->getMode();
            //_blink = ((mode & 0x20) != 0);
            //_bw = ((mode & 4) != 0);
        if ((mode & 0x80) != 0)
            setMode(8 + (mode & 1));
        else {
            switch (mode & 0x13) {
                case 0: setMode(0); break;
                case 1: setMode(1); break;
                case 2: setMode(3); break;
                case 3: setMode(7); break;
                case 0x10: setMode(4); break;
                case 0x11: setMode(5); break;
                case 0x12: setMode(2); break;
                case 0x13: setMode(6); break;
            }
        }
        setBW((mode & 4) != 0);
        setBlink((mode & 0x20) != 0);
        setPhase(_matcher->getPhase() == 0);
        setInterlace(_matcher->getInterlace());
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
        setDiffusionHorizontal(_matcher->getDiffusionHorizontal());
        setDiffusionVertical(_matcher->getDiffusionVertical());
        setDiffusionTemporal(_matcher->getDiffusionTemporal());
        setQuality(_matcher->getQuality());
        setCharacterSet(_matcher->getCharacterSet());
        setScanlinesPerRow(_matcher->getScanlinesPerRow() - 1);
        setScanlinesRepeat(_matcher->getScanlinesRepeat() - 1);

        update();
        uiUpdate();
    }
    void sizeSet(Vector size)
    {
        _output.setPosition(Vector(20, 20));
        int w = max(_output.right(), _gamut.right()) + 20;

        _gamut.setPosition(Vector(20, _output.bottom() + 20));

        Vector vSpace(0, 15);

        _brightness.setPositionAndSize(Vector(w, 20), Vector(301, 24));
        _autoBrightness.setPosition(_brightness.bottomLeft() + vSpace);

        _saturation.setPositionAndSize(_autoBrightness.bottomLeft() + 2*vSpace,
            Vector(301, 24));
        _autoSaturation.setPosition(_saturation.bottomLeft() + vSpace);

        _contrast.setPositionAndSize(_autoSaturation.bottomLeft() + 2*vSpace,
            Vector(301, 24));
        _autoContrastClip.setPosition(_contrast.bottomLeft() + vSpace);
        _autoContrastMono.setPosition(_autoContrastClip.topRight() +
            Vector(20, 0));

        _hue.setPositionAndSize(_autoContrastClip.bottomLeft() + 2*vSpace,
            Vector(301, 24));

        _sharpness.setPositionAndSize(_hue.bottomLeft() + 2*vSpace,
            Vector(301, 24));

        _newCGA.setPosition(_sharpness.bottomLeft() + 2*vSpace);
        _fixPrimaries.setPosition(_newCGA.topRight() + Vector(20, 0));

        _blackText.setPosition(_newCGA.bottomLeft() + 2*vSpace);
        _whiteText.setPosition(_blackText.bottomLeft());
        _mostSaturatedText.setPosition(_whiteText.bottomLeft());
        _clippedColoursText.setPosition(_mostSaturatedText.bottomLeft());

        _matchMode.setPosition(_clippedColoursText.bottomLeft() + 2*vSpace);
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
    void setDecoder(NTSCDecoder* decoder)
    {
        _decoder = decoder;
        _output.setDecoder(decoder);
        _gamut.setDecoder(decoder);
    }
    void setMatcher(CGAMatcher* matcher)
    {
        _output.setRGBI(matcher->_rgbi);
        _matcher = matcher;
    }
    void setShower(CGAShower* shower)
    {
        _shower = shower;
    }
    void uiUpdate()
    {
        _updating = true;
        _whiteText.setText(format("White level: %f", _white));
        _whiteText.size();
        _blackText.setText(format("Black level: %f", _black));
        _blackText.size();
        _mostSaturatedText.setText(
            format("Most saturated: %f", _maxSaturation));
        _mostSaturatedText.size();
        _clippedColoursText.setText(format("%i colours clipped", _clips));
        _clippedColoursText.size();
        _output.draw();
        _output.invalidate();
        _gamut.invalidate();
        _saturation.setValue(_decoder->_saturation);
        _contrast.setValue(_decoder->_contrast);
        _updating = false;
    }
    Colour colourFromSeq(UInt64 seq)
    {
        Byte ntsc[7];
        int phase = (seq >> 32) & 3;
        for (int x = 0; x < 7; ++x) {
            ntsc[x] = _composite.simulateCGA(seq & 15, (seq >> 4) & 15,
                (x + phase) & 3);
            seq >>= 4;
        }
        return _decoder->decode(ntsc, phase);
    }
    void update()
    {
        Byte burst[4];
        for (int i = 0; i < 4; ++i)
            burst[i] = _composite.simulateCGA(6, 6, i);
        _decoder->calculateBurst(burst);
        int s[4];
        _composite.decode(0, s);
        Colour black = _decoder->decode(s);
        _black = 0.299*black.x + 0.587*black.y + 0.114*black.z;
        _composite.decode(0xffff, s);
        Colour white = _decoder->decode(s);
        _white = 0.299*white.x + 0.587*white.y + 0.114*white.z;
        _clips = 0;
        _maxSaturation = 0;
        _gamut.reset();
        for (auto i : _colours) {
            Colour c = colourFromSeq(i);
            double r = c.x;
            double g = c.y;
            double b = c.z;
            if (r < 0 || r >= 256 || g < 0 || g >= 256 || b < 0 || b >= 256) {
                ++_clips;
                _clipped = i;
            }
            double y = 0.299*r + 0.587*g + 0.114*b;
            _maxSaturation =
                max(_maxSaturation, (c - Colour(y, y, y)).modulus());
            _gamut.add(c);
        }
        _gamut.draw();
        _gamut.invalidate();
    }
    void beginConvert()
    {
        if (!_matchMode.checked())
            _shower->convert();
        else
            _matcher->convert();
    }

    void modeSet(int value)
    {
        static const int modes[8] = {0, 1, 0x12, 2, 0x10, 0x11, 0x13, 3};
        int mode = modes[value] | 8 | (_bwCheckBox.checked() ? 4 : 0) |
            (_blinkCheckBox.checked() ? 0x20 : 0);
        _matcher->setMode(mode);
        beginConvert();
    }
    void setMode(int value) { _mode.set(value); }

    void backgroundSet(int value)
    {
        _backgroundSelected = value;
        setPaletteAndBackground();
        beginConvert();
    }

    void paletteSet(int value)
    {
        _paletteSelected = value;
        setPaletteAndBackground();
        beginConvert();
    }
    void setPaletteAndBackground()
    {
        if (_backgroundSelected == 0x10)
            _matcher->setPalette(0xff);
        else {
            _matcher->setPalette(
                _backgroundSelected + (_paletteSelected << 4));
        }
    }

    void scanlinesPerRowSet(int value)
    {
        _matcher->setScanlinesPerRow(value + 1);
        beginConvert();
    }
    void setScanlinesPerRow(int value) { _scanlinesPerRow.set(value); }

    void scanlinesRepeatSet(int value)
    {
        _matcher->setScanlinesRepeat(value + 1);
        beginConvert();
    }
    void setScanlinesRepeat(int value) { _scanlinesRepeat.set(value); }

    void setDiffusionHorizontal(double value)
    {
        _diffusionHorizontal.setValue(value);
    }
    void diffusionHorizontalSet(double value)
    {
        _matcher->setDiffusionHorizontal(value);
        beginConvert();
    }

    void setDiffusionVertical(double value)
    {
        _diffusionVertical.setValue(value);
    }
    void diffusionVerticalSet(double value)
    {
        _matcher->setDiffusionVertical(value);
        beginConvert();
    }

    void setDiffusionTemporal(double value)
    {
        _diffusionTemporal.setValue(value);
    }
    void diffusionTemporalSet(double value)
    {
        _matcher->setDiffusionTemporal(value);
        beginConvert();
    }

    void setQuality(double value) { _quality.setValue(value); }
    void qualitySet(double value)
    {
        _matcher->setQuality(value);
        beginConvert();
    }

    void setScanlineWidth(double value) { _scanlineWidth.setValue(value); }
    void scanlineWidthSet(double value) { _output.setScanlineWidth(value); }

    void setScanlineProfile(int value) { _scanlineProfile.set(value); }
    void scanlineProfileSet(int value) { _output.setScanlineProfile(value); }

    void setScanlineOffset(double value) { _scanlineOffset.setValue(value); }
    void scanlineOffsetSet(double value) { _output.setScanlineOffset(value); }

    void setZoom(double value) { _zoom.setValue(value); }
    void zoomSet(double value) { _output.setZoom(value); }

    void scanlineBleedingPressed()
    {
        _output.setScanlineBleeding(_scanlineBleeding.checked() ? 0 : 1);
    }
    void setScanlineBleeding(bool bleeding)
    {
        _scanlineBleeding.setCheckState(bleeding);
    }

    void setAspectRatio(double value) { _aspectRatio.setValue(value); }
    void aspectRatioSet(double value) { _output.setAspectRatio(value); }

    void setCombFilterVertical(int value) { _combFilterVertical.set(value); }
    void combFilterVerticalSet(int value)
    {
        _output.setCombFilterVertical(value);
    }

    void setCombFilterTemporal(int value) { _combFilterTemporal.set(value); }
    void combFilterTemporalSet(int value)
    {
        _output.setCombFilterTemporal(value);
    }


    void bwPressed()
    {
        bool bw = _bwCheckBox.checked();
        _matcher->setMode((_matcher->getMode() & ~4) | (bw ? 4 : 0));
        _output.setBW(bw);
        _composite.setBW(bw);
        beginConvert();
    }
    void setBW(bool bw) { _bwCheckBox.setCheckState(bw); }

    void blinkPressed()
    {
        bool blink = _blinkCheckBox.checked();
        int mode = _matcher->getMode();
        _matcher->setMode((mode & ~0x20) | (blink ? 0x20 : 0));
        if ((mode & 2) == 0)
            beginConvert();
    }
    void setBlink(bool blink) { _blinkCheckBox.setCheckState(blink); }

    void phasePressed()
    {
        _matcher->setPhase(_phaseCheckBox.checked() ? 0 : 1);
    }
    void setPhase(bool phase) { _phaseCheckBox.setCheckState(phase); }

    void interlaceSet(int value)
    {
        _matcher->setInterlace(value);
        beginConvert();
    }
    void setInterlace(int value) { _interlaceCombo.set(value); }

    void characterSetSet(int value)
    {
        _matcher->setCharacterSet(value);
        beginConvert();
    }
    void setCharacterSet(int value) { _characterSetCombo.set(value); }

    void matchModePressed()
    {
        beginConvert();
        reCreateNTSC();
    }
    void setMatchMode(bool matchMode) { _matchMode.setCheckState(matchMode); }

    void allAutos()
    {
        autoSaturation();
        autoContrastClip();
        autoContrastMono();
        autoBrightness();
    }

    void brightnessSet(double brightness)
    {
        _decoder->_brightness = brightness;
        if (!_updating) {
            update();
            uiUpdate();
        }
    }
    void setBrightness(double brightness) { _brightness.setValue(brightness); }

    void saturationSet(double saturation)
    {
        _decoder->_saturation = saturation;
        if (!_updating) {
            update();
            autoContrastClip();
            uiUpdate();
        }
    }
    void setSaturation(double saturation) { _saturation.setValue(saturation); }

    void contrastSet(double contrast)
    {
        _decoder->_contrast = contrast;
        if (!_updating) {
            update();
            autoBrightness();
            autoSaturation();
            uiUpdate();
        }
    }
    void setContrast(double contrast) { _contrast.setValue(contrast); }

    void hueSet(double hue)
    {
        _decoder->_hue = hue;
        if (!_updating) {
            update();
            allAutos();
            uiUpdate();
        }
    }
    void setHue(double hue) { _hue.setValue(hue); }

    void sharpnessSet(double sharpness)
    {
        _decoder->_sharpness = sharpness;
        if (!_updating) {
            update();
            uiUpdate();
        }
    }
    void setSharpness(double sharpness) { _sharpness.setValue(sharpness); }

    void autoContrastClipPressed()
    {
        _autoContrastClipFlag = _autoContrastClip.checked();
        if (_autoContrastClipFlag) {
            _autoContrastMono.uncheck();
            _autoContrastMonoFlag = false;
        }
        autoContrastClip();
        uiUpdate();
    }
    void autoContrastMonoPressed()
    {
        _autoContrastMonoFlag = _autoContrastMono.checked();
        if (_autoContrastMonoFlag) {
            _autoContrastClip.uncheck();
            _autoContrastClipFlag = false;
        }
        autoContrastMono();
        autoBrightness();
        uiUpdate();
    }
    void autoBrightnessPressed()
    {
        _autoBrightnessFlag = _autoBrightness.checked();
        autoContrastMono();
        autoBrightness();
        uiUpdate();
    }
    void autoSaturationPressed()
    {
        _autoSaturationFlag = _autoSaturation.checked();
        autoSaturation();
        uiUpdate();
    }
    void reCreateNTSC()
    {
        _output.reCreateNTSC();
        update();
        allAutos();
        uiUpdate();
    }
    void newCGAPressed()
    {
        bool newCGA = _newCGA.checked();
        _output.setNewCGA(newCGA);
        _matcher->setNewCGA(newCGA);
        _composite.setNewCGA(newCGA);
        reCreateNTSC();
    }
    void setNewCGA(bool newCGA) { _newCGA.setCheckState(newCGA); }

    void fixPrimariesPressed()
    {
        _decoder->_fixPrimaries = _fixPrimaries.checked();
        update();
        allAutos();
        uiUpdate();
    }

    void autoBrightness(bool force = false)
    {
        if (!false && !_autoBrightnessFlag)
            return;
        setBrightness(_brightness.getValue() + (256 - (_black + _white))/512);
        update();
    }
    void autoSaturation()
    {
        if (!_autoSaturationFlag)
            return;
        _decoder->_saturation *=
            sqrt(3.0)*(_white - _black)/(2*_maxSaturation);
        update();
    }
    void autoContrastClip()
    {
        if (!_autoContrastClipFlag)
            return;
        double minContrast = 0;
        double maxContrast = 2;
        do {
            double contrast = (maxContrast + minContrast)/2;
            _decoder->_contrast = contrast;
            update();
            autoBrightness();
            if (_clips == 1 || (maxContrast - minContrast) < 0.000001)
                break;
            else
                if (_clips == 0)
                    minContrast = contrast;
                else
                    maxContrast = contrast;
        } while (true);
        double midPoint = (_white + _black)/2;
        double fudge = 0.99999;
        for (int i = 0; i < 3; ++i) {
            Colour c = colourFromSeq(_clipped);
            double r = c.x;
            double g = c.y;
            double b = c.z;
            bool found = false;
            if (r < 0) {
                _decoder->_contrast *= fudge*midPoint/(midPoint - r);
                found = true;
            }
            if (!found && r >= 256) {
                _decoder->_contrast *= fudge*midPoint/(r - midPoint);
                found = true;
            }
            if (!found && g < 0) {
                _decoder->_contrast *= fudge*midPoint/(midPoint - g);
                found = true;
            }
            if (!found && g >= 256) {
                _decoder->_contrast *= fudge*midPoint/(g - midPoint);
                found = true;
            }
            if (!found && b < 0) {
                _decoder->_contrast *= fudge*midPoint/(midPoint - b);
                found = true;
            }
            if (!found && b >= 256)
                _decoder->_contrast *= fudge*midPoint/(b - midPoint);
            update();
            autoBrightness();
            autoSaturation();
            if (_clips == 0)
                break;
        }
    }
    void autoContrastMono()
    {
        if (!_autoContrastMonoFlag)
            return;
        _decoder->_contrast *= 256/(_white - _black);
        update();
    }
    void save(String outputFileName) { _output.save(outputFileName); }
    void resetColours() { _colours = Set<UInt64>(); }
    void addColour(UInt64 seq) { _colours.add(seq); }

private:
    template<class T> void add2(T* p)
    {
        add(p);
        p->setHost(this);
    }

    AnimatedWindow _animated;
    OutputWindow _output;
    BrightnessSliderWindow _brightness;
    AutoBrightnessButtonWindow _autoBrightness;
    SaturationSliderWindow _saturation;
    AutoSaturationButtonWindow _autoSaturation;
    ContrastSliderWindow _contrast;
    AutoContrastClipButtonWindow _autoContrastClip;
    AutoContrastMonoButtonWindow _autoContrastMono;
    HueSliderWindow _hue;
    SharpnessSliderWindow _sharpness;
    TextWindow _blackText;
    TextWindow _whiteText;
    TextWindow _mostSaturatedText;
    TextWindow _clippedColoursText;
    GamutWindow _gamut;
    NewCGAButtonWindow _newCGA;
    FixPrimariesButtonWindow _fixPrimaries;
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
    CGAComposite _composite;
    NTSCDecoder* _decoder;
    double _black;
    double _white;
    Set<UInt64> _colours;
    int _clips;
    double _maxSaturation;
    UInt64 _clipped;
    bool _autoBrightnessFlag;
    bool _autoSaturationFlag;
    bool _autoContrastClipFlag;
    bool _autoContrastMonoFlag;
    bool _updating;
    int _paletteSelected;
    int _backgroundSelected;
};

class BitmapType : public NamedNullary<Type, BitmapType>
{
public:
    static String name() { return "Bitmap"; }
    class Body : public NamedNullary<Type, BitmapType>::Body
    {
    public:
        bool canConvertFrom(const Type& from, String* reason) const
        {
            return from == StringType();
        }
        Value convert(const Value& value) const
        {
            return Value(BitmapType(), value.value(), value.span());
        }
        Value defaultValue() const
        {
            return Value(BitmapType(), Any(), Span());
        }
    };
};

class BitmapIsRGBIFunction : public Nullary<Function, BitmapIsRGBIFunction>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto s = arguments.begin()->value<String>();
            Bitmap<SRGB> input = PNGFileFormat<SRGB>().load(File(s, true));
            Vector size = input.size();

            int maxDistance = 0;
            const Byte* inputRow = input.data();

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
                inputRow += input.stride();
            }

            return Value(maxDistance < 15*15*3);
        }
        Identifier identifier() const { return "bitmapIsRGBI"; }
        FunctionType type() const
        {
            return FunctionType(BooleanType(), BitmapType());
        }
    };
};

class Program : public WindowProgram<CGA2NTSCWindow>
{
public:
    void run()
    {
        ConfigFile configFile;
        configFile.addOption("inputPicture", BitmapType());
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
        configFile.addDefaultOption("sharpness", 0.0);
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

        configFile.addFunco(BitmapIsRGBIFunction());

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

        configFile.load(configPath);
        bool matchMode = configFile.get<bool>("matchMode");


        NTSCDecoder decoder;
        decoder._fixPrimaries = configFile.get<bool>("ntscPrimaries");
        decoder._brightness = configFile.get<double>("brightness");
        decoder._hue = configFile.get<double>("hue");
        decoder._contrast = configFile.get<double>("contrast");
        decoder._saturation = configFile.get<double>("saturation");
        _matcher.setDiffusionHorizontal(
            configFile.get<double>("horizontalDiffusion"));
        _matcher.setDiffusionVertical(
            configFile.get<double>("verticalDiffusion"));
        _matcher.setDiffusionTemporal(
            configFile.get<double>("temporalDiffusion"));
        _matcher.setQuality(configFile.get<double>("quality"));
        _matcher.setInterlace(configFile.get<int>("interlaceMode"));
        _matcher.setCharacterSet(configFile.get<int>("characterSet"));
        //Byte burst[4];
        //for (int i = 0; i < 4; ++i)
        //    burst[i] = composite.simulateCGA(6, 6, i);
        //decoder.calculateBurst(burst);
        _matcher.setDecoder(&decoder);
        _matcher.setMode(configFile.get<int>("mode"));
        _matcher.setPalette(configFile.get<int>("palette"));
        _matcher.setScanlinesPerRow(configFile.get<int>("scanlinesPerRow"));
        _matcher.setScanlinesRepeat(configFile.get<int>("scanlinesRepeat"));
        _matcher.setROM(configFile.get<String>("cgaROM"));
        _window.setScanlineWidth(configFile.get<double>("scanlineWidth"));
        _window.setScanlineProfile(configFile.get<int>("scanlineProfile"));
        _window.setScanlineOffset(configFile.get<double>("scanlineOffset"));
        _window.setZoom(configFile.get<double>("zoom"));
        _window.setScanlineBleeding(configFile.get<bool>("scanlineBleeding"));
        _window.setAspectRatio(configFile.get<double>("aspectRatio"));
        _window.setCombFilterVertical(
            configFile.get<int>("combFilterVertical"));
        _window.setCombFilterTemporal(
            configFile.get<int>("combFilterTemporal"));

        String inputFileName = configFile.get<String>("inputPicture");

        Bitmap<SRGB> input =
            PNGFileFormat<SRGB>().load(File(inputFileName, true));
        Bitmap<SRGB> input2 = input;
        Vector size = input.size();
        if (size.y > 262) {
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
        if (size.x <= 456) {
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

        _window.setMatchMode(matchMode);
        _matcher.setInput(input);
        _matcher.setWindow(&_window);

        _window.setDecoder(&decoder);
        _window.setMatcher(&_matcher);
        _window.setShower(&_shower);
        _window.setNewCGA(configFile.get<bool>("newCGA"));

        if (configFile.get<bool>("interactive"))
            WindowProgram::run();

        int i;
        for (i = inputFileName.length() - 1; i >= 0; --i)
            if (inputFileName[i] == '.')
                break;
        if (i != -1)
            inputFileName = inputFileName.subString(0, i);

        _window.save(inputFileName + "_out.png");
        _matcher.save(inputFileName + "_out.dat");
        _matcher.saveRGBI(inputFileName + "_out.rgbi");
        _matcher.savePalettes(inputFileName + "_out.palettes");
    }
    bool idle() { return _matcher.idle(); }
private:
    CGAMatcher _matcher;
    CGAShower _shower;
};
