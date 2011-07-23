#include "unity/main.h"
#include "unity/file.h"
#include "unity/perceptual.h"
#include <stdio.h>
#include "unity/user.h"
#include "unity/thread.h"

typedef Vector3<int> YIQ;

class AttributeClashImage : public Image
{
public:
    AttributeClashImage()
    {
        // Determine the set of unique patterns that appear in the top lines
        // of CGA text characters.
        {
            File file(String("/t/projects/emulation/mamemess/mess_run/roms/"
                "pc/5788005.u33"));
            String cgaROM = file.contents();
            _patterns.allocate(0x100);
            _characters.allocate(0x100);
            _patternCount = 0;
            for (int i = 0; i < 0x100; ++i) {
                UInt8 bits = cgaROM[(3*256 + i)*8];
                int j;
                for (j = 0; j < _patternCount; ++j)
                    if (_patterns[j] == bits || _patterns[j] == ~bits)
                        break;
                if (j == _patternCount) {
                    _patterns[_patternCount] = bits;
                    _characters[_patternCount] = i;
                    ++_patternCount;
                }
            }
        }
#if 0
        // Dump entire ROM so we can see how it's laid out.
        int fileSize = 8*16 * 4*16*8;
        OwningBuffer buffer(fileSize);

        for (int set = 0; set < 4; ++set) {
            for (int ch = 0; ch < 256; ++ch) {
                for (int y = 0; y < 8; ++y) {
                    int bits = data[(set*256 + ch)*8 + y];
                    for (int x = 0; x < 8; ++x)
                        buffer[
                            ((set*16 + (ch >> 4))*8 + y)*8*16 + (ch & 15)*8 + x
                            ] = ((bits & (128 >> x)) != 0 ? 255 : 0);
                }
            }
        }
        // Set 0: MDA characters rows 0-7
        // Set 1: MDA characters rows 8-13
        // Set 2: CGA narrow characters
        // Set 3: CGA normal characters
#endif
#if 0
        // Dump the top rows to a text file
        int fileSize = 13*256;
        OwningBuffer buffer(fileSize);

        int hexDigit(int n) { return n < 10 ? n + '0' : n + 'a' - 10; }

        for (int ch = 0; ch < 256; ++ch) {
            int bits = data[(3*256 + ch)*8];
            for (int x = 0; x < 8; ++x) {
                int p = ch*13;
                buffer[p + x] = ((bits & (128 >> x)) != 0 ? '*' : ' ');
                buffer[p + 8] = ' ';
                buffer[p + 9] = hexDigit(ch >> 4);
                buffer[p + 10] = hexDigit(ch & 15);
                buffer[p + 11] = 13;
                buffer[p + 12] = 10;
            }
        }
#endif
        _model = PerceptualModel::luv();

        String srgbInput;
        {
#if 1
            File inputFile(String("/t/castle.raw"));
            srgbInput = inputFile.contents();
            _pictureSize = Vector(640, 100);
#else
            File inputFile(String("/t/rose.raw"));
            srgbInput = inputFile.contents();
            _pictureSize = Vector(320, 200);
#endif
        }

        //_srgbPalette[0x00] = SRGB(0x00, 0x00, 0x00);
        //_srgbPalette[0x01] = SRGB(0x00, 0x00, 0xaa);
        //_srgbPalette[0x02] = SRGB(0x00, 0xaa, 0x00);
        //_srgbPalette[0x03] = SRGB(0x00, 0xaa, 0xaa);
        //_srgbPalette[0x04] = SRGB(0xaa, 0x00, 0x00);
        //_srgbPalette[0x05] = SRGB(0xaa, 0x00, 0xaa);
        //_srgbPalette[0x06] = SRGB(0xaa, 0x55, 0x00);
        //_srgbPalette[0x07] = SRGB(0xaa, 0xaa, 0xaa);
        //_srgbPalette[0x08] = SRGB(0x55, 0x55, 0x55);
        //_srgbPalette[0x09] = SRGB(0x55, 0x55, 0xff);
        //_srgbPalette[0x0a] = SRGB(0x55, 0xff, 0x55);
        //_srgbPalette[0x0b] = SRGB(0x55, 0xff, 0xff);
        //_srgbPalette[0x0c] = SRGB(0xff, 0x55, 0x55);
        //_srgbPalette[0x0d] = SRGB(0xff, 0x55, 0xff);
        //_srgbPalette[0x0e] = SRGB(0xff, 0xff, 0x55);
        //_srgbPalette[0x0f] = SRGB(0xff, 0xff, 0xff);

        //for (int i = 0; i < 0x10; ++i)
        //    _perceptualPalette[i] =
        //        _model.perceptualFromSrgb(_srgbPalette[i]);

        _dataOutput.allocate(_pictureSize.x*_pictureSize.y/4);

        _position = Vector(0, 0);
        _changed = false;

        _compositeOffset = Vector(8, 4);
        _compositeSize = _pictureSize + _compositeOffset*2;
        _compositeData.allocate(_compositeSize.x*_compositeSize.y);
        _outputSize = _compositeSize - Vector(6, 0);

        _srgbOutput.allocate(_outputSize.x*_outputSize.y);
        _perceptualOutput.allocate(_outputSize.x*_outputSize.y);
        _perceptualError.allocate(_outputSize.x*_outputSize.y);
        _perceptualInput.allocate(_outputSize.x*_outputSize.y);

        static const int overscanColour = 6;

        static const float brightness = 0.06f;
        static const float contrast = 3.0f;
        static const float saturation = 0.7f;
        static const float tint = 18.0f;

        _yContrast = static_cast<int>(contrast*1463.0f);
        static const float radians = static_cast<float>(M_PI)/180.0f;
        float tintI = -cos((103.0f + tint)*radians);
        float tintQ = sin((103.0f + tint)*radians);

        // Determine the color burst.
        float colorBurst[4];
        // First set _iqMultipliers to 0. The result of setCompositeData will
        // have valid Y data but not valid I and Q data yet. Fortunately we
        // only need the Y data to find the color burst.
        for (int i = 0; i < 4; ++i)
            _iqMultipliers[i] = 0;
        for (int i = 0; i < 4; ++i) {
            setCompositeData(Vector(i, 0) - _compositeOffset, overscanColour);
            colorBurst[i] = static_cast<float>(_compositeData[i].x);
        }
        float burstI = colorBurst[2] - colorBurst[0];      
        float burstQ = colorBurst[3] - colorBurst[1];
        float colorBurstGain = 32.0f/sqrt((burstI*burstI + burstQ*burstQ)/2);
        float s = saturation*contrast*colorBurstGain*0.352f;
        _iqMultipliers[0] = static_cast<int>((burstI*tintI - burstQ*tintQ)*s);
        _iqMultipliers[1] = static_cast<int>((burstQ*tintI + burstI*tintQ)*s);
        _iqMultipliers[2] = -_iqMultipliers[0];
        _iqMultipliers[3] = -_iqMultipliers[1];

        _gamma.allocate(256);
         for (int i = 0; i < 256; ++i)
            _gamma[i] = static_cast<int>(
                pow(static_cast<float>(i)/255.0f, 1.9f)*255.0f);

        _brightness =
            static_cast<int>(brightness*100.0 - 7.5f*256.0f*contrast)<<8;

        // Now that _iqMultipliers has been initialized correctly, we can set
        // initialize _compositeData. Let's start it off 
        for (int y = 0; y < _compositeSize.y; ++y)
            for (int x = 0; x < _compositeSize.x; ++x)
                setCompositeData(Vector(x, y) - _compositeOffset,
                    overscanColour);

        errorFor(0, 0, overscanColour);

        int p = 0;
        int q = 0;
        Colour border = _perceptualOutput[6];
        for (int y = 0; y < _outputSize.y; ++y)
            for (int x = 0; x < _outputSize.x; ++x) {
                int p = y*_outputSize.x + x;
                _srgbOutput[p] = _srgbOutput[6];
                _perceptualOutput[p] =
                    _model.perceptualFromSrgb(_srgbOutput[p]);
                Colour c;
                if ((Vector(x, y) - _compositeOffset).inside(_pictureSize)) {
                    c = _model.perceptualFromSrgb(SRGB(
                        srgbInput[p], srgbInput[p + 1], srgbInput[p + 2]));
                    p += 3;
                }
                else
                    c = border;
                _perceptualInput[q++] = c;
                _perceptualError[p] =
                    _perceptualOutput[p] - _perceptualInput[p];
            }

        _thread.initialize(this);
        _thread.start();
    }

