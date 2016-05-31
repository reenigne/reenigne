#include "alfe/main.h"
#include "alfe/bitmap_png.h"
#include "alfe/terminal6.h"
#include "alfe/complex.h"

class Slider
{
public:
    Slider() { }
    Slider(double low, double high, double initial, String caption, double* p, int max = 512)
      : _low(low), _high(high), _caption(caption), _p(p), _max(max)
    {
        *p = initial;
        _dragStartX = (initial - low)*max/(high - low);
    }
    void setBitmap(Bitmap<SRGB> bitmap) { _bitmap = bitmap; }
    void slideTo(int x)
    {
        if (x < 0)
            x = 0;
        if (x >= _max)
            x = _max - 1;
        _dragStartX = x;
        *_p = (_high - _low)*x/_max + _low;
        draw();
    }
    void draw()
    {
        char buffer[0x20];
        for (int i = 0; i < 0x20; ++i)
            buffer[i] = 0;
        sprintf(buffer, ": %lf", *_p);
        int x = 0;
        for (; x < _caption.length(); ++x)
            drawCharacter(x, _caption[x]);
        for (int x2 = 0; x2 < 0x20; ++x2)
            drawCharacter(x + x2, buffer[x2]);
    }

    void drawCharacter(int x, char c)
    {
        for (int y = 0; y < 8; ++y)
            for (int xx = 0; xx < 6; ++xx)
                _bitmap[Vector(x*6 + xx, y)] = (((glyphs[c*8 + y] << xx) & 0x80) != 0 ? SRGB(255, 255, 255) : SRGB(0, 0, 0));
    }

    int currentX() { return _dragStartX; }
private:
    double _low;
    double _high;
    String _caption;
    double* _p;
    int _dragStartX;
    int _max;

    Bitmap<SRGB> _bitmap;
};

float sinc(float z)
{
    if (z == 0.0f)
        return 1.0f;
    z *= M_PI;
    return sin(z)/z;
}

float lanczos(float z)
{
    if (z < -3 || z > 3)
        return 0;
    return sinc(z)*sinc(z/3);
}

template<class T> Byte checkClamp(T x)
{
    int y = static_cast<int>(x);
    return clamp(0, y, 255);
//    return x;
}

Complex<float> rotor(float phase)
{
    float angle = phase*tau;
    return Complex<float>(cos(angle), sin(angle));
}

class NTSCDecoder
{
public:
    NTSCDecoder()
    {
        //contrast = 1.41;
        //brightness = -11.0;
        //saturation = 0.303;
        contrast = 1.97;
        brightness = -72.8;
        saturation = 0.25;
        hue = 0;
        wobbleAmplitude = 0;
        wobblePhase = 180;
    }
    void setBuffers(Byte* input, Byte* output) { _input = input; _output = output; }

