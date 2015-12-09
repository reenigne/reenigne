#include "alfe/main.h"
#include "alfe/bitmap_png.h"
#include "alfe/terminal6.h"
#include "alfe/complex.h"

#define ELEMENTS 64
#define PIXEL_ALIGN 0

void writeCharacter(Bitmap<SRGB> bitmap, int x, char c)
{
    for (int y = 0; y < 8; ++y)
        for (int xx = 0; xx < 6; ++xx)
            bitmap[Vector(x*6 + xx, y)] = (((glyphs[c*8 + y] << xx) & 0x80) != 0 ? SRGB(255, 255, 255) : SRGB(0, 0, 0));
}

void write(Bitmap<SRGB> bitmap, String caption, double value)
{
    char buffer[0x20];
    for (int i = 0; i < 0x20; ++i)
        buffer[i] = 0;
    sprintf(buffer, ": %.20lg", value);
    int x = 0;
    for (; x < caption.length(); ++x)
        writeCharacter(bitmap, x, caption[x]);
    for (int x2 = 0; x2 < 0x20; ++x2)
        writeCharacter(bitmap, x + x2, buffer[x2]);
}

class MySlider
{
public:
    MySlider() { }
    MySlider(double low, double high, double initial, String caption, double* p, bool use = true)
      : _low(low), _high(high), _caption(caption), _p(p), _max(512), _use(use)
    {
        *p = initial;
        //_dragStartX = static_cast<int>((initial - low)*_max/(high - low));
    }
    MySlider(double low, double high, String caption, double* p, bool use = true)
      : _low(low), _high(high), _caption(caption), _p(p), _max(512), _use(use)
    {
    }

    void setBitmap(Bitmap<SRGB> bitmap) { _bitmap = bitmap; }
    void slideTo(int x)
    {
        if (x < 0)
            x = 0;
        if (x >= _max)
            x = _max - 1;
//        _dragStartX = x;
        *_p = (_high - _low)*x/_max + _low;
    }
    void draw() const { write(_bitmap, _caption, *_p); }
    int currentX() const { return static_cast<int>((*_p - _low)*_max/(_high - _low)); /*_dragStartX;*/ }
    bool use() const { return _use; }
    double value() const { return *_p; }
    double low() const { return _low; }
    double high() const { return _high; }
    void setValue(double v) { *_p = v; }
private:
    double _low;
    double _high;
    String _caption;
    double* _p;
//    int _dragStartX;
    int _max;
    bool _use;

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
        contrast = 1.41; //1.97;
        brightness = -11.0; //-72.8;
        saturation = 0.303; //0.25;
        hue = 0;
        wobbleAmplitude = 0;
        wobblePhase = 180;
    }

    void decode(Byte* input, Bitmap<Colour> output)
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

        Byte* b = input;


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

        Byte* outputLine = output.data();

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
            double* o = reinterpret_cast<double*>(outputLine);
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

                o[0] = y + 0.9563*c.x + 0.6210*c.y;
                o[1] = y - 0.2721*c.x - 0.6474*c.y;
                o[2] = y - 1.1069*c.x + 1.7046*c.y;
                o += 3;
            }
            outputLine += output.stride();
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
};

class Transition
{
public:
    Transition() : _index(0) {}
//    Transition(int i) : _index(i) {}
    Transition(int left, int right, int position)
#if ELEMENTS==8
        : _index(((left & 7) << 6) | position)
#elif ELEMENTS==64
        : _index(((left & 7) << 6) | ((right & 7) << 2) | position)
#else
        : _index((left << 6) | (right << 2) | position)
#endif
    {
    }
    int left() const { return _index >> 6; }
    int right() const { return (_index >> 2) & 0x0f; }
    int position() const { return _index & 3; }
    int index() const { return _index; }
    bool operator==(Transition other) const { return _index == other._index; }
private:
    int _index;
};