    void paint(const PaintHandle& paint)
    {
        Byte* l = getBits();
        for (int y = 0; y < _size.y; ++y) {
            DWord* p = reinterpret_cast<DWord*>(l);
            for (int x = 0; x < _size.x; ++x) {
                DWord srgb = 0;
                if (Vector(x, y).inside(_outputSize)) {
                    SRGB s = _srgbOutput[y*_outputSize.x + x];
                    srgb = (s.x<<16) + (s.y<<8) + s.z;
                }
                *(p++) = srgb;
            }
            l += _byteWidth;
        }
        Image::paint(paint);
    }

    void destroy()
    {
        _thread.end();

        File outputFile(String("attribute_clash.raw"));
        outputFile.save(String(
            Buffer(&_srgbOutput[0].x), 0, _pictureSize.x*_pictureSize.y*3));

        File dataFile(String("picture.dat"));
        dataFile.save(String(
            Buffer(&_dataOutput[0]), 0, _pictureSize.x*_pictureSize.y/4));
    }

    void setCompositeData(Vector p, int c)
    {
        // These give the colour burst patterns for the 8 colours ignoring
        // the intensity bit.
        static const int colorBurst[8][4] = {
            {0, 0, 0, 0}, /* Black */
            {0, 1, 3, 2}, /* Blue */
            {3, 0, 0, 3}, /* Green */
            {2, 0, 1, 3}, /* Cyan */
            {1, 3, 2, 0}, /* Red */
            {0, 3, 3, 0}, /* Magenta */
            {3, 2, 0, 1}, /* Yellow-burst */
            {3, 3, 3, 3}};/* White */

        // The values in the colorBurst array index into phaseLevels which
        // gives us the amount of time that the +CHROMA bit spends high during
        // that pixel. Changing "phase" corresponds to tuning the "color
        // adjust" trimmer on the PC motherboard. This trimmer adjusts the
        // hues of green and magenta and artifact colours, not
        // blue/cyan/red/yellow-burst chroma colours.

        static const int phase = 128;
        static const int phaseLevels[4] = {0, phase, 256, 256-phase};

        // The following levels are computed as follows:
        // Using Falstad's circuit simulator applet
        // (http://www.falstad.com/circuit/) with the CGA composite output
        // stage and a 75 ohm load gives the following voltages:
        //   +CHROMA = 0,  +I = 0  0.416V  (colour 0)
        //   +CHROMA = 0,  +I = 1  0.709V  (colour 8)
        //   +CHROMA = 1,  +I = 0  1.160V  (colour 7)
        //   +CHROMA = 1,  +I = 1  1.460V  (colour 15)
        // Scaling these and adding an offset (equivalent to adjusting the
        // contrast and brightness respectively) such that colour 0 is at the
        // standard black level of IRE 7.5 and that colour 15 is at the
        // standard white level of IRE 100 gives:
        //   +CHROMA = 0,  +I = 0  IRE   7.5
        //   +CHROMA = 0,  +I = 1  IRE  33.5
        //   +CHROMA = 1,  +I = 0  IRE  73.4
        //   +CHROMA = 1,  +I = 1  IRE 100.0
        // Then we convert to sample levels using the standard formula:
        //   sample = 1.4*IRE + 60
        static const int sampleLevels[4] = {71, 107, 163, 200};

        // The sample grid should be aligned such that 00330033 is green/magenta[/orange/aqua], not blue/cyan/red/yellow-burst
        //   The former aligns the samples with the pixels with the composite samples
        // 0  0000  black
        // 1  0001  dark cyan
        // 2  0010  dark blue
        // 3  0011  aqua
        // 4  0100  dark red
        // 5  0101  grey
        // 6  0110  magenta
        // 7  0111  light blue
        // 8  1000  dark yellow-burst
        // 9  1001  green
        // A  1010  grey
        // B  1011  light cyan
        // C  1100  orange
        // D  1101  light yellow-burst
        // E  1110  light red
        // F  1111  white

        // So the order of bits is yellow-burst/red/blue/cyan

        int chroma = phaseLevels[colorBurst[c & 7][p.x & 3]];
        int intensity = (c & 8) >> 3;
        int sampleLow = sampleLevels[intensity];
        int sampleHigh = sampleLevels[intensity + 2];
        int sample = (((sampleHigh - sampleLow)*chroma) >> 8) + sampleLow - 60;
        Vector q = p + _compositeOffset;
        _compositeData[q.y*_compositeSize.x + q.x] = YIQ(sample,
            sample*_iqMultipliers[p.x & 3], 
            sample*_iqMultipliers[(p.x + 3)&3]);
    }

