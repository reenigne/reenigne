#include "alfe/main.h"
#include "alfe/bitmap_png.h"
#include "alfe/terminal6.h"
#include "alfe/complex.h"

float sinc(float z)
{
    if (z == 0.0f)
        return 1.0f;
    z *= M_PI;
    return sin(z)/z;
}

static const int lobes = 3;

float lanczos(float z)
{
    return sinc(z)*sinc(z/lobes);
}

template<class T> Byte checkClamp(T x)
{
    int y = static_cast<int>(x);
    return clamp(0, y, 255);
//    return x;
}

Complex<float> rotor(float phase)
{
    float angle = static_cast<float>(phase*tau);
    return Complex<float>(cos(angle), sin(angle));
}

template<class T> class NTSCCaptureDecoder
{
public:
    void setInputBuffer(Byte* input) { _input = input; }

    void decode()
    {
        // Settings

        static const int nominalSamplesPerLine = 1824;
        static const int firstSyncSample = -40;
        static const int driftSamples = 40;
        static const int burstSamples = 48;  // Central 6 of 8-10 cycles
        static const int firstBurstSample = 32 + driftSamples;      // == 72

        Byte* b = _input;

        int p = -40 - driftSamples;                  // == -80
        for (int line = 0; line < 240; ++line) {
            Complex<float> burst = 0;
            float burstDC = 0;
            for (int i = firstBurstSample; i < firstBurstSample + burstSamples; ++i) {
                int j = p + i;                                   // == -8
                int sample = b[j];
                float phase = (j&7)/8.0f;
                burst += rotor(phase)*sample;
                burstDC += sample;
            }
            burst /= burstSamples;
            _burstAmplitude[line] = burst.modulus()/burstSamples;
            _burstDC[line] = burstDC/burstSamples;
            double burstPhase = 4 - 8*burst.argument()/tau;  // 0 to 8

            int ibp = static_cast<int>(burstPhase);
            _p[line] = _b+p+ibp;
            _o[line] = burstPhase - ibp;
            p += nominalSamplesPerLine - driftSamples;
            int i;
            for (i = 0; i < driftSamples*2; ++i) {
                if (b[p] < 9)
                    break;
                ++p;
            }
            for (; i < driftSamples*2; ++i) {
                if (b[p] >= 12)
                    break;
                ++p;
            }

            if (line > 0)
                _v[line] = ((_p[line + 1] - _p[line]) - (_o[line + 1] - _o[line]))/nominalSamplesPerLine;
        }
        _v[0] = _v[1];
    }

    // Get a pointer to the samples that are on scanline y at 8*x color carrier
    // cycles from end of hsync pulse. *p is a pointer to the next sample after
    // that point. *o is the distance between x and where *p is sampled, in
    // eights of a color carrier cycle. 0<=*o<1
    void getSamples(double x, int y, Byte** p, double* o)
    {
        // Need:
        //   z0
        //   k
        Byte* pp = _p[y];  // Sample at x = 0
        double oo = _o[y] + _v[y]*x;
        int i = static_cast<int>(oo);
        *o = oo - static_cast<double>(i);
        *p = pp + i;
    }
private:
    Byte* _input;

    Byte* _p[240];
    double _o[240];
    double _v[240];
    double _burstDC[240];
    double _burstAmplitude[240];
};

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

struct ColourHF
{
    ColourHF() : c(Colour(0, 0, 0)), hf(0) { }
    ColourHF(Colour _c, Complex<double> _hf) : c(_c), hf(_hf) { }
    const ColourHF& operator-=(const ColourHF& other)
    {
        c -= other.c;
        hf -= other.hf;
        return *this;
    }
    const ColourHF& operator+=(const ColourHF& other)
    {
        c += other.c;
        hf += other.hf;
        return *this;
    }
    const ColourHF& operator/=(const double& other)
    {
        c /= other;
        hf /= other;
        return *this;
    }
    ColourHF operator-(const ColourHF& other) const
    {
        ColourHF t(*this);
        t -= other;
        return t;
    }
    ColourHF operator+(const ColourHF& other) const
    {
        ColourHF t(*this);
        t += other;
        return t;
    }
    ColourHF operator/(const double& other) const
    {
        ColourHF t(*this);
        t /= other;
        return t;
    }
    double modulus2() const { return c.modulus2() + hf.modulus2()*10; }

    Colour c;
    Complex<double> hf;
};

class Transition
{
public:
    Transition() : _index(0) {}
//    Transition(int i) : _index(i) {}
    Transition(int left, int right, int position)
        : _index(((left & 7) << 6) | ((right & 7) << 2) | position) { }
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
        int rightBit = (phase + 1)&3;
        bool right = ((b << rightBit) & 8) != 0;
        int rightColour = (right ? fg : bg);
        return (leftColour >> 3) | ((rightColour >> 2) & 2);
    }
    bool operator==(Block other) const { return _index == other._index; }