class Block
{
public:
    Block() : _index(0) { }
    Block(int i) : _index(i) { }
    Block(int foreground, int background, int bits)
      : _index(foreground | (background << 4) | (bits << 8)) { }
    Block(Vector p)
      : _index((p.x >> 6) | ((p.y >> 6) << 4) |
        ((p.x & 0x30) << 4) | ((p.y & 0x30) << 6)) { }
    Vector vector() const
    {
        int b = bits();
        return Vector((foreground() << 6) | ((b & 3) << 4),
            (background() << 6) | ((b & 0xc) << 2));
    }
    int attribute() const { return _index & 0xff; }
    int foreground() const { return _index & 0xf; }
    int background() const { return (_index >> 4) & 0xf; }
    int bits() const { return _index >> 8; }
    int index() const { return _index; }
    Transition transition(int phase)
    {
        int b = bits();
        int fg = foreground();
        int bg = background();
        int leftBit = phase;
        int rightBit = (phase + 1)&3;
        bool left = ((b << leftBit) & 8) != 0;
        bool right = ((b << rightBit) & 8) != 0;
        int leftColour = (left ? fg : bg);
        int rightColour = (right ? fg : bg);
        return Transition(leftColour, rightColour, phase);
    }
    int iState(int phase)
    {
        int b = bits();
        int fg = foreground();
        int bg = background();
        int leftBit = phase;
        bool left = ((b << leftBit) & 8) != 0;
        int leftColour = (left ? fg : bg);
#if PIXEL_ALIGN
        int rightColour = 0;
#else
        int rightBit = (phase + 1)&3;
        bool right = ((b << rightBit) & 8) != 0;
        int rightColour = (right ? fg : bg);
#endif
        return (leftColour >> 3) | ((rightColour >> 2) & 2);
    }
    bool operator==(Block other) const { return _index == other._index; }
private:
    int _index;
};

class CalibrateWindow;