    void decode()
    {
        // Settings

        static const int lines = 240;
        static const int nominalSamplesPerLine = 1820;
        static const int firstSyncSample = -130;  // Assumed position of previous hsync before our samples started
        static const int pixelsPerLine = 760;
        static const float kernelSize = 3;  // Lanczos parameter
        static const int nominalSamplesPerCycle = 8;
        static const int driftSamples = 40;
        static const int burstSamples = 40;
        static const int firstBurstSample = 192;
        static const int burstCenter = firstBurstSample + burstSamples/2;

        Byte* b = _input;


        // Pass 1 - find sync and burst pulses, compute wobble amplitude and phase

        float deltaSamplesPerCycle = 0;

        int syncPositions[lines + 1];
        int oldP = firstSyncSample - driftSamples;
        int p = oldP + nominalSamplesPerLine;
        float samplesPerLine = nominalSamplesPerLine;
        Complex<float> bursts[lines + 1];
        float burstDCs[lines + 1];
        Complex<float> wobbleRotor = 0;
        Complex<float> hueRotor = rotor((33 + hue)/360);
        float totalBurstAmplitude = 0;
        float burstDCAverage = 0;
        for (int line = 0; line < lines + 1; ++line) {
            Complex<float> burst = 0;
            float burstDC = 0;
            for (int i = firstBurstSample; i < firstBurstSample + burstSamples; ++i) {
                int j = oldP + i;
                int sample = b[j];
                float phase = (j&7)/8.0;
                burst += rotor(phase)*sample;
                burstDC += sample;
            }

            float burstAmplitude = burst.modulus()/burstSamples;
            totalBurstAmplitude += burstAmplitude;
            wobbleRotor += burstAmplitude*rotor(burst.argument() * 8 / tau);
            bursts[line] = burst*hueRotor/burstSamples;
            burstDC /= burstSamples;
            burstDCs[line] = burstDC;

            syncPositions[line] = p;
            oldP = p;
            for (int i = 0; i < driftSamples*2; ++i) {
                if (b[p] < 9)
                    break;
                ++p;
            }
            p += nominalSamplesPerLine - driftSamples;

            samplesPerLine = (2*samplesPerLine + p - oldP)/3;
            burstDCAverage = (2*burstDCAverage + burstDC)/3;
        }
        float averageBurstAmplitude = totalBurstAmplitude / (lines + 1);
        //wobbleAmplitude = 0.0042; //wobbleRotor.modulus() / (lines + 1);
        //wobblePhase = 0.94;  //wobbleRotor.argument() / tau;

        float deltaSamplesPerLine = samplesPerLine - nominalSamplesPerLine;


        // Pass 2 - render

        Byte* output = _output;

        float q = syncPositions[1] - samplesPerLine;
        Complex<float> burst = bursts[0];
        float rotorTable[8];
        for (int i = 0; i < 8; ++i)
            rotorTable[i] = rotor(i/8.0).x*saturation;
        Complex<float> expectedBurst = burst;
        int oldActualSamplesPerLine = nominalSamplesPerLine;
        for (int line = 0; line < lines; ++line) {
            // Determine the phase, amplitude and DC offset of the color signal
            // from the color burst, which starts shortly after the horizontal
            // sync pulse ends. The color burst is 9 cycles long, and we look
            // at the middle 5 cycles.

            float contrast1 = contrast;
            int samplesPerLineInt = samplesPerLine;
            Complex<float> actualBurst = bursts[line];
            burst = (expectedBurst*2 + actualBurst)/3;

            float phaseDifference = (actualBurst*(expectedBurst.conjugate())).argument()/tau;
            float adjust = -phaseDifference/pixelsPerLine;

            Complex<float> chromaAdjust = burst.conjugate()*contrast1*saturation;
            burstDCAverage = (2*burstDCAverage + burstDCs[line])/3;
            float brightness1 = brightness + 65 - burstDCAverage;

            // Resample the image data

            //float samplesPerLine = nominalSamplesPerLine + deltaSamplesPerLine;
            for (int x = 0 /*101*/; x < 760 /*101 + 640*/; ++x) {
                float y = 0;
                Complex<float> c = 0;
                float t = 0;

                float kFrac0 = x*samplesPerLine/pixelsPerLine;
                float kFrac = q + kFrac0;
                int k = kFrac;
                kFrac -= k;
                float samplesPerCycle = nominalSamplesPerCycle + deltaSamplesPerCycle;
                float z0 = -kFrac/samplesPerCycle;
                int firstInput = -kernelSize*samplesPerCycle + kFrac;
                int lastInput = kernelSize*samplesPerCycle + kFrac;

                for (int j = firstInput; j <= lastInput; ++j) {
                    // The input sample corresponding to the output pixel is k+kFrac
                    // The sample we're looking at in this iteration is j+k
                    // The difference is j-kFrac
                    // So the value we pass to lanczos() is (j-kFrac)/samplesPerCycle
                    // So z0 = -kFrac/samplesPerCycle;

                    float s = lanczos(j/samplesPerCycle + z0);
                    int i = j + k;
                    float z = s*b[i];
                    y += z;
                    c.x += rotorTable[i & 7]*z;
                    c.y += rotorTable[(i + 6) & 7]*z;
                    //c += rotor((i&7)/8.0)*z*saturation;
                    t += s;
                }

                float wobble = 1 - cos(c.argument()*8 + wobblePhase*tau)*wobbleAmplitude; ///(averageBurstAmplitude*contrast);

                y = y*contrast1*wobble/t + brightness1; // - cos(c.argument()*8 + wobblePhase*tau)*wobbleAmplitude*contrast;
                c = c*chromaAdjust*rotor((x - burstCenter*pixelsPerLine/samplesPerLine)*adjust)*wobble/t;

                output[0] = checkClamp(y + 0.9563*c.x + 0.6210*c.y);
                output[1] = checkClamp(y - 0.2721*c.x - 0.6474*c.y);
                output[2] = checkClamp(y - 1.1069*c.x + 1.7046*c.y);
                output += 3;
            }
            //output += (pixelsPerLine - 190)*3;

            int p = syncPositions[line + 1];
            int actualSamplesPerLine = p - syncPositions[line];
            samplesPerLine = (2*samplesPerLine + actualSamplesPerLine)/3;
            q += samplesPerLine;
            q = (10*q + p)/11;

            expectedBurst = actualBurst;
            //printf("line %i: actual=%f, used=%f. difference=%f\n",line,actualBurst.argument()*360/tau, burst.argument()*360/tau, actualBurst.argument()*360/tau - burst.argument()*360/tau);
        }
    }
    float contrast;
    float brightness;
    float saturation;
    float hue;
    float wobbleAmplitude;
    float wobblePhase;
    Byte* _input;
    Byte* _output;
};