    Colour target(Vector p)
    {
        Colour c = _perceptualInput[p.y*_outputSize.x + p.x];
        //if (p.y > 0)
        //    c += _perceptualError[(p.y - 1)*_outputSize.x + p.x]/2;
        //if (p.x > 0)
        //    c += _perceptualError[p.y*_outputSize.x + p.x - 1]/2;
        return c;
    }

    double errorFor(UInt8 bits, UInt8 fg, UInt8 bg)
    {
        for (int i = 0; i < 8; ++i) {
            int colour = ((bits & (128 >> i)) != 0 ? fg : bg);
            setCompositeData(_position + Vector(i, 0), colour);
        }

        Vector pos = _position;
        YIQ* d = &_compositeData[pos.y*_compositeSize.x + pos.x + 6];

        int p = pos.y*_outputSize.x + pos.x;

        double error = 0;

        for (int x = 0; x < 14; ++x) {
            // We use a low-pass Finite Impulse Response filter to
            // remove high frequencies (including the color carrier
            // frequency) from the signal. We could just keep a
            // 4-sample running average but that leads to sharp edges
            // in the resulting image.
            // The kernel of this FIR is [1, 4, 7, 8, 7, 4, 1]
            YIQ yiq = 
                    d[x - 6] + d[x - 0]
                + ((d[x - 5] + d[x - 1])<<2) 
                +  (d[x - 4] + d[x - 2])*7 
                +  (d[x - 3]<<3);

            // Contrast for I and Q is handled by _iqMultipliers, along with
            // saturation. Brightness only affects Y.
            int y = yiq.x*_yContrast + _brightness;
            int i = yiq.y;
            int q = yiq.z;

            _srgbOutput[p] = SRGB(
                _gamma[clamp(0, (y + 243*i + 160*q)>>16, 255)],
                _gamma[clamp(0, (y -  71*i - 164*q)>>16, 255)],
                _gamma[clamp(0, (y - 283*i + 443*q)>>16, 255)]);

            _perceptualOutput[p] =
                _model.perceptualFromSrgb(_srgbOutput[p]);
            _perceptualError[p] =
                _perceptualOutput[p] - target(pos);
            error += _perceptualError[p].modulus2();
            ++p;
            ++pos.x;
        }
        return error;
    }