template<class T> class CalibrateBitmapWindowT : public BitmapWindow
{
public:
    ~CalibrateBitmapWindowT()
    {
        //Array<Byte> data(1024);
        //for (int i = 0; i < 1024; ++i)
        //    data[i] = static_cast<Byte>(_tSamples[i]*256);
        AutoStream h = File("output.dat").openWrite();
        h.write(reinterpret_cast<Byte*>(&_tSamples[0]), 1024*sizeof(double));
    }
    void setCalibrateWindow(CalibrateWindow* window)
    {
        _calibrateWindow = window;
    }

    void create()
    {
        setSize(Vector(1536, 1024));

        double brightness = -0.124;
        double contrast = 1.052;
        _aPower = 0;

        AutoStream st = File("q:\\Pictures\\reenigne\\top.raw", true).openRead();
        AutoStream sb = File("q:\\Pictures\\reenigne\\bottom.raw", true).openRead();

        static const int samples = 450*1024;
        static const int sampleSpaceBefore = 256;
        static const int sampleSpaceAfter = 256;
        Array<Byte> topNTSC;
        Array<Byte> bottomNTSC;
        topNTSC.allocate(sampleSpaceBefore + samples + sampleSpaceAfter);
        bottomNTSC.allocate(sampleSpaceBefore + samples + sampleSpaceAfter);
        Byte* bt = &topNTSC[0] + sampleSpaceBefore;
        Byte* bb = &bottomNTSC[0] + sampleSpaceBefore;
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
        _top = Bitmap<Colour>(Vector(760, 240));
        _bottom = Bitmap<Colour>(Vector(760, 240));

        NTSCDecoder decoder;
        decoder.brightness = -11.0;
        decoder.contrast = 1.41;
        decoder.hue = 0;
        decoder.saturation = 0.303;
        decoder.decode(bt, _top);
        decoder.decode(bb, _bottom);

        //AutoStream topStream = File("q:\\pictures\\reenigne\\top_decoded.raw", true).openRead();
        //AutoStream bottomStream = File("q:\\pictures\\reenigne\\bottom_decoded.raw", true).openRead();
        //topStream.read(_top.data(), 760*240*3);
        //bottomStream.read(_bottom.data(), 760*240*3);

        _output = Bitmap<SRGB>(Vector(1536, 1024));
        _rgb = ColourSpace::rgb();
        _srgb = ColourSpace::srgb();

        drawCaptures();

        //Colour directColours[16];
        //Colour directColoursSRGB[16];
        //double dcVoltage[16];
        //for (int c = 0; c < 16; ++c) {
        //    directColours[c] = Colour(0, 0, 0);
        //    for (int b = 0; b < 16; ++b)
        //        directColours[c] += _captures[Block(c, c, b).index()];
        //    directColours[c] /= 16;
        //    Colour srgb = _rgb.toSrgb(directColours[c]);
        //    directColoursSRGB[c] = srgb;
        //    dcVoltage[c] = (srgb.x*0.299 + srgb.y*0.587 + srgb.z*0.114)/256.0;
        //}
        //_voltages[0] = dcVoltage[0];
        //_voltages[1] = dcVoltage[8];
        //_voltages[2] = dcVoltage[7];
        //_voltages[3] = dcVoltage[15];

        //_sampleScale = (_voltages[3] - _voltages[0])*256/(161 - 38);
        //_sampleOffset = _voltages[0]*256 - _sampleScale*38;

        for (int i = 0; i < 1024; ++i)
            _tSamples[i] = 0.5;
        for (int i = 0; i < 4; ++i)
            _iSamples[i] = 0;

        _sliders[0] = MySlider(0, 2, 0.5655, "saturation", &_saturation);
        _sliders[1] = MySlider(-180, 180, 0, "hue", &_hue, PIXEL_ALIGN);
        _sliders[2] = MySlider(-1, 1, 0, "brightness", &_brightness, false);
        _sliders[3] = MySlider(0, 2, 1, "contrast", &_contrast, false);
        _sliders[4] = MySlider(0, 1, 0.185, "transition", &_transitionPoint, true);
#if ELEMENTS!=256
        _baseSliders = 8;
        _sliders[5] = MySlider(0, 1, "I On", &_iSamples[3], !PIXEL_ALIGN);
        _sliders[6] = MySlider(0, 1, "I Rising", &_iSamples[2], !PIXEL_ALIGN);
        _sliders[7] = MySlider(0, 1, "I Falling", &_iSamples[1]);
#else
        _baseSliders = 5;
#endif
        _sliderCount = _baseSliders + 4;

        int fg = 0;
        _attribute = Vector(fg, fg);
        newAttribute();

        for (int i = 0; i < _sliderCount; ++i) {
            _sliders[i].setBitmap(_output.subBitmap(Vector(1032, i*8), Vector(512, 8)));
            _sliders[i].draw();
        }
        _slider = -1;

        for (int i = 0; i < 4096; ++i)
            _optimizing[i] = true;
        computeFitness();

        _clicked = Block(0);
        escapePressed();

        SRGB white(255, 255, 255);
        _waveformTL = Vector(1032, (20 + 4)*8);
        for (int x = -1; x <= 256; ++x) {
            _output[_waveformTL + Vector(-1, x)] = white;
            _output[_waveformTL + Vector(256, x)] = white;
            _output[_waveformTL + Vector(x, -1)] = white;
            //_output[_waveformTL + Vector(x, 256)] = white;
        }

        _paused = false;

        BitmapWindow::create();

        _iteration = 0;
        _doneClimb = false;
    }

    void paint()
    {
        _calibrateWindow->restart();
    }

    virtual void draw()										
    {
        if (!_bitmap.valid())
            _bitmap = Bitmap<DWORD>(Vector(1536, 1024));
        setNextBitmap(_bitmap);

        for (int i = 0; i < _sliderCount; ++i)
            _sliders[i].draw();

        drawCaptures();
        for (int i = 0; i < 4096; ++i) {
            Block b(i);
            Vector p = b.vector();
            SRGB c = _srgb.toSrgb24(_computes[i]);
            for (int xx = 0; xx < 16; ++xx)
                for (int yy = 0; yy < 8; ++yy)
                    _output[p + Vector(xx, yy + 8)] = c;
        }

        write(_output.subBitmap(Vector(1024, (23 + 1)*8), Vector(512, 8)), "Fitness", _fitness);
        write(_output.subBitmap(Vector(1024, (23 + 2)*8), Vector(512, 8)), "Delta", _aPower);

        Vector p = _attribute << 6;
        SRGB white(255, 255, 255);
        //for (int x = 0; x < 64; ++x) {
        //    _output[p + Vector(0, x)] = white;
        //    _output[p + Vector(63, x)] = white;
        //    _output[p + Vector(x, 0)] = white;
        //    _output[p + Vector(x, 63)] = white;
        //}
        //for (int y = 0; y < 256; ++y)
        //    for (int x = 0; x < 256; ++x)
        //        _output[_waveformTL + Vector(x, y)] = SRGB(0, 0, 0);

        // Copy the _output bitmap to the Image
        Vector zero(0, 0);
        Vector sz = size();
        if (sz.x > 1536)
            sz.x = 1536;
        if (sz.y > 1024)
            sz.y = 1024;

        Byte* row = _bitmap.data();
        const Byte* otherRow = _output.data();
        for (int y = 0; y < sz.y; ++y) {
            DWORD* p = reinterpret_cast<DWORD*>(row);
            const SRGB* op = reinterpret_cast<const SRGB*>(otherRow);
            for (int x = 0; x < sz.x; ++x) {
                *p = (op->x << 16) | (op->y << 8) | op->z;
                ++p;
                ++op;
            }
            row += _bitmap.stride();
            otherRow += _output.stride();
        }
        invalidate();
    }

    void drawCaptures()
    {
        for (int i = 0; i < 4096; ++i) {
            Block b(i);
            Colour rgb = getPixel4(b.foreground(), b.background(), b.bits());
            _captures[i] = rgb;
            SRGB c = _srgb.toSrgb24(rgb);
            Vector p = b.vector();
            for (int xx = 0; xx < 16; ++xx)
                for (int yy = 0; yy < 8; ++yy)
                    _output[p + Vector(xx, yy)] = c;
        }
    }

    void newAttribute()
    {
//        drawCaptures();
        int fg = _attribute.x;
        int bg = _attribute.y;
        if (_attribute.x == _attribute.y)
            _sliderCount = _baseSliders + 4;
        else
            _sliderCount = _baseSliders + 16;
        for (int i = _baseSliders; i < _sliderCount; ++i) {
            _sliders[i] = MySlider(0, 1, "", &_tSamples[transitionForSlider(i).index()]);
            _sliders[i].setBitmap(_output.subBitmap(Vector(1032, i*8), Vector(512, 8)));
            _sliders[i].draw();
        }
    }
    Transition transitionForSlider(int slider)
    {
        int fg = _attribute.x;
        int bg = _attribute.y;
        slider -= _baseSliders;
        int phase = slider&3;
        switch (slider >> 2) {
            case 0: return Transition(fg, fg, phase);
            case 1: return Transition(bg, bg, phase);
            case 2: return Transition(fg, bg, phase);
            case 3: return Transition(bg, fg, phase);
        }
        throw Exception();
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
            for (int i = 0; i < _sliderCount; ++i)
                _output[Vector(1028, i*8 + 4)] = (i == _slider ? SRGB(255, 255, 255) : SRGB(0, 0, 0));

            int bestI = 0;
            double bestFitness = 1000000;
            MySlider* slider = &_sliders[_slider];
            double current = slider->value();
            for (int i = -10; i < 10; ++i) {
                double trial = clamp(slider->low(), current + i*(slider->high() - slider->low())/1000, slider->high());
                slider->setValue(trial);
                computeFitness();
                if (_fitness < bestFitness) {
                    bestFitness = _fitness;
                    bestI = i;
                }
            }
            slider->setValue(clamp(slider->low(), current + bestI*(slider->high() - slider->low())/1000, slider->high()));
            computeFitness();

            _dragStartX = _sliders[_slider].currentX();
            mouseMove(position);
            return true;
        }
        if (position.inside(Vector(1024, 1024))) {
            _attribute = position >> 6;
            newAttribute();
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
            computeFitness();
            draw();
            invalidate();
        }
        _position = position;
    }

    void escapePressed()
    {
        for (int i = 0; i < 4096; ++i) {
            Block b(i);
            int bits = b.bits();
            _optimizing[i] = true; //(bits != 4 && bits != 0x0b);
        }
    }

    void pause() { _paused = !_paused; }
