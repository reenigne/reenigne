#include "alfe/main.h"
#include "alfe/bitmap_png.h"
#include "alfe/complex.h"
#include "alfe/space.h"
#include "alfe/set.h"

class CGASimulator
{
public:
    void initChroma()
    {
        static Byte chromaData[256] = {
             65, 11, 62,  6, 121, 87, 63,  6,  60,  9,120, 65,  61, 59,129,  5,
            121,  6, 58, 58, 134, 65, 62,  6,  57,  9,108, 72, 126, 72,125, 77,
             60, 98,160,  6, 113,195,194,  8,  53, 94,218, 64,  56,152,225,  5,
            118, 90,147, 56, 115,154,156,  0,  52, 92,197, 73, 107,156,213, 62,
            119, 10, 97,122, 178, 77, 60, 87, 119, 12,174,205, 119, 58,135, 88,
            185,  6, 54,158, 194, 67, 57, 87, 114, 10,101,168, 181, 67,114,160,
             64,  8,156,109, 121, 73,177,122,  58,  8,244,207,  65, 58,251,137,
            127,  5,141,156, 126, 58,144, 97,  57,  7,189,168, 106, 55,201,162,
            163,124, 62, 10, 185,159, 59,  8, 135,104,128, 80, 119,142,140,  5,
            241,141, 59, 57, 210,160, 61,  5, 137,108,103, 61, 177,140,110, 65,
             59,107,124,  4, 180,201,122,  6,  52,104,194, 77,  55,159,197,  3,
            130,128,121, 51, 174,197,123,  3,  52,100,162, 62, 101,156,171, 51,
            173, 11, 60,113, 199, 93, 58, 77, 167, 11,118,196, 132, 63,129, 74,
            255,  9, 54,195, 192, 55, 59, 74, 183, 14,103,199, 206, 74,118,154,
            153,108,156,105, 255,202,188,123, 143,107,246,203, 164,208,250,129,
            209,103,148,157, 253,195,171,120, 163,106,196,207, 245,202,249,208
        };

        static double intensity[4] = {
            0, 0.047932237386703491, 0.15110087022185326, 0.18384206667542458};

        static const double minChroma = 0.070565;
        static const double maxChroma = 0.727546;

        for (int x = 0; x < 1024; ++x) {
            int phase = x & 3;
            int right = (x >> 2) & 15;
            int left = (x >> 6) & 15;
            double c = minChroma +
                chromaData[((left & 7) << 5) | ((right & 7) << 2) | phase]*
                   (maxChroma-minChroma)/256.0;
            double i = intensity[(left >> 3) | ((right >> 2) & 2)];
            if (!_newCGA)
                _table[x] = byteClamp(static_cast<int>((c + i)*256));
            else {
                double r = intensity[((left >> 2) & 1) | ((right >> 1) & 2)];
                double g = intensity[((left >> 1) & 1) | (right & 2)];
                double b = intensity[(left & 1) | ((right << 1) & 1)];
                _table[x] = byteClamp(static_cast<int>(((c/0.72)*0.29 +
                    (i/0.28)*0.32 + (r/0.28)*0.1 + (g/0.28)*0.22 +
                    (b/0.28)*0.07)*256));
            }
        }
    }
    Byte simulateCGA(int left, int right, int phase)
    {
        return _table[((left & 15) << 6) | ((right & 15) << 2) | phase];
    }
    void simulateLine(const Byte* rgbi, Byte* ntsc, int length, int phase)
    {
        for (int x = 0; x < length; ++x) {
            phase = (phase + 1) & 3;
            int left = *rgbi;
            ++rgbi;
            int right = *rgbi;
            *ntsc = simulateCGA(left, right, phase);
            ++ntsc;
        }
    }
    void decode(int pixels, int* s)
    {
        int rgbi[4];
        rgbi[0] = pixels & 15;
        rgbi[1] = (pixels >> 4) & 15;
        rgbi[2] = (pixels >> 8) & 15;
        rgbi[3] = (pixels >> 12) & 15;
        for (int t = 0; t < 4; ++t)
            s[t] = simulateCGA(rgbi[t], rgbi[(t+1)&3], t);
    }

    bool _newCGA;
private:

    int _table[1024];
};

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