class CalibrateBitmapWindow : public BitmapWindow
{
public:
    void create()
    {
        setInnerSize(Vector(1536, 1024));

        BitmapFileFormat<SRGB> png = PNGFileFormat();
        _top1 = png.load(File("q:\\Pictures\\reenigne\\top1.png", true));
        _top2 = png.load(File("q:\\Pictures\\reenigne\\top2.png", true));
        _bottom1 = png.load(File("q:\\Pictures\\reenigne\\bottom1.png", true));
        _bottom2 = png.load(File("q:\\Pictures\\reenigne\\bottom2.png", true));
        _output = Bitmap<SRGB>(Vector(1536, 1024));
        _rgb = ColourSpace::rgb();

        AutoStream st = File("q:\\Pictures\\reenigne\\top.raw", true).openRead();
        AutoStream sb = File("q:\\Pictures\\reenigne\\bottom.raw", true).openRead();

        static const int samples = 450*1024;
        static const int sampleSpaceBefore = 256;
        static const int sampleSpaceAfter = 256;
        _topNTSC.allocate(sampleSpaceBefore + samples + sampleSpaceAfter);
        _bottomNTSC.allocate(sampleSpaceBefore + samples + sampleSpaceAfter);
        Byte* bt = &_topNTSC[0] + sampleSpaceBefore;
        Byte* bb = &_bottomNTSC[0] + sampleSpaceBefore;
        for (int i = 0; i < sampleSpaceBefore; ++i) {
            bt[i - sampleSpaceBefore] = 0;
            bb[i - sampleSpaceBefore] = 0;
        }
        for (int i = 0; i < sampleSpaceAfter; ++i) {
            bt[i + samples] = 0;
            bb[i + samples] = 0;
        }
        for (int i = 0; i < 450; ++i) {
            st.read(&bt[i*1024], 1024);
            sb.read(&bb[i*1024], 1024);
        }
        _topDecoded = Bitmap<SRGB>(Vector(760, 240));
        _bottomDecoded = Bitmap<SRGB>(Vector(760, 240));

        _captures = Bitmap<Colour>(Vector(256, 16));

        for (int fg = 0; fg < 16; ++fg)
            for (int bg = 0; bg < 16; ++bg)
                for (int bits = 0; bits < 16; ++bits) {
                    int x = (fg << 3) | ((bits & 3) << 1);
                    int y = (bg << 3) | ((bits & 0xc) >> 1);
                    Colour rgb = getPixel4(fg, bg, bits);
                    _captures[Vector(bg*16 + fg, bits)] = rgb;
                    SRGB c = _rgb.toSrgb24(rgb);
                    for (int xx = 0; xx < 16; ++xx)
                        for (int yy = 0; yy < 8; ++yy) {
                            Vector p = (Vector(x, y)<<3) + Vector(xx, yy);
                            _output[p] = c;
                        }
                }

        _sliderCount = 4;
        //_sliders[0] = Slider(0, 1, 0.303, "saturation", &_saturation);
        //_sliders[1] = Slider(-180, 180, 0, "hue", &_hue);
        //_sliders[2] = Slider(-255, 255, -11, "brightness", &_brightness);
        //_sliders[3] = Slider(0, 4, 1.41, "contrast", &_contrast);
        _sliders[0] = Slider(0, 1, 0.238, "saturation", &_saturation);
        _sliders[1] = Slider(-180, 180, 0, "hue", &_hue);
        _sliders[2] = Slider(-255, 255, -103.6, "brightness", &_brightness);
        _sliders[3] = Slider(0, 4, 1.969, "contrast", &_contrast);

        for (int i = 0; i < _sliderCount; ++i) {
            _sliders[i].setBitmap(_output.subBitmap(Vector(1024, i*8), Vector(512, 8)));
            _sliders[i].draw();
        }
        _slider = -1;

        BitmapWindow::create();
    }