//
    bool idle()
    {
        if (_paused)
            return false;
        bool climbed = false;
        for (int i = 0; i < _sliderCount*2; ++i) {
            // Pick a slider
            MySlider* slider = &_sliders[i >> 1];
            if (!slider->use())
                continue;

            double oldFitness = _fitness;
            Colour oldComputes[4096];
            double oldFitnesses[4096];
            for (int j = 0; j < 4096; ++j) {
                oldComputes[j] = _computes[j];
                oldFitnesses[j] = _fitnesses[j];
            }

            // Pick a direction to move it in
            bool lower = ((i & 1) == 0);

            // Save old slider value
            double oldValue = slider->value();

            // Move the slider
            double amount = (slider->high() - slider->low())/(1 << _aPower);
            double value;
            if (lower) {
                value = oldValue - amount;
                if (value < slider->low())
                    value = slider->low();
            }
            else {
                value = oldValue + amount;
                if (value > slider->high())
                    value = slider->high();
            }
            slider->setValue(value);

            if (i < (_baseSliders << 1))
                for (int bn = 0; bn < 4096; ++bn)
                    _optimizing[bn] = true;
            else {
                Transition t = transitionForSlider(i >> 1);
                if (t.left() == 6 && t.right() == 6)
                    for (int bn = 0; bn < 4096; ++bn)
                        _optimizing[bn] = true;
                else
                    for (int bn = 0; bn < 4096; ++bn) {
                        _optimizing[bn] =
                            (t == Block(bn).transition(t.position()));
                    }
            }

            computeFitness();

            // If old fitness was better, restore _fitness, slider value and
            // computed colours.
            if (oldFitness <= _fitness) {
                _fitness = oldFitness;
                slider->setValue(oldValue);
                for (int j = 0; j < 4096; ++j) {
                    _computes[j] = oldComputes[j];
                    _fitnesses[j] = oldFitnesses[j];
                }
            }
            else {
                climbed = true;
                _doneClimb = true;
            }
        }
        if (!climbed) {
            ++_attribute.x;
            if (_attribute.x == 16) {
                _attribute.x = 0;
                ++_attribute.y;
                if (_attribute.y == 16) {
                    _attribute.y = 0;
                    if (!_doneClimb) {
                        ++_aPower;
                        if (_aPower == 38)
                            remove();
                    }
                    _doneClimb = false;
                }
            }
            newAttribute();
        }
        return true;
    }

