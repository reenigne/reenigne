#include "unity/main.h"
#include "unity/file.h"
#include "unity/perceptual.h"
#include <stdio.h>
#include "unity/user.h"
#include "unity/thread.h"

typedef Vector3<double> Colour;

// TODO: Move this to perceptual.h and refactor?
class PerceptualModel
{
public:
    PerceptualModel() { }
    static PerceptualModel luv() { return PerceptualModel(true); }
    static PerceptualModel lab() { return PerceptualModel(false); }
    Colour perceptualFromSrgb(const Vector3<UInt8>& srgb)
    {
        if (_luv)
            return luvFromSrgb(srgb);
        else
            return labFromSrgb(srgb);
    }
private:
    PerceptualModel(bool luv) : _luv(luv) { }
    bool _luv;
};

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

        {
            String srgbInput;
#if 1
            File inputFile(String("/t/castle.raw"));
            srgbInput = inputFile.contents();
            _pictureSize = Vector(640, 100);
#else
            File inputFile(String("/t/rose.raw"));
            srgbInput = inputFile.contents();
            _pictureSize = Vector(320, 200);
#endif
            _perceptualInput.allocate(_pictureSize.x*_pictureSize.y);
            int p = 0;
            int q = 0;
            for (int y = 0; y < _pictureSize.y; ++y)
                for (int x = 0; x < _pictureSize.x; ++x) {
                    _perceptualInput[q++] = _model.perceptualFromSrgb(Colour(
                        srgbInput[p], srgbInput[p + 1], srgbInput[p + 2]));
                    p += 3;
                }
        }

        _srgbOutput.allocate(_size.x*_size.y*3);

        _srgbPalette[0x00] = Vector3<UInt8>(0x00, 0x00, 0x00);
        _srgbPalette[0x01] = Vector3<UInt8>(0x00, 0x00, 0xaa);
        _srgbPalette[0x02] = Vector3<UInt8>(0x00, 0xaa, 0x00);
        _srgbPalette[0x03] = Vector3<UInt8>(0x00, 0xaa, 0xaa);
        _srgbPalette[0x04] = Vector3<UInt8>(0xaa, 0x00, 0x00);
        _srgbPalette[0x05] = Vector3<UInt8>(0xaa, 0x00, 0xaa);
        _srgbPalette[0x06] = Vector3<UInt8>(0xaa, 0x55, 0x00);
        _srgbPalette[0x07] = Vector3<UInt8>(0xaa, 0xaa, 0xaa);
        _srgbPalette[0x08] = Vector3<UInt8>(0x55, 0x55, 0x55);
        _srgbPalette[0x09] = Vector3<UInt8>(0x55, 0x55, 0xff);
        _srgbPalette[0x0a] = Vector3<UInt8>(0x55, 0xff, 0x55);
        _srgbPalette[0x0b] = Vector3<UInt8>(0x55, 0xff, 0xff);
        _srgbPalette[0x0c] = Vector3<UInt8>(0xff, 0x55, 0x55);
        _srgbPalette[0x0d] = Vector3<UInt8>(0xff, 0x55, 0xff);
        _srgbPalette[0x0e] = Vector3<UInt8>(0xff, 0xff, 0x55);
        _srgbPalette[0x0f] = Vector3<UInt8>(0xff, 0xff, 0xff);

        for (int i = 0; i < 0x10; ++i)
            _perceptualPalette[i] =
                _model.perceptualFromSrgb(Colour(_srgbPalette[i]));

        _dataOutput.allocate(_pictureSize.x*_pictureSize.y/4);

        _position = Vector(0, 0);
        _changed = false;

        _compositeData.allocate((_pictureSize.x+6)*_pictureSize.y);

        _srgbPixels.allocate(14);
        _perceptualPixels.allocate(14);
        _compositePixels.allocate(8);

        _thread.initialize(this);
        _thread.start();
    }

    void paint(const PaintHandle& paint)
    {
        // TODO: scaling?
        Byte* l = getBits();
        for (int y = 0; y < _size.y; ++y) {
            DWord* p = reinterpret_cast<DWord*>(l);
            for (int x = 0; x < _size.x; ++x) {
                DWord srgb = 0;
                if (Vector(x, y).inside(_pictureSize)) {
                    int o = (y*_pictureSize.x + x)*3;
                    srgb = (_srgbOutput[o]<<16) + (_srgbOutput[o + 1]<<8) +
                        _srgbOutput[o + 2];
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
        outputFile.save(String(Buffer(_srgbOutput), 0, _pictureSize.x*_pictureSize.y*3));

        File dataFile(String("picture.dat"));
        dataFile.save(String(Buffer(_dataOutput), 0, _pictureSize.x*_pictureSize.y/4));
    }

    void computeSrgbPixels(UInt8 bits, UInt8 fg, UInt8 bg)
    {
        // These give the colour burst patterns for the 8 colours ignoring
        // the intensity bit.
        static const int colorBurst[8][4] = {
            {0,0,0,0}, /* Black */
            {0,1,3,2}, /* Blue */
            {3,0,0,3}, /* Green */
            {2,0,1,3}, /* Cyan */
            {1,3,2,0}, /* Red */
            {0,3,3,0}, /* Magenta */
            {3,2,0,1}, /* Yellow-burst */
            {3,3,3,3}};/* White */

        // The values in the colorBurst array index into phaseLevels which 
        // gives us the amount of time that the +CHROMA bit spends high during
        // that pixel. Changing "phase" corresponds to tuning the "colour
        // adjust" trimmer on the PC motherboard.
        // TODO: In -HRES modes the color burst phase is always colour 6, but 
        // in +HRES modes the color burst is the border colour due to a bug in
        // the design of the CGA. Try all 6 possible border colours (7 if you
        // count black as well, which is different from setting +BW) to see
        // which gives the best image.
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

        for (int i = 0; i < 8; ++i) {


        }



        // TODO: update _compositePixels
        // TODO: update _srgbPixels using _compositePixels and _compositeData
        // ppppppppPPPPPPPPpppppppp
        //         12345677654321
        //         0123456789ABCD
    }

    void calculate()
    {
        int bestPattern;
        int bestAt;
        double bestScore = 1e99;
        for (int pattern = 0; pattern < _patternCount; ++pattern) {
            UInt8 bits = _patterns[pattern];
            for (int at = 0; at < 0x100; ++at) {
                int fg = at & 0x0f;
                int bg = at >> 4;
                if ((bits == 0 || bits == 0xff) != (fg == bg))
                    continue;
                double score = 0;
                computeSrgbPixels(bits, fg, bg);
                // TODO: Compute the perceptual colour for all affected pixels
                // TODO: Compute the error for all affected pixels
                // TODO: Compute the total error

//                Colour error2 = error;
//                for (int xx = 0; xx < 8; ++xx) {
//                    int col = ((bits & (128 >> xx)) != 0) ? fg : bg;
//                    Colour target = _perceptualImage[_position.y*width + _position.x + xx] + error2;
//                    error2 = target - colours[col];
//                    score += error2.modulus2();
//                    error2 *= 0.5;
//                }
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

            computeSrgbPixels(_patterns[bestPattern], bestAt & 0x0f, bestAt >> 4);
            for (int i = 0; i < 8; ++i)
                _compositeData[
            // TODO: Copy _srgbPixels to _srgbOutput
            // TODO: Copy _compositePixels to _compositeData
            // TODO: Compute the perceptual colour for all affected pixels
            // TODO: Copy _perceptualPixels to _perceptualOutput
            // TODO: Diffuse the error

//            for (int xx = 0; xx < 8; ++xx) {
//                int col = ((bits & (128 >> xx)) != 0) ?
//                    (bestAt & 0x0f) : (bestAt >> 4);
//                int p = (y*width + x + xx)*3;
//                Vector3<UInt8> rgb = palette[col];
//                buffer[p] = rgb.x;
//                buffer[p + 1] = rgb.y;
//                buffer[p + 2] = rgb.z;
//                Colour target = image[y*width + x + xx] + error;
//                error = target - colours[col];
//                error *= 0.5;
//                if (y < height - 1)
//                    image[(y + 1)*width + x + xx] += error;
//            }
        }


        _position.x += 8;
        if (_position.x >= _size.x) {
            ++_position.y;
            _position.x = 0;
            if (_position.y == _size.y) {
                if (!_changed)
                    _thread.finished();
                _position.y = 0;
            }
        }
    }
private:
    // TODO: Refactor this with CalcThread in fractal.h as EndableThread?
    // TODO: Make a BackgroundCalculationImage base class?
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
    Array<UInt8> _srgbOutput;
    Array<UInt8> _dataOutput;
    Array<int> _compositeData;

    Array<UInt8> _srgbPixels;
    Array<Colour> _perceptualPixels;
    Array<int> _compositePixels;

    Vector _pictureSize;
    PerceptualModel _model;
    CalcThread _thread;
    Vector3<UInt8> _srgbPalette[0x10];
    Colour _perceptualPalette[0x10];
    Vector _position;
    bool _changed;

    Array<UInt8> _patterns;
    Array<UInt8> _characters;
    int _patternCount;
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


//  Use 640/912 pixels per scanline, so we can re-use crtsim code
//    Problem: For some colours we need 1280 pixels per scanline
//      Just use linear interpolation for the half-pixels - that way the hue can be adjusted. Frequency response works out the same once high frequencies filtered out
//  Don't forget: colour 6 required in the borders!
//  To compute each output pixel, need to look at 7 input pixels
//    Trying all 36*256 possibilities for 2 characters would take ~10 hours!
//    Not taking into account next character when computing current one will lead to poorer images than ideal
//    Genetic algorithm?

// The sample grid should be aligned such that 00330033 is green/magenta[/orange/aqua], not blue/cyan/red/yellow-burst
//   The former aligns the samples with the pixels with the composite samples