template<class T> class CGAEncoderT
{
public:
    CGAEncoderT()
      : _mode(2), _palette(3), _background(15), _characterHeight(1),
        _matchMode(false), _matchModeSet(false), _horizontalDiffusion(256),
        _verticalDiffusion(256), _skip(256)
    {
        _patterns.allocate(0x10000*8*17 + 0x100*80*5);
    }
    void setInput(Bitmap<SRGB> input)
    {
        _cgaROM = File("5788005.u33").contents();
        _input = input;
        _size = input.size();
        _rgbi = Bitmap<Byte>(_size + Vector(14, 0));
        _input2 = Bitmap<SRGB>(_size + Vector(11, 0));
        _input2.fill(SRGB(0, 0, 0));
        _input2.subBitmap(Vector(5, 0), _size).copyFrom(_input);
        _configs.allocate(_size.y);
    }
    void setWindow(CGA2NTSCWindow* window) { _window = window; }
    void beginConvert()
    {
        if (!_matchMode)
            showConvert();
        else
            matchConvert();
    }
    void showConvert()
    {
        // Convert to RGBI indexes and add left and right borders.
        int maxDistance = 0;
        const Byte* inputRow = _input.data();
        Byte* rgbiRow = _rgbi.data();
        int background = _background & 15;
        if (_mode == 2)
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
                    int distance =
                        (Vector3Cast<int>(rgbiPalette[i]) -
                        Vector3Cast<int>(s)).modulus2();
                    if (distance < bestDistance) {
                        bestDistance = distance;
                        bestRGBI = i;
                        if (distance < 42*42)
                            break;
                    }
                }
                maxDistance = max(bestDistance, maxDistance);
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
        _window->resetColours();
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
                _window->addColour(static_cast<UInt64>(seq) |
                    (static_cast<UInt64>(xx & 3) << 32));
            }
            rgbiRow += _rgbi.stride();
        }

        if (!_matchModeSet) {
            _matchMode = (maxDistance >= 15*15*3);
            _matchModeSet = true;
            if (_matchMode)
                matchConvert();
        }
        _converting = false;
    }
    static void filterHF(const Byte* input, SInt16* output, int n)
    {
        for (int x = 0; x < n; ++x)
            output[x] = (-input[x] + input[x+1]*2 + input[x+2]*6 + input[x+3]*2
                -input[x+4]);
    }
    void matchConvert()
    {
        if (_mode == 2 || _mode == 3) {
            _block.y = 1;
            for (int i = 0; i < 256; ++i)
                _skip[i] = false;
        }
        else {
            _block.y = _characterHeight;
            if (_characterHeight == 0)
                _block.y = 2;
            int lines = max(_characterHeight, 1);
            for (int i = 0; i < 256; ++i) {
                _skip[i] = false;
                if (_palette == 0) {
                    _skip[i] = (i != 0xdd);
                    continue;
                }
                if (_palette == 1) {
                    _skip[i] = (i != 0x13 && i != 0x55);
                    continue;
                }
                if (_palette == 2) {
                    _skip[i] =
                        (i != 0x13 && i != 0x55 && i != 0xb0 && i != 0xb1);
                    continue;
                }
                bool isBackground = true;
                bool isForeground = true;
                for (int y = 0; y < lines; ++y) {
                    Byte b = _cgaROM[(0x300 + i)*8 + y];
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
                        if (_cgaROM[(0x300 + i)*8 + y] !=
                            _cgaROM[(0x300 + j)*8 + y])
                            break;
                    if (y == lines)
                        break;
                }
                if (j != i)
                    _skip[i] = true;
                for (j = 0; j < i; ++j) {
                    int y;
                    for (y = 0; y < lines; ++y)
                        if (_cgaROM[(0x300 + i)*8 + y] !=
                            (_cgaROM[(0x300 + j)*8 + y]^0xff))
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

        switch (_mode) {
            case 0:
                _startConfig = 80;
                break;
            case 1:
                _startConfig = 81;
                break;
            case 2:
                _startConfig = 64 + (_background & 15);
                break;
            case 3:
                _startConfig = (_background & 15) | (_palette << 4);
                break;
        }
        _endConfig = _startConfig + 1;
        if (_background == 16) {
            if (_mode == 2) {
                _startConfig = 64;
                _endConfig = 80;
            }
            if (_mode == 3) {
                _startConfig = 0;
                _endConfig = 64;
            }
        }
        if (_mode == 4) {
            _startConfig = 0;
            _endConfig = 81;
        }

        for (_config = _startConfig; _config < _endConfig; ++_config) {
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
                    _simulator->simulateLine(&rgbi[0], &ntscTemp[0],
                        _block.x + 5, 0);
                    filterHF(&ntscTemp[0], &_patterns[_config*5*256 +
                        (pattern*_block.y + line)*w], w);
                }
            }
        }

        _rgbi.fill(_mode == 2 ? 0 : (_background & 15));
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
            case 80:
                _block.x = 16;
                _patternCount = 0x10000;
                _hdots = 16;
                break;
            case 81:
                _block.x = 8;
                _patternCount = 0x10000;
                _hdots = 8;
                break;
            default:
                _block.x = 4;
                _patternCount = 16;
                _hdots = 16;
                break;
        }
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
        UInt64 lineScore = 0;
        for (int x = 0; x < (_size.x & -_hdots); x += _block.x) {
            int bestPattern = 0;
            int bestScore = 0x7fffffff;
            bool unskipped = false;
            for (int pattern = 0; pattern < _patternCount; ++pattern) {
                if (_mode < 2) {
                    if (_skip[pattern & 0xff])
                        continue;
                    if ((pattern & 0x0f00) == ((pattern >> 4) & 0x0f00) &&
                        unskipped)
                        continue;
                    unskipped = true;
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
                        int test = _patterns[_config*5*256 +
                            (pattern*_block.y + yy)*w + xx];
                        Vector p(xx, yy);
                        int target = inputPixel[xx] +
                            (errorPixel[xx] + _testError[p])/8;
                        int d = target - test;
                        int weight = (xx == 0 || xx == _block.x ? 1 : 2);
                        score += weight*d*d;
                        int error = weight*d;
                        _testError[p + Vector(4, 0)] +=
                            (error*_horizontalDiffusion)/256;
                        _testError[p + Vector(0, 1)] +=
                            (error*_verticalDiffusion)/256;
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
            if (_mode == 0 || _mode == 1)
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
                    int test = _patterns[_config*5*256 +
                        (bestPattern*_block.y + yy)*w + xx];
                    int target = inputPixel[xx] + errorPixel[xx]/8;
                    int d = target - test;
                    int weight = (xx == 0 || xx == _block.x ? 1 : 2);
                    lineScore += weight*d*d;
                    int error = weight*d;
                    errorPixel[xx + 4] += (error*_horizontalDiffusion)/256;
                    reinterpret_cast<int*>(errorRow2 + _error.stride())[x + xx]
                        += (error*_verticalDiffusion/256);
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
            if (_mode == 4)
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
        if (_config < 64) {
            static int palettes[4] = {0, 8, 1, 9};
            for (int x = 0; x < 4; x += 2) {
                int c = _config & 15;
                int b = ((pattern >> (2 - x)) & 3);
                if (b != 0)
                    c = b + palettes[_config >> 4];
                rgbi[x] = c;
                rgbi[x + 1] = c;
            }
            return;
        }
        if (_config < 80) {
            for (int x = 0; x < 4; ++x)
                rgbi[x] = (pattern & (8 >> x)) != 0 ? (_config & 15) : 0;
            return;
        }
        Byte ch = pattern & 0xff;
        Byte at = pattern >> 8;
        if (_characterHeight == 0)
            line = 0;
        Byte b = _cgaROM[(0x300 + ch)*8 + line];
        for (int x = 0; x < 8; ++x) {
            int c = ((b & (128 >> x)) != 0 ? at : (at >> 4)) & 15;
            if (_config == 80) {
                rgbi[x*2] = c;
                rgbi[x*2 + 1] = c;
            }
            else
                rgbi[x] = c;
        }
    }

    void setSimulator(CGASimulator* simulator) { _simulator = simulator; }
    void setDecoder(NTSCDecoder* decoder) { _decoder = decoder; }

    int _mode;
    int _palette;
    int _background;
    int _characterHeight;
    bool _matchModeSet;
    bool _matchMode;
    Vector _size;
    Bitmap<SRGB> _input;
    Bitmap<Byte> _rgbi;
    CGASimulator* _simulator;
    NTSCDecoder* _decoder;
    CGA2NTSCWindow* _window;
    String _cgaROM;
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
    int _horizontalDiffusion;
    int _verticalDiffusion;
    UInt64 _configScore;
    Array<bool> _skip;
    Array<int> _configs;
    int _startConfig;
    int _endConfig;
    int _config;
    bool _testConfig;
    int _bestConfig;
};

typedef CGAEncoderT<void> CGAEncoder;

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
    void setSimulator(CGASimulator* simulator) { _simulator = simulator; }
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
    CGASimulator* _simulator;
    NTSCDecoder* _decoder;
    Vector2<double> _delta;
    AnimatedWindow* _animated;
    Bitmap<DWORD> _bitmap;

    int _particle;
};

class CGA2NTSCWindow;

template<class T> class BrightnessSliderWindowT : public Slider
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void valueSet(double value) { _host->setBrightness(value); }
    void create()
    {
        setRange(-2, 2);
        Slider::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef BrightnessSliderWindowT<void> BrightnessSliderWindow;

template<class T> class SaturationSliderWindowT : public Slider
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void valueSet(double value) { _host->setSaturation(value); }
    void create()
    {
        setRange(0, 4);
        Slider::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef SaturationSliderWindowT<void> SaturationSliderWindow;

template<class T> class ContrastSliderWindowT : public Slider
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void valueSet(double value) { _host->setContrast(value); }
    void create()
    {
        setRange(0, 4);
        Slider::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef ContrastSliderWindowT<void> ContrastSliderWindow;

template<class T> class HueSliderWindowT : public Slider
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void valueSet(double value) { _host->setHue(value); }
    void create()
    {
        setRange(-180, 180);
        Slider::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef HueSliderWindowT<void> HueSliderWindow;

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
    void changed(int value) { _host->setMode(value); }
    void create()
    {
        ComboBox::create();
        add(String("40 column text"));
        add(String("80 column text"));
        add(String("1bpp graphics"));
        add(String("2bpp graphics"));
        add(String("Auto"));
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
    void changed(int value) { _host->setBackground(value); }
    void create()
    {
        ComboBox::create();
        for (int i = 0; i < 16; ++i)
            add(decimal(i));
        add(String("Auto"));
        set(15);
        autoSize();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef BackgroundComboT<void> BackgroundCombo;

template<class T> class CharacterHeightComboT : public ComboBox
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void changed(int value) { _host->setCharacterHeight(value); }
    void create()
    {
        ComboBox::create();
        add(String("1 doubled"));
        for (int i = 1; i <= 8; ++i)
            add(decimal(i));
        set(1);
        autoSize();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef CharacterHeightComboT<void> CharacterHeightCombo;

template<class T> class PaletteComboT : public ComboBox
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void changed(int value) { _host->setPalette(value); }
    void create()
    {
        ComboBox::create();
        add(String("2/4/6"));
        add(String("10/12/14"));
        add(String("3/5/7"));
        add(String("11/13/15"));
        set(3);
        autoSize();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef PaletteComboT<void> PaletteCombo;

template<class T> class DiffusionHorizontalSliderWindowT : public Slider
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void valueSet(double value) { _host->setHorizontalDiffusion(value); }
    void create()
    {
        setRange(0, 512);
        setValue(256);
        Slider::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef DiffusionHorizontalSliderWindowT<void>
    DiffusionHorizontalSliderWindow;

template<class T> class DiffusionVerticalSliderWindowT : public Slider
{
public:
    void setHost(CGA2NTSCWindow* host) { _host = host; }
    void valueSet(double value) { _host->setVerticalDiffusion(value); }
    void create()
    {
        setRange(0, 512);
        setValue(256);
        Slider::create();
    }
private:
    CGA2NTSCWindow* _host;
};
typedef DiffusionVerticalSliderWindowT<void>
    DiffusionVerticalSliderWindow;


class OutputWindow : public BitmapWindow
{
public:
    void setSimulator(CGASimulator* simulator) { _simulator = simulator; }
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
        _simulator->initChroma();
        Byte burst[4];
        for (int i = 0; i < 4; ++i)
            burst[i] = _simulator->simulateCGA(6, 6, i);
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
                *ntscPixel = _simulator->simulateCGA(left, right, (x + 1) & 3);
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

private:
    Bitmap<DWORD> _bitmap;
    Bitmap<Byte> _rgbi;
    Bitmap<Byte> _ntsc;
    CGASimulator* _simulator;
    NTSCDecoder* _decoder;
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
        add(&_brightnessCaption);
        add(&_brightness);
        add(&_autoBrightness);
        add(&_brightnessText);
        add(&_saturationCaption);
        add(&_saturation);
        add(&_autoSaturation);
        add(&_saturationText);
        add(&_contrastCaption);
        add(&_contrast);
        add(&_contrastText);
        add(&_autoContrastClip);
        add(&_autoContrastMono);
        add(&_hueCaption);
        add(&_hue);
        add(&_hueText);
        add(&_blackText);
        add(&_whiteText);
        add(&_mostSaturatedText);
        add(&_clippedColoursText);
        add(&_gamut);
        add(&_newCGA);
        add(&_fixPrimaries);
        add(&_animated);
        add(&_matchMode);
        add(&_mode);
        add(&_background);
        add(&_palette);
        add(&_characterHeight);
        add(&_diffusionHorizontalCaption);
        add(&_diffusionHorizontal);
        add(&_diffusionHorizontalText);
        add(&_diffusionVerticalCaption);
        add(&_diffusionVertical);
        add(&_diffusionVerticalText);
        RootWindow::setWindows(windows);
    }
    void create()
    {
        _animated.setDrawWindow(&_gamut);
        _gamut.setAnimated(&_animated);

        _brightnessCaption.setText("Brightness: ");
        _saturationCaption.setText("Saturation: ");
        _contrastCaption.setText("Contrast: ");
        _hueCaption.setText("Hue: ");
        _diffusionHorizontalCaption.setText("Horizontal diffusion: ");
        _diffusionVerticalCaption.setText("Vertical diffusion: ");

        _brightness.setHost(this);
        _saturation.setHost(this);
        _contrast.setHost(this);
        _hue.setHost(this);
        _autoBrightness.setHost(this);
        _autoSaturation.setHost(this);
        _autoContrastClip.setHost(this);
        _autoContrastMono.setHost(this);
        _newCGA.setHost(this);
        _fixPrimaries.setHost(this);
        _matchMode.setHost(this);
        _mode.setHost(this);
        _background.setHost(this);
        _palette.setHost(this);
        _characterHeight.setHost(this);
        _diffusionHorizontal.setHost(this);
        _diffusionVertical.setHost(this);

        setText("CGA to NTSC");
        setSize(Vector(640, 480));

        RootWindow::create();

        sizeSet(size());
        setSize(Vector(_brightness.right() + 20, _gamut.bottom() + 20));

        _decoder->_contrast = 1;
        _decoder->_hue = 0;
        _decoder->_brightness = 0;
        _decoder->_saturation = 1;

        if (_encoder->_matchMode)
            _matchMode.check();

        update();
        uiUpdate();
    }
    void sizeSet(Vector size)
    {
        _output.setPosition(Vector(20, 20));
        int w = max(_output.right(), _gamut.right()) + 20;

        _gamut.setPosition(Vector(20, _output.bottom() + 20));

        Vector vSpace(0, 15);

        _brightness.setSize(Vector(301, 24));
        _brightness.setPosition(Vector(w, 20));
        _brightnessCaption.setPosition(_brightness.bottomLeft() + vSpace);
        _brightnessText.setPosition(_brightnessCaption.topRight());
        _autoBrightness.setPosition(_brightnessCaption.bottomLeft() + vSpace);

        _saturation.setSize(Vector(301, 24));
        _saturation.setPosition(_autoBrightness.bottomLeft() + 2*vSpace);
        _saturationCaption.setPosition(_saturation.bottomLeft() + vSpace);
        _saturationText.setPosition(_saturationCaption.topRight());
        _autoSaturation.setPosition(_saturationCaption.bottomLeft() + vSpace);

        _contrast.setSize(Vector(301, 24));
        _contrast.setPosition(_autoSaturation.bottomLeft() + 2*vSpace);
        _contrastCaption.setPosition(_contrast.bottomLeft() + vSpace);
        _contrastText.setPosition(_contrastCaption.topRight());
        _autoContrastClip.setPosition(_contrastCaption.bottomLeft() + vSpace);
        _autoContrastMono.setPosition(_autoContrastClip.topRight() +
            Vector(20, 0));

        _hue.setSize(Vector(301, 24));
        _hue.setPosition(_autoContrastClip.bottomLeft() + 2*vSpace);
        _hueCaption.setPosition(_hue.bottomLeft() + vSpace);
        _hueText.setPosition(_hueCaption.topRight());

        _newCGA.setPosition(_hueCaption.bottomLeft() + 2*vSpace);
        _fixPrimaries.setPosition(_newCGA.topRight() + Vector(20, 0));

        _blackText.setPosition(_newCGA.bottomLeft() + 2*vSpace);
        _whiteText.setPosition(_blackText.bottomLeft());
        _mostSaturatedText.setPosition(_whiteText.bottomLeft());
        _clippedColoursText.setPosition(_mostSaturatedText.bottomLeft());

        _matchMode.setPosition(_clippedColoursText.bottomLeft() + 2*vSpace);
        _mode.setPosition(_matchMode.bottomLeft() + vSpace);
        _background.setPosition(_mode.topRight());
        _palette.setPosition(_background.topRight());
        _characterHeight.setPosition(_palette.topRight());

        _diffusionHorizontal.setSize(Vector(301, 24));
        _diffusionHorizontal.setPosition(_matchMode.bottomLeft() + 3*vSpace);
        _diffusionHorizontalCaption.setPosition(
            _diffusionHorizontal.bottomLeft() + vSpace);
        _diffusionHorizontalText.setPosition(
            _diffusionHorizontalCaption.topRight());

        _diffusionVertical.setSize(Vector(301, 24));
        _diffusionVertical.setPosition(_diffusionHorizontalCaption.bottomLeft()
            + vSpace);
        _diffusionVerticalCaption.setPosition(
            _diffusionVertical.bottomLeft() + vSpace);
        _diffusionVerticalText.setPosition(
            _diffusionVerticalCaption.topRight());
    }
    void keyboardCharacter(int character)
    {
        if (character == VK_ESCAPE)
            remove();
    }
    void setSimulator(CGASimulator* simulator)
    {
        _simulator = simulator;
        _output.setSimulator(simulator);
        _gamut.setSimulator(simulator);
    }
    void setDecoder(NTSCDecoder* decoder)
    {
        _decoder = decoder;
        _output.setDecoder(decoder);
        _gamut.setDecoder(decoder);
    }
    void setEncoder(CGAEncoder* encoder)
    {
        _output.setRGBI(encoder->_rgbi);
        _encoder = encoder;
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
        _hueText.setText(format("%f", _decoder->_hue));
        _hueText.size();
        _brightnessText.setText(format("%f", _decoder->_brightness));
        _brightnessText.size();
        _saturationText.setText(format("%f", _decoder->_saturation));
        _saturationText.size();
        _contrastText.setText(format("%f", _decoder->_contrast));
        _contrastText.size();
        _brightness.setValue(_decoder->_brightness);
        _saturation.setValue(_decoder->_saturation);
        _contrast.setValue(_decoder->_contrast);
        _updating = false;
    }
    Colour colourFromSeq(UInt64 seq)
    {
        Byte ntsc[7];
        int phase = (seq >> 32) & 3;
        for (int x = 0; x < 7; ++x) {
            ntsc[x] = _simulator->simulateCGA(seq & 15, (seq >> 4) & 15,
                (x + phase) & 3);
            seq >>= 4;
        }
        return _decoder->decode(ntsc, phase);
    }
    void update()
    {
        Byte burst[4];
        for (int i = 0; i < 4; ++i)
            burst[i] = _simulator->simulateCGA(6, 6, i);
        _decoder->calculateBurst(burst);
        int s[4];
        _simulator->decode(0, s);
        Colour black = _decoder->decode(s);
        _black = 0.299*black.x + 0.587*black.y + 0.114*black.z;
        _simulator->decode(0xffff, s);
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
                _clipped = *i;
            }
            double y = 0.299*r + 0.587*g + 0.114*b;
            _maxSaturation =
                max(_maxSaturation, (c - Colour(y, y, y)).modulus());
            _gamut.add(c);
        }
        _gamut.draw();
        _gamut.invalidate();
    }

    void setMode(int value)
    {
        _encoder->_mode = value;
        _encoder->beginConvert();
    }

    void setBackground(int value)
    {
        _encoder->_background = value;
        _encoder->beginConvert();
    }

    void setPalette(int value)
    {
        _encoder->_palette = value;
        _encoder->beginConvert();
    }

    void setCharacterHeight(int value)
    {
        _encoder->_characterHeight = value;
        _encoder->beginConvert();
    }

    void setHorizontalDiffusion(double value)
    {
        _encoder->_horizontalDiffusion = static_cast<int>(value);
        _diffusionHorizontalText.setText(format("%f", value / 256.0));
        _diffusionHorizontalText.size();
        _encoder->beginConvert();
    }

    void setVerticalDiffusion(double value)
    {
        _encoder->_verticalDiffusion = static_cast<int>(value);
        _diffusionVerticalText.setText(format("%f", value / 256.0));
        _diffusionVerticalText.size();
        _encoder->beginConvert();
    }

    void matchModePressed()
    {
        _encoder->_matchMode = _matchMode.checked();
        _encoder->beginConvert();
        reCreateNTSC();
    }

    void allAutos()
    {
        autoSaturation();
        autoContrastClip();
        autoContrastMono();
        autoBrightness();
    }

    void setBrightness(double brightness)
    {
        _decoder->_brightness = brightness;
        if (!_updating) {
            update();
            uiUpdate();
        }
    }
    void setSaturation(double saturation)
    {
        _decoder->_saturation = saturation;
        if (!_updating) {
            update();
            autoContrastClip();
            uiUpdate();
        }
    }
    void setContrast(double contrast)
    {
        _decoder->_contrast = contrast;
        if (!_updating) {
            update();
            autoBrightness();
            autoSaturation();
            uiUpdate();
        }
    }
    void setHue(double hue)
    {
        _decoder->_hue = hue;
        if (!_updating) {
            update();
            allAutos();
            uiUpdate();
        }
    }
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
        _simulator->_newCGA = _newCGA.checked();
        reCreateNTSC();
    }
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
        _decoder->_brightness += (256 - (_black + _white))/512;
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
    void resetColours() { _colours.reset(); }
    void addColour(UInt64 seq) { _colours.add(seq); }

private:
    AnimatedWindow _animated;
    OutputWindow _output;
    TextWindow _brightnessCaption;
    BrightnessSliderWindow _brightness;
    AutoBrightnessButtonWindow _autoBrightness;
    TextWindow _brightnessText;
    TextWindow _saturationCaption;
    SaturationSliderWindow _saturation;
    AutoSaturationButtonWindow _autoSaturation;
    TextWindow _saturationText;
    TextWindow _contrastCaption;
    ContrastSliderWindow _contrast;
    TextWindow _contrastText;
    AutoContrastClipButtonWindow _autoContrastClip;
    AutoContrastMonoButtonWindow _autoContrastMono;
    TextWindow _hueCaption;
    HueSliderWindow _hue;
    TextWindow _hueText;
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
    CharacterHeightCombo _characterHeight;
    TextWindow _diffusionHorizontalCaption;
    DiffusionHorizontalSliderWindow _diffusionHorizontal;
    TextWindow _diffusionHorizontalText;
    TextWindow _diffusionVerticalCaption;
    DiffusionVerticalSliderWindow _diffusionVertical;
    TextWindow _diffusionVerticalText;
    CGAEncoder* _encoder;
    CGASimulator* _simulator;
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
};

class Program : public WindowProgram<CGA2NTSCWindow>
{
public:
    void run()
    {
        CGASimulator simulator;
        simulator._newCGA = false;
        simulator.initChroma();
        NTSCDecoder decoder;
        decoder._fixPrimaries = false;
        decoder._brightness = 0;
        decoder._hue = 0;
        decoder._contrast = 1;
        decoder._saturation = 1;
        Byte burst[4];
        for (int i = 0; i < 4; ++i)
            burst[i] = simulator.simulateCGA(6, 6, i);
        decoder.calculateBurst(burst);
        _encoder.setSimulator(&simulator);
        _encoder.setDecoder(&decoder);

        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] +
                " <input file name.png>\n");
            return;
        }

        String inputFileName = _arguments[1];

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

        _encoder.setInput(input);
        _encoder.setWindow(&_window);
        _encoder.beginConvert();

        _window.setSimulator(&simulator);
        _window.setDecoder(&decoder);
        _window.setEncoder(&_encoder);

        WindowProgram::run();

        int i;
        for (i = inputFileName.length() - 1; i >= 0; --i)
            if (inputFileName[i] == '.')
                break;
        if (i != -1)
            inputFileName = inputFileName.subString(0, i);

        _window.save(inputFileName + "_out.png");
        _encoder.save(inputFileName + "_out.dat");
        _encoder.saveRGBI(inputFileName + "_out.rgbi");
        _encoder.savePalettes(inputFileName + "_out.palettes");
    }
    bool idle() { return _encoder.idle(); }
private:
    CGAEncoder _encoder;
};