private:
    void integrate(Block b, double* dc, Complex<double>* iq/*, double* hf*/)
    {
        double s[4];
        for (int t = 0; t < 4; ++t) {
            s[t] = _tSamples[b.transition(t).index()];
#if ELEMENTS!=256
            s[t] += _iSamples[b.iState(t)];
#endif
        }
        *dc = (s[0] + s[1] + s[2] + s[3])/4;
        iq->x = (s[0] - s[2])/2;
        iq->y = (s[1] - s[3])/2;
//        *hf = ((s[0] + s[2]) - (s[1] + s[3]))/4;
        //*iq *= unit(_transitionPoint);
    }

    void computeFitness()
    {
        double dc;
        Complex<double> iqBurst;
//        double hf;
        integrate(Block(6, 6, 0), &dc, &iqBurst/*, &hf*/);
        Complex<double> iqAdjust = -iqBurst.conjugate()*unit((33 + 90 + _hue)/360.0)*_saturation*_contrast/iqBurst.modulus();
        if (iqBurst.modulus2() == 0)
            iqAdjust = 0;

        _fitness = 0;
        for (int bn = 0; bn < 4096; ++bn) {
            Block block(bn);
            if (_optimizing[bn]) {
                Complex<double> iq;
                integrate(block, &dc, &iq /*, &hf*/);
                double y = dc*_contrast + _brightness;
                iq *= iqAdjust;

                double r = 255*(y + 0.9563*iq.x + 0.6210*iq.y);
                double g = 255*(y - 0.2721*iq.x - 0.6474*iq.y);
                double b = 255*(y - 1.1069*iq.x + 1.7046*iq.y);
                Colour c(r, g, b);
                _computes[bn] = c;

                double f = (c - /*_rgb.toSrgb*/(_captures[bn])).modulus2();
                _fitnesses[bn] = f;
            }

            _fitness += _fitnesses[bn];
        }
        _fitness /= 4096;
    }

    Colour getDecodedPixel0(int bitmap, Vector p)
    {
        Bitmap<Colour> b;
        switch (bitmap) {
            case 0: b = _top; break;
            case 1: b = _bottom; break;
        }
        return b[p];
    }
    Colour getDecodedPixel1(int bitmap, Vector p)
    {
        return /*_rgb.fromSrgb*/(getDecodedPixel0(bitmap, p));
    }
    Colour getPixel2(Vector p)
    {
        int bitmap = 0;
        if (p.y >= 100) {
            bitmap = 1;
            p.y -= 100;
        }
        Vector p2(p.x*40/3 + 158, p.y*2 + 17);
        Colour c(0, 0, 0);
        c += getDecodedPixel1(bitmap, p2);
        c += getDecodedPixel1(bitmap, p2 + Vector(0, 1));
        return c/2;
    }
    Colour getPixel3(int patch, int line, int set)
    {
        int y = (set/3)*2;
        bool firstHalf = (patch < 3);
        patch += line*10;
        switch (set % 3) {
            case 0: return getPixel2(Vector(patch, y));
            case 1:
                if (firstHalf)
                    return getPixel2(Vector(patch + 6, y));
                return getPixel2(Vector(patch - 3, y + 1));
            case 2: return getPixel2(Vector(patch + 3, y + 1));
        }
        return Colour(0, 0, 0);
    }
    Colour getPixel4(int fg, int bg, int bits)
    {
        int patch = 0;
        int row = 0;
        Colour c(0, 0, 0);
        switch (bits) {
            case 0x00: return getPixel3(2, 0, (bg << 4) | bg);
            case 0x01: return getPixel3(0, 1, (fg << 4) | bg);
            case 0x02: return getPixel3(1, 0, (bg << 4) | fg);
            case 0x03: return getPixel3(2, 0, (fg << 4) | bg);
            case 0x04: return getPixel3(5, 3, (fg << 4) | bg);
            case 0x05: return getPixel3(3, 0, (bg << 4) | fg);
            case 0x06: return getPixel3(4, 0, (bg << 4) | fg);
            case 0x07: return getPixel3(1, 1, (fg << 4) | bg);
            case 0x08: return getPixel3(1, 1, (bg << 4) | fg);
            case 0x09: return getPixel3(4, 0, (fg << 4) | bg);
            case 0x0a: return getPixel3(3, 0, (fg << 4) | bg);
            case 0x0b: return getPixel3(5, 3, (bg << 4) | fg);
            case 0x0c: return getPixel3(2, 0, (bg << 4) | fg);
            case 0x0d: return getPixel3(1, 0, (fg << 4) | bg);
            case 0x0e: return getPixel3(0, 1, (bg << 4) | fg);
            case 0x0f: return getPixel3(2, 0, (fg << 4) | fg);
        }
        return c;
    }

    Bitmap<DWORD> _bitmap;

    Bitmap<Colour> _top;
    Bitmap<Colour> _bottom;
    Bitmap<SRGB> _output;
    ColourSpace _rgb;
    ColourSpace _srgb;

    double _ab;
    Complex<double> _qamAdjust;

    MySlider _sliders[25];
    int _slider;
    int _sliderCount;
    int _baseSliders;

    Vector _dragStart;
    int _dragStartX;

    double _saturation;
    double _hue;
    double _brightness;
    double _contrast;