    virtual void draw()
    {
        NTSCDecoder decoder;
        decoder.brightness = _brightness;
        decoder.contrast = _contrast;
        decoder.hue = _hue;
        decoder.saturation = _saturation;
        decoder.setBuffers(&_topNTSC[0] + 256, _topDecoded.data());
        decoder.decode();
        decoder.setBuffers(&_bottomNTSC[0] + 256, _bottomDecoded.data());
        decoder.decode();

        File("q:\\top_decoded.raw",true).save(_topDecoded.data(), 760*240*3);
        File("q:\\bottom_decoded.raw",true).save(_bottomDecoded.data(), 760*240*3);

        for (int fg = 0; fg < 16; ++fg)
            for (int bg = 0; bg < 16; ++bg)
                for (int bits = 0; bits < 16; ++bits) {
                    int x = (fg << 3) | ((bits & 3) << 1);
                    int y = (bg << 3) | ((bits & 0xc) >> 1);
                    SRGB g = decode(fg, bg, bits);
                    for (int xx = 0; xx < 16; ++xx)
                        for (int yy = 0; yy < 8; ++yy) {
                            Vector p = (Vector(x, y)<<3) + Vector(xx, yy);
                            _output[p + Vector(0, 8)] = g;
                        }
                }

        Vector zero(0, 0);
        Vector sz = innerSize();
        if (sz.x > 1536)
            sz.x = 1536;
        if (sz.y > 1024)
            sz.y = 1024;
        //subBitmap(zero, s).copyFrom(_output.subBitmap(zero, s));
        //_output.subBitmap(zero, Vector(760, 240)).copyFrom(_topDecoded);

        Byte* row = data();
        const Byte* otherRow = _output.data();
        for (int y = 0; y < sz.y; ++y) {
            DWORD* p = reinterpret_cast<DWORD*>(row);
            const SRGB* op = reinterpret_cast<const SRGB*>(otherRow);
            for (int x = 0; x < sz.x; ++x) {
                *p = (op->x << 16) | (op->y << 8) | op->z;
                ++p;
                ++op;
            }
            row += stride();
            otherRow += _output.stride();
        }
    }

    bool mouseInput(Vector position, int buttons)
    {
        bool capture = false;
        buttons &= MK_LBUTTON;
        if (buttons != 0 && _buttons == 0)
            capture = buttonDown(position);
        if (buttons == 0 && _buttons != 0)
            buttonUp(position);
        if (_position != position)
            mouseMove(position);
        _position = position;
        _buttons = buttons;
        return capture;
    }

    bool buttonDown(Vector position)
    {
        Vector p = position - Vector(1024, 0);
        if (p.inside(Vector(512, _sliderCount*8))) {
            _dragStart = position;
            _slider = p.y/8;
            _dragStartX = _sliders[_slider].currentX();
            mouseMove(position);
            return true;
        }
        return false;
    }

    void buttonUp(Vector position)
    {
        mouseMove(position);
        _slider = -1;
    }

    void mouseMove(Vector position)
    {
        if (_slider != -1) {
            Vector p = position -= _dragStart;
            _sliders[_slider].slideTo(_dragStartX + p.x - p.y);
            draw();
            invalidate();
        }
    }

private:
    SRGB decode(int foreground, int background, int bits)
    {
        return _rgb.toSrgb24(getPixel4(foreground, background, bits, true));
    }