private:
    int _index;
};

//struct WaveformPoint
//{
//public:
//
//private:
//    int _n;
//    double _total;
//    double _total2;
//};

class CalibrateWindow;

template<class T> class CalibrateBitmapWindowTemplate : public BitmapWindow
{
public:
    CalibrateBitmapWindowTemplate()
      : _rsqrt2(1/sqrt(2.0)), _doneCapture(false)
    {
        _waveforms = Bitmap<Byte>(Vector(16*4*256, 16*4*256));
        _waveforms.fill(0);
    }
    ~CalibrateBitmapWindowTemplate()
    {
        AutoHandle h = File("output.dat").openWrite();
        h.write(reinterpret_cast<Byte*>(&_tSamples[0]), 2048*sizeof(double));
    }
    void setCalibrateWindow(CalibrateWindow* window)
    {
        _calibrateWindow = window;
    }

    void create()
    {
        _vbiCapPipe = File("\\\\.\\pipe\\vbicap", true).openPipe();
        _vbiCapPipe.write<int>(1);

        int samples = 450*1024;
        int sampleSpaceBefore = 256;
        int sampleSpaceAfter = 256;
        _buffer.allocate(sampleSpaceBefore + samples + sampleSpaceAfter);
        _b = &_buffer[0] + sampleSpaceBefore;
        for (int i = 0; i < sampleSpaceBefore; ++i)
            _b[i - sampleSpaceBefore] = 0;
        for (int i = 0; i < sampleSpaceAfter; ++i)
            _b[i + samples] = 0;
        _decoder.setInputBuffer(_b);


        setSize(Vector(1536, 1024));

        double brightness = -0.124;
        double contrast = 1.052;
        _aPower = 0;

        AutoHandle ht = File("q:\\Pictures\\reenigne\\top.raw", true).openRead();
        AutoHandle hb = File("q:\\Pictures\\reenigne\\bottom.raw", true).openRead();

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
            ht.read(&bt[i*1024], 1024);
            hb.read(&bb[i*1024], 1024);
        }
        _top = Bitmap<ColourHF>(Vector(760, 240));
        _bottom = Bitmap<ColourHF>(Vector(760, 240));

        NTSCDecoder decoder;
        decoder.brightness = -11.0;
        decoder.contrast = 1.41;
        decoder.hue = 0;
        decoder.saturation = 0.303;
        decoder.decode(bt, _top);
        decoder.decode(bb, _bottom);

        _output = Bitmap<SRGB>(Vector(1536, 1024));
        _rgb = ColourSpace::rgb();
        _srgb = ColourSpace::srgb();

        drawCaptures();

        for (int i = 0; i < 2048; ++i)
            _tSamples[i] = 0.5;
        for (int i = 0; i < 8; ++i)
            _iSamples[i] = 0.125;

        _sliders[0] = MySlider(0, 2, 0.5655, "saturation", &_saturation);
        _sliders[1] = MySlider(-180, 180, 0, "hue", &_hue, false);
        _sliders[2] = MySlider(-1, 1, 0, "brightness", &_brightness, false);
        _sliders[3] = MySlider(0, 2, 1, "contrast", &_contrast, false);
        _sliders[4] = MySlider(0, 1, "I On 0", &_iSamples[6], true);
        _sliders[5] = MySlider(0, 1, "I On 1", &_iSamples[7], true);
        _sliders[6] = MySlider(0, 1, "I Rising 0", &_iSamples[4], true);
        _sliders[7] = MySlider(0, 1, "I Rising 1", &_iSamples[5], true);
        _sliders[8] = MySlider(0, 1, "I Falling 0", &_iSamples[2], true);
        _sliders[9] = MySlider(0, 1, "I Falling 1", &_iSamples[3], true);
        _sliders[10] = MySlider(0, 1, "I Off 0", &_iSamples[0], true);
        _sliders[11] = MySlider(0, 1, "I Off 1", &_iSamples[1], true);
        _sliders[12] = MySlider(0, 2, 1, "Sharpness", &_sharpness, false);
        _sliders[13] = MySlider(-1, 1, 0, "hf phase", &_hfPhase, true);
        _baseSliders = 14;
        _sliderCount = _baseSliders + 8;

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
        _waveformTL = Vector(1032, (_baseSliders + 32 + 4)*8);
        for (int x = -1; x <= 256; ++x) {
            _output[_waveformTL + Vector(-1, x)] = white;
            _output[_waveformTL + Vector(256, x)] = white;
            _output[_waveformTL + Vector(x, -1)] = white;
            _output[_waveformTL + Vector(x, 256)] = white;
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
        for (int i = 0; i < 4096; ++i)
            drawBlock(_computes[i], i, 8);

        write(_output.subBitmap(Vector(1024, (_baseSliders + 32 + 1)*8), Vector(512, 8)), "Fitness", _fitness);
        write(_output.subBitmap(Vector(1024, (_baseSliders + 32 + 2)*8), Vector(512, 8)), "Delta", _aPower);

        Vector p = _attribute << 6;
        SRGB white(255, 255, 255);

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
            ColourHF chf = getPixel4(b.foreground(), b.background(), b.bits());
            _captures[i] = chf;
            drawBlock(chf, i, 0);
        }
    }

    void drawBlock(ColourHF colour, int i, int y)
    {
        Block b(i);
        Vector p = b.vector();
        for (int xx = 0; xx < 16; ++xx)
            for (int yy = 0; yy < 8; ++yy) {
                int h;
                switch (xx & 3) {
                    case 0:	h = colour.hf.x; break;
                    case 1: h = colour.hf.y; break;
                    case 2: h = -colour.hf.x; break;
                    case 3: h = -colour.hf.y; break;
                }
                //h *= 256;
                SRGB c = _srgb.toSrgb24(colour.c);
                c = SRGB(byteClamp(c.x + h), byteClamp(c.y + h), byteClamp(c.z + h));
                _output[p + Vector(xx, yy + y)] = c;
            }

    }

    void newAttribute()
    {
        int fg = _attribute.x;
        int bg = _attribute.y;
        if (_attribute.x == _attribute.y)
            _sliderCount = _baseSliders + 8;
        else
            _sliderCount = _baseSliders + 32;
        for (int i = _baseSliders; i < _sliderCount; ++i) {
            _sliders[i] = MySlider(0, 1, "", &_tSamples[transitionForSlider(i).index()*2 + (i&1)]);
            _sliders[i].setBitmap(_output.subBitmap(Vector(1032, i*8), Vector(512, 8)));
            _sliders[i].draw();
        }
    }
    Transition transitionForSlider(int slider)
    {
        int fg = _attribute.x;
        int bg = _attribute.y;
        slider -= _baseSliders;
        slider >>= 1;
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
            _waveform = position >> 4;
            newWaveform();
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
            _optimizing[i] = true;
        }
    }

    void pause() { _paused = !_paused; }

    bool idle()
    {
        if (!doCapture())
            return true;

        if (_paused)
            return false;
        bool climbed = false;
        for (int i = 0; i < _sliderCount*2; ++i) {
            // Pick a slider
            MySlider* slider = &_sliders[i >> 1];
            if (!slider->use())
                continue;

            double oldFitness = _fitness;
            ColourHF oldComputes[4096];
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
    void newWaveform()
    {
        for (int y = 0; y < 256; ++y)
            for (int x = 0; x < 256; ++x) {
                Vector p(x,y);
                Byte b = _waveforms[(_waveform << 8) + p];
                double v = 255*(1-exp(-0.693*b));
                _output[p] = SRGB(v, v, v);
            }
    }

    bool doCapture()
    {
        if (_doneCapture)
            return true;
        _vbiCapPipe.read(_b, 1024*450);
        _decoder.decode();

        for (int i = 0; i < 4096; ++i) {
            Block b(i);
            int fg = b.foreground();
            int bg = b.background();
            int patch;
            int line;
            int set;
            switch (b.bits()) {
                case 0x00: patch = 2; line = 0; set = (bg << 4) | bg; break;
                case 0x01: patch = 0; line = 1; set = (fg << 4) | bg; break;
                case 0x02: patch = 1; line = 0; set = (bg << 4) | fg; break;
                case 0x03: patch = 2; line = 0; set = (fg << 4) | bg; break;
                case 0x04: patch = 5; line = 3; set = (fg << 4) | bg; break;
                case 0x05: patch = 3; line = 0; set = (bg << 4) | fg; break;
                case 0x06: patch = 4; line = 0; set = (bg << 4) | fg; break;
                case 0x07: patch = 1; line = 1; set = (fg << 4) | bg; break;
                case 0x08: patch = 1; line = 1; set = (bg << 4) | fg; break;
                case 0x09: patch = 4; line = 0; set = (fg << 4) | bg; break;
                case 0x0a: patch = 3; line = 0; set = (fg << 4) | bg; break;
                case 0x0b: patch = 5; line = 3; set = (bg << 4) | fg; break;
                case 0x0c: patch = 2; line = 0; set = (bg << 4) | fg; break;
                case 0x0d: patch = 1; line = 0; set = (fg << 4) | bg; break;
                case 0x0e: patch = 0; line = 1; set = (bg << 4) | fg; break;
                case 0x0f: patch = 2; line = 0; set = (fg << 4) | fg; break;
            }
            Vector p;
            switch (set % 3) {
                case 0:
                    p = Vector(0, 0);
                    break;
                case 1:
                    if (patch < 3)
                        p = Vector(6, 0);
                    else
                        p = Vector(-3, 1);
                    break;
                case 2:
                    p = Vector(3, 1);
                    break;
            }
            p += Vector(patch + line*10, (set/3)*2);
            p = Vector(p.x*32 + 221, p.y + 17);
            Byte* bp;
            double o;
            _decoder.getSamples(p.x, p.y, &bp, &o);
        }

        return false;
    }

    void integrate(Block b, double* dc, Complex<double>* iq, Complex<double>* hf)
    {
        double s[8];
        for (int t = 0; t < 4; ++t) {
            s[t*2] = _tSamples[b.transition(t).index()*2];
            s[t*2] += _iSamples[b.iState(t)*2];
            s[t*2 + 1] = _tSamples[b.transition(t).index()*2 + 1];
            s[t*2 + 1] += _iSamples[b.iState(t)*2 + 1];
        }
        *dc = (s[0] + s[1] + s[2] + s[3] + s[4] + s[5] + s[6] + s[7])/8;
        iq->x = ((s[0] - s[4]) + _rsqrt2*((s[1]+s[7])-(s[3]+s[5])))/8;
        iq->y = ((s[2] - s[6]) + _rsqrt2*((s[1]+s[3])-(s[5]+s[7])))/8;
        hf->x = ((s[0] + s[4]) - (s[2] + s[6]))/8;
        hf->y = ((s[1] + s[5]) - (s[3] + s[7]))/8;
    }

    void computeFitness()
    {
        double dc;
        Complex<double> iqBurst;
        Complex<double> hf;
        integrate(Block(6, 6, 0), &dc, &iqBurst, &hf);
        Complex<double> iqAdjust;
        Complex<double> hfAdjust;
        if (iqBurst.modulus2() < 1.0e-6) {
            iqAdjust = 0;
//			hfAdjust = 0; //unit(_hfPhase)*_contrast*_sharpness;
            _fitness = 4096.0e6;
        }
        else {
            iqAdjust = -iqBurst.conjugate()*unit((33 + 90 + _hue)/360.0)*_saturation*_contrast/iqBurst.modulus();
//			hfAdjust = unit(_hfPhase)*iqAdjust*iqAdjust*_sharpness/(_saturation*_saturation*_contrast);
            _fitness = 0;
        }
        hfAdjust = unit(_hfPhase)*_contrast*_sharpness;

        for (int bn = 0; bn < 4096; ++bn) {
            Block block(bn);
            if (_optimizing[bn]) {
                Complex<double> iq;
                integrate(block, &dc, &iq, &hf);
                double y = dc*_contrast + _brightness;
                iq *= iqAdjust;

                double r = 255*(y + 0.9563*iq.x + 0.6210*iq.y);
                double g = 255*(y - 0.2721*iq.x - 0.6474*iq.y);
                double b = 255*(y - 1.1069*iq.x + 1.7046*iq.y);
                ColourHF c;
                c.c = Colour(r, g, b);
                c.hf = hf*255*hfAdjust;
                _computes[bn] = c;

                double f = (c - _captures[bn]).modulus2();
                _fitnesses[bn] = f;
            }

            _fitness += _fitnesses[bn];
        }
        _fitness /= 4096;
    }

    Array<Byte> _buffer;
    Byte* _b;
    AutoHandle _vbiCapPipe;
    bool _doneCapture;
    NTSCDecoder _decoder;

    Bitmap<Byte> _waveforms;
    Vector _waveform;

    double _rsqrt2;

    Bitmap<DWORD> _bitmap;

    Bitmap<ColourHF> _top;
    Bitmap<ColourHF> _bottom;
    Bitmap<SRGB> _output;
    ColourSpace _rgb;
    ColourSpace _srgb;

    double _ab;
    Complex<double> _qamAdjust;

    MySlider _sliders[46];
    int _slider;
    int _sliderCount;
    int _baseSliders;

    Vector _dragStart;
    int _dragStartX;

    double _saturation;
    double _hue;
    double _brightness;
    double _contrast;
    double _sharpness;
    double _hfPhase;

    ColourHF _captures[4096];
    ColourHF _computes[4096];
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

    double _tSamples[2048];
    double _iSamples[8];

    int _aPower;

    int _buttons;
    Vector _position;

    int _iteration;

    bool _doneClimb;

    CalibrateWindow* _calibrateWindow;

};

typedef CalibrateBitmapWindowTemplate<void> CalibrateBitmapWindow;

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