//    double _voltages[4];
    Colour _captures[4096];
    Colour _computes[4096];
    double _fitness;

    Block _clicked;
    Vector _waveformTL;
    Vector _attribute;
    bool _optimizing[4096];
    double _fitnesses[4096];

    double _sampleScale;
    double _sampleOffset;
    double _topPhase;
    double _bottomPhase;

    bool _paused;

//    double _variance;

    double _tSamples[1024];
    double _iSamples[4];

    double _transitionPoint;

    int _aPower;

    int _buttons;
    Vector _position;

    int _iteration;

    bool _doneClimb;

    CalibrateWindow* _calibrateWindow;
};

typedef CalibrateBitmapWindowT<void> CalibrateBitmapWindow;

class CalibrateWindow : public RootWindow
{
public:
    void restart() { _animated.restart(); }
    void setWindows(Windows* windows)
    {
        _bitmap.setCalibrateWindow(this);

        add(&_bitmap);
        add(&_animated);

        _animated.setDrawWindow(&_bitmap);
        _animated.setRate(1);
        RootWindow::setWindows(windows);
    }
    void create()
    {
        setText("CGA Calibration");
        setSize(Vector(1536, 1024));
        _bitmap.setPosition(Vector(0, 0));
        RootWindow::create();
        _animated.start();
    }
    void keyboardCharacter(int character)
    {
        if (character == 'x' || character == 'X')
            _bitmap.escapePressed();
        if (character == 'p' || character == 'P')
            _bitmap.pause();
    }
    bool idle() { return _bitmap.idle(); }
private:
    CalibrateBitmapWindow _bitmap;
    AnimatedWindow _animated;
};

class Program : public WindowProgram<CalibrateWindow>
{
public:
    bool idle() { return _window.idle(); }
};