    SRGB getPixel0(int bitmap, Vector p)
    {
        Bitmap<SRGB> b;
        switch (bitmap) {
            case 0: b = _top1; break;
            case 1: b = _top2; break;
            case 2: b = _bottom1; break;
            case 3: b = _bottom2; break;
        }
        return b[p];
    }
    SRGB getDecodedPixel0(int bitmap, Vector p)
    {
        Bitmap<SRGB> b;
        switch (bitmap) {
            case 0: b = _topDecoded; break;
            case 1: b = _bottomDecoded; break;
        }
        return b[p];
    }
    Colour getPixel1(int bitmap, Vector p)
    {
        Colour p0 = _rgb.fromSrgb(getPixel0(bitmap << 1, p));
        Colour p1 = _rgb.fromSrgb(getPixel0((bitmap << 1) | 1, p));
        return (p0 + p1)/2;
    }
    Colour getDecodedPixel1(int bitmap, Vector p)
    {
        return _rgb.fromSrgb(getDecodedPixel0(bitmap, p));
    }
    Colour getPixel2(Vector p, bool decoded)
    {
        int bitmap = 0;
        if (p.y >= 100) {
            bitmap = 1;
            p.y -= 100;
        }
        if (decoded) {
            Vector p2(p.x*40/3 + 158, p.y*2 + 17);
            Colour c(0, 0, 0);
            c += getDecodedPixel1(bitmap, p2);
            c += getDecodedPixel1(bitmap, p2 + Vector(0, 1));
            return c/2;
        }

        Vector p2(p.x*14 + 31, p.y*4 + 14);
        Colour c(0, 0, 0);
        c += getPixel1(bitmap, p2);
        c += getPixel1(bitmap, p2 + Vector(0, 1));
        c += getPixel1(bitmap, p2 + Vector(1, 0));
        c += getPixel1(bitmap, p2 + Vector(1, 1));
        c += getPixel1(bitmap, p2 + Vector(-1, 0));
        c += getPixel1(bitmap, p2 + Vector(-1, 1));
        c += getPixel1(bitmap, p2 + Vector(-2, 0));
        c += getPixel1(bitmap, p2 + Vector(-2, 1));
        return c/8;
    }
    Colour getPixel3(int patch, int line, int set, bool decoded)
    {
        int y = (set/3)*2;
        bool firstHalf = (patch < 3);
        patch += line*10;
        switch (set % 3) {
            case 0: return getPixel2(Vector(patch, y), decoded);
            case 1:
                if (firstHalf)
                    return getPixel2(Vector(patch + 6, y), decoded);
                return getPixel2(Vector(patch - 3, y + 1), decoded);
            case 2: return getPixel2(Vector(patch + 3, y + 1), decoded);
        }
        return Colour(0, 0, 0);
    }
    Colour getPixel4(int fg, int bg, int bits, bool decoded = false)
    {
        int patch = 0;
        int row = 0;
        Colour c(0, 0, 0);
        switch (bits) {
            case 0x00: return getPixel3(2, 0, (bg << 4) | bg, decoded);
            case 0x01: return getPixel3(0, 1, (fg << 4) | bg, decoded);
            case 0x02: return getPixel3(1, 0, (bg << 4) | fg, decoded);
            case 0x03: return getPixel3(2, 0, (fg << 4) | bg, decoded);
            case 0x04: return getPixel3(5, 3, (fg << 4) | bg, decoded);
            case 0x05: return getPixel3(3, 0, (bg << 4) | fg, decoded);
            case 0x06: return getPixel3(4, 0, (bg << 4) | fg, decoded);
            case 0x07: return getPixel3(1, 1, (fg << 4) | bg, decoded);
            case 0x08: return getPixel3(1, 1, (bg << 4) | fg, decoded);
            case 0x09: return getPixel3(4, 0, (fg << 4) | bg, decoded);
            case 0x0a: return getPixel3(3, 0, (fg << 4) | bg, decoded);
            case 0x0b: return getPixel3(5, 3, (bg << 4) | fg, decoded);
            case 0x0c: return getPixel3(2, 0, (bg << 4) | fg, decoded);
            case 0x0d: return getPixel3(1, 0, (fg << 4) | bg, decoded);
            case 0x0e: return getPixel3(0, 1, (bg << 4) | fg, decoded);
            case 0x0f: return getPixel3(2, 0, (fg << 4) | fg, decoded);
        }
        return c;
    }

    Bitmap<SRGB> _top1;
    Bitmap<SRGB> _top2;
    Bitmap<SRGB> _bottom1;
    Bitmap<SRGB> _bottom2;
    Bitmap<SRGB> _output;
    Array<Byte> _topNTSC;
    Array<Byte> _bottomNTSC;
    Bitmap<SRGB> _topDecoded;
    Bitmap<SRGB> _bottomDecoded;
    ColourSpace _rgb;

    double _ab;
    Complex<double> _qamAdjust;

    Slider _sliders[19];
    int _slider;
    int _sliderCount;

    Vector _dragStart;
    int _dragStartX;

    double _saturation;
    double _hue;
    double _brightness;
    double _contrast;

    Bitmap<Colour> _captures;

    int _buttons;
    Vector _position;
};

class CalibrateWindow : public RootWindow
{
public:
    void create()
    {
        setInnerSize(1536, 1024);
        setText("CGA Calibration");
        add(&_bitmap);
        RootWindow::create();
    }
    void keyboardCharacter(int character)
    {
        if (character == VK_ESCAPE)
            _bitmap.remove();
    }
private:
    CalibrateBitmapWindow _bitmap;
};

class Program : public WindowProgram<CalibrateWindow> { };