    void calculate()
    {
        int bestPattern;
        int bestAt;
        double bestScore = 1e99;
//        _patternCount = 1;
        for (int pattern = 0; pattern < _patternCount; ++pattern) {
            UInt8 bits = _patterns[pattern];
            for (int at = 0; at < 0x100; ++at) {
                int fg = at & 0x0f;
                int bg = at >> 4;
                if ((bits == 0 || bits == 0xff) != (fg == bg))
                    continue;
                double score = errorFor(bits, fg, bg);
                if (score < bestScore) {
                    bestScore = score;
                    bestPattern = pattern;
                    bestAt = at;
                }
            }
        }
        int character = _characters[bestPattern];
        int p = (_position.y*_pictureSize.x + _position.x)/4;
        if (character != _dataOutput[p] || bestAt != _dataOutput[p + 1]) {
            _changed = true;
            _dataOutput[p] = character;
            _dataOutput[p + 1] = bestAt;
        }
        errorFor(_patterns[bestPattern], bestAt & 0x0f, bestAt >> 4);

        _position.x += 8;
        if (_position.x >= _pictureSize.x) {
            ++_position.y;
            _position.x = 0;
            if (_position.y == _pictureSize.y) {
                if (!_changed)
                    _thread.finished();
                _position.y = 0;
            }
        }
    }
private:
    class CalcThread : public Thread
    {
    public:
        CalcThread() : _ending(false) { }

        void initialize(AttributeClashImage* image)
        {
            _image = image;
            doRestart();
        }

        void end()
        {
            _ending = true;
            join();
        }

        void finished()
        {
            _ending = true;
        }

    private:                                        
        void doRestart()
        {
            _restartRequested = false;
        }

        void threadProc()
        {
            do {
                if (_restartRequested) {
                    doRestart();
                    _event.signal();
                }
                _image->calculate();
            } while (!_ending);
        }

        bool _restartRequested;
        bool _ending;
        Event _event;
        AttributeClashImage* _image;
    };

    Array<Colour> _perceptualInput;

    Array<Colour> _perceptualOutput;
    Array<Colour> _perceptualError;
    Array<SRGB> _srgbOutput;
    Array<UInt8> _dataOutput;
    Array<YIQ> _compositeData;

    Vector _pictureSize;
    Vector _compositeSize;
    Vector _compositeOffset;
    Vector _outputSize;
    PerceptualModel _model;
    CalcThread _thread;
    //SRGB _srgbPalette[0x10];
//    Colour _perceptualPalette[0x10];
    Vector _position;
    bool _changed;

    Array<UInt8> _patterns;
    Array<UInt8> _characters;
    int _patternCount;

    Array<int> _gamma;

    int _iqMultipliers[4];

    int _yContrast;
    int _brightness;
};

class Program : public ProgramBase
{
public:
    int run()
    {
        AttributeClashImage image;

        Window::Params wp(&_windows, L"Composite output");
        typedef RootWindow<Window> RootWindow;
        RootWindow::Params rwp(wp);
        typedef ImageWindow<RootWindow, AttributeClashImage> ImageWindow;
        ImageWindow::Params iwp(rwp, &image);
        typedef AnimatedWindow<ImageWindow> AnimatedWindow;
        AnimatedWindow::Params awp(iwp);
        AnimatedWindow window(awp);

        window.show(_nCmdShow);
        return pumpMessages();
    }
};
