#include "alfe/main.h"
#include "alfe/bitmap_png.h"
#include "alfe/terminal6.h"
#include "alfe/complex.h"

class NTSCCaptureDecoder
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
            Complex<double> burst = 0;
            double burstDC = 0;
            for (int i = firstBurstSample; i < firstBurstSample + burstSamples; ++i) {
                int j = p + i;                                   // == -8
                int sample = b[j];
                burst += unit((i&7)/8.0)*sample;
                burstDC += sample;
            }
            burst /= burstSamples;
            burstDC /= burstSamples;
            double burstAmplitude = burst.modulus();
            _burstAmplitude[line] = burstAmplitude;
            _burstDC[line] = burstDC;

            double burstPhase = 4 - 8*burst.argument()/tau;  // 0 to 8

            int ibp = static_cast<int>(burstPhase);
            _p[line] = b+p;
            _o[line] = burstPhase;

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
                _v[line] = ((_p[line] - _p[line - 1]) - (_o[line] - _o[line - 1]))/nominalSamplesPerLine;
        }
        _v[0] = _v[1];

        double averageVelocity = 0;
        for (int line = 10; line < 240; ++line)
            averageVelocity += nominalSamplesPerLine*(1 - _v[line]);
        averageVelocity /= 230;
        Complex<double> burstDCRotor = 0;
        Complex<double> burstAmplitudeRotor = 0;
        for (int line = 10; line < 240; ++line) {
            Complex<double> u = unit(line*averageVelocity);
            burstDCRotor += _burstDC[line]*u;
            burstAmplitudeRotor += _burstAmplitude[line]*u;
        }
        burstDCRotor = 2.0*burstDCRotor.conjugate()/230;
        burstAmplitudeRotor = 2.0*burstAmplitudeRotor.conjugate()/230;
        for (int line = 0; line < 240; ++line) {
            Complex<double> u = unit(line*averageVelocity);
            _burstDC[line] -= (burstDCRotor*u).x;
            _burstAmplitude[line] -= (burstAmplitudeRotor*u).x;
        }
    }

    // Get a pointer to the samples that are on scanline y at 8*x color carrier
    // cycles from end of hsync pulse. *p is a pointer to the next sample after
    // that point. *o is the distance between x and where *p is sampled, in
    // eights of a color carrier cycle. 0<=*o<1
    double getSample(double x, int y, double* o)
    {
        Byte* pp = _p[y];  // Sample at x = 0
        double oo = _v[y]*x - _o[y];
        int i = static_cast<int>(oo);
        *o = static_cast<double>(i) - oo;
        unsigned int z = pp[i];
        return (z - _burstDC[y])/_burstAmplitude[y];
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
    MySlider(double low, double high, double initial, String caption, double* p, bool use)
      : _low(low), _high(high), _caption(caption), _p(p), _max(512), _use(use)
    {
        *p = initial;
        //_dragStartX = static_cast<int>((initial - low)*_max/(high - low));
    }
    MySlider(double low, double high, String caption, double* p, bool use)
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

struct ColourHF
{
    const ColourHF& operator-=(const ColourHF& other)
    {
        y -= other.y;
        iq -= other.iq;
        hf -= other.hf;
        return *this;
    }
    const ColourHF& operator+=(const ColourHF& other)
    {
        y += other.y;
        iq += other.iq;
        hf += other.hf;
        return *this;
    }
    const ColourHF& operator/=(const double& other)
    {
        y /= other;
        iq /= other;
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
    double modulus2() const { return y*y + iq.modulus2() + hf.modulus2(); }

    double y;
    Complex<double> iq;
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

class WaveformPoint
{
public:
    WaveformPoint() : _n(0), _total(0), _enough(false) { }
    bool add(double v)
    {
        ++_n;
        _total += v;
        if (!_enough && _n >= 10) {
            _enough = true;
            return true;
        }
        return false;
    }
    double value() { return _total/_n; }
private:
    double _total;
    int _n;
    bool _enough;
};

class Waveform
{
public:
    Waveform() : /*_y(0), _iq(0), _hf(0), _n(0),*/ _completed(0)
    {
        _points.allocate(0x100);
    }
    bool add(double x, double yy)
    {
        //++_n;
        //_y += yy;
        //_iq += yy*unit(x/8);
        //_hf += yy*unit(x/4);
        if (_points[static_cast<int>(x*0x20) & 0xff].add(yy)) {
            ++_completed;
            if (_completed == 0x100)
                return true;
        }
        return false;
    }
    double getValue(int x) { return _points[x].value(); }

    //double _y;
    //Complex<double> _iq;
    //Complex<double> _hf;
    //int _n;
    int _completed;
    Array<WaveformPoint> _points;
};

class CalibrateWindow;

template<class T> class CalibrateBitmapWindowT : public BitmapWindow
{
public:
    CalibrateBitmapWindowT()
      : _rsqrt2(1/sqrt(2.0)), _doneCapture(false), _graph(0, 0), _completed(0)
    {
        _graphs = Bitmap<Byte>(Vector(16*4*256, 16*4*256));
        _graphs.fill(0);
    }
    void setCalibrateWindow(CalibrateWindow* window)
    {
        _calibrateWindow = window;
    }

    void create()
    {
        _outputStream = File("output.dat").openWrite();
        for (int i = 0; i < 4096; ++i)
            _fitnesses[i] = 1000000;

//        _vbiCapPipe = File("\\\\.\\pipe\\vbicap", true).openPipe();
//        _vbiCapPipe.write<int>(1);
        _vbiCapPipe = File("captured.png.raw", true).openRead();

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

        double brightness = -0.124;
        double contrast = 1.052;
        _aPower = 0;

        _output = Bitmap<SRGB>(Vector(1536, 1024));
        _rgb = ColourSpace::rgb();
        _srgb = ColourSpace::srgb();

        reset();

        _sliders[0] = MySlider(0, 2, 0.5655, "saturation", &_saturation, true);
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
        _sliders[12] = MySlider(0, 2, 1, "Sharpness", &_sharpness, true);
        _sliders[13] = MySlider(0, 1, 0.5, "Transition", &_transition, false);
        _sliders[14] = MySlider(0, 2, 1, "Disp Sat.", &_displaySaturation, false);
        _sliders[15] = MySlider(0, 2, 1, "Disp Sharp.", &_displaySharpness, false);
        _baseSliders = 16;
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
        _graphTL = Vector(1032, (_baseSliders + 32 + 4)*8);
        for (int x = -1; x <= 256; ++x) {
            _output[_graphTL + Vector(-1, x)] = white;
            _output[_graphTL + Vector(256, x)] = white;
            _output[_graphTL + Vector(x, -1)] = white;
            _output[_graphTL + Vector(x, 256)] = white;
        }

        _paused = false;

        setInnerSize(Vector(1536, 1024));
        BitmapWindow::create();

        _iteration = 0;
        _doneClimb = false;

        _eight = true;
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

        for (int i = 0; i < 4096; ++i) {
            drawBlock(_captures[i], i, 0);
            drawBlock(_computes[i], i, 8);
        }

        write(_output.subBitmap(Vector(1024, (_baseSliders + 32 + 1)*8), Vector(512, 8)), "Fitness", _fitness);
        write(_output.subBitmap(Vector(1024, (_baseSliders + 32 + 2)*8), Vector(512, 8)), "Delta", _aPower);

        newGraph();

        Vector p = _attribute << 6;
        SRGB white(255, 255, 255);

        // Copy the _output bitmap to the Image
        Vector zero(0, 0);
        Vector sz = innerSize();
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

    void drawBlock(ColourHF colour, int i, int yp)
    {
        double y = colour.y*_contrast + _brightness*256;
        Complex<double> iq = colour.iq*unit(_hue)*_displaySaturation*_contrast;
        Complex<double> hf = colour.hf*_displaySharpness;

        double r = y + 0.9563*iq.x + 0.6210*iq.y;
        double g = y - 0.2721*iq.x - 0.6474*iq.y;
        double b = y - 1.1069*iq.x + 1.7046*iq.y;
        SRGB c = _srgb.toSrgb24(Colour(r, g, b));
        if (yp == 0)
            _captureColours[i] = c;

        Vector p = Block(i).vector();
        for (int xx = 0; xx < 16; ++xx) {
            int h;
            switch (xx & 3) {
                case 0:	h = static_cast<int>(hf.x); break;
                case 1: h = static_cast<int>(hf.y); break;
                case 2: h = -static_cast<int>(hf.x); break;
                case 3: h = -static_cast<int>(hf.y); break;
            }
            SRGB c1(byteClamp(c.x + h), byteClamp(c.y + h), byteClamp(c.z + h));
            for (int yy = 0; yy < 8; ++yy)
                _output[p + Vector(xx, yy + yp)] = c1;
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
            _sliders[i] = MySlider(0, 1, "", &_tSamples[transitionForSlider(i).index()*2 + (i&1)], true);
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
            _graph = position >> 4;
            newGraph();
            //_entireAttribute = ((position.y & 15) < 8);
            //_attribute = position >> 6;
            //newAttribute();
            invalidate();
        }
        if ((position - _graphTL).inside(Vector(256, 256))) {
            _graph = Vector(-1, -1);
            newGraph();
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
                //if (value < slider->low())
                //    value = slider->low();
            }
            else {
                value = oldValue + amount;
                //if (value > slider->high())
                //    value = slider->high();
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
                        if (_aPower == 16) { //30) {
                            _outputStream.write(reinterpret_cast<Byte*>(&_tSamples[0]), 2048*sizeof(double));
                            _outputStream.write(reinterpret_cast<Byte*>(&_iSamples[0]), 8*sizeof(double));
                            _outputStream.write(reinterpret_cast<Byte*>(&_sharpness), sizeof(double));
                            _outputStream.write(reinterpret_cast<Byte*>(&_saturation), sizeof(double));

                            double colourFitness = 0;
                            for (int bn = 0; bn < 4096; ++bn) {
                                Block block(bn);
                                ColourHF capture = _captures[bn];
                                capture.hf *= _sharpness;
                                capture.iq *= _saturation;
                                ColourHF delta = integrate(block) - capture;
                                colourFitness += delta.y*delta.y + delta.iq.modulus2();
                            }
                            printf("Transition %f fitness %f colour fitness %f eight %i\n",_transition,_fitness,colourFitness/4096,_eight);
                            reset();
                            _fitness = 1000000;
                            _eight = !_eight;
                            if (_eight) {
                                _transition += (sqrt(5.0)-1)/2;
                                if (_transition > 1)
                                    _transition -= 1;
                            }
                            _aPower = 0;
                            integrateCaptures();
                        }
                    }
                    _doneClimb = false;
                }
            }
            newAttribute();
        }
        return true;
    }

private:
    void reset()
    {
        _sharpness = 1;
        _saturation = 1;
        for (int i = 0; i < 2048; ++i)
            _tSamples[i] = 0.5;
        for (int i = 0; i < 8; ++i)
            _iSamples[i] = 0.125;
    }
    void newGraph()
    {
        //if (_doneCapture) {
        //    influenceGraph();
        //    return;
        //}
        if (_graph.x == -1)
            return;
        //for (int y = 0; y < 256; ++y)
        //    for (int x = 0; x < 256; ++x) {
        //        Vector p(x,y);
        //        Byte b = _graphs[(_graph << 8) + p];
        //        double v = 255*(1-exp(-0.05*b));
        //        _output[p + _graphTL] = SRGB(v, v, v);
        //    }
        for (int y = 0; y < 256; ++y)
            for (int x = 0; x < 256; ++x) {
                Vector p(x, y);
                _output[p + _graphTL] = SRGB(0, 0, 0);
            }
        Block b(_graph << 4);
        for (int x = 0; x < 256; ++x) {
            double yy = _waveforms[b.index()].getValue(x);
            int y = byteClamp(255 - yy);
            Vector p(x, y);
            _output[p + _graphTL] = SRGB(255, 255, 255);
        }
        double s[8];
        getCaptureSamples(b.index(), s);
        for (int t = 0; t < 8; ++t) {
            Vector p(t*32 + 16, byteClamp(255 - s[t]));
            _output[p + _graphTL] = SRGB(255, 0, 0);
        }
        getComputeSamples(b.index(), s);
        for (int t = 0; t < 8; ++t) {
            Vector p(t*32 + 16, byteClamp(255 - s[t]));
            _output[p + _graphTL] = SRGB(0, 255, 0);
        }
    }
    void influenceGraph()
    {
        // Draw the pixel graph
        Array<double> pixelGraph(256);
        for (int y = 0; y < 0x100; ++y)
            for (int x = 0; x < 0x100; ++x)
                _output[Vector(x, y) + _graphTL] = SRGB(0, 0, 0);
        for (int pixel = 0; pixel < 4; ++pixel) {
            int bit = 1 << pixel;
            for (int x = 0; x < 0x100; ++x)
                pixelGraph[x] = 0;
            int n = 0;
            for (int i = 0; i < 0x1000; ++i) {
                int j = i & 0xf77;
                if (_graph.x != -1) {
                    j = Block(_graph << 4).index();
                    if (_entireAttribute)
                        j = Block(Block(j).foreground(), Block(j).background(), Block(i).bits()).index();
                }

                Block b1(j);
                Block b2(b1.foreground(), b1.background(), b1.bits() ^ bit);
                for (int x = 0; x < 0x100; ++x) {
                    double d = _waveforms[j].getValue(x) -
                        _waveforms[b2.index()].getValue(x);
                    pixelGraph[x] += abs(d);
                }
            }
            for (int x = 0; x < 0x100; ++x) {
                int yy = 255 - static_cast<int>((pixelGraph[x]/0x1000));
                for (int y = 0; y < 0x100; ++y)
                    if (y >= yy)
                        _output[Vector(x, y) + _graphTL].x |= bit;
            }
        }
        for (int y = 0; y < 0x100; ++y)
            for (int x = 0; x < 0x100; ++x) {
                Vector p = Vector(x, y) + _graphTL;
                int i = Block(0xf, 0, (_output[p]).x).index();
                _output[p] = _captureColours[i];
            }
    }

    bool doCapture()
    {
        if (_doneCapture)
            return true;
        static bool doneRead = false;
        if (!doneRead) {
            _vbiCapPipe.read(_b, 1024*450);
            doneRead = true;
        }
        _decoder.decode();

        for (int i = 0; i < 4096; ++i) {
            Block b(i);
            int fg = b.foreground();
            int bg = b.background();
            int bits = b.bits();
            int patch;
            int line;
            int set;
            if (fg < bg) {
                int t = fg;
                fg = bg;
                bg = t;
                bits = !bits;
            }

            Vector p;
            p.x = b.bits()*80;
            static const int yTable[16] = {2, 18, 33, 47, 60, 72, 83, 93,
                102, 110, 117, 123, 128, 132, 135, 137};
            p.y = fg + yTable[bg];
            p += Vector(240, 30);
            double o;
            for (int j = 0; j < 9; ++j) {
                double yy = _decoder.getSample(p.x + j, p.y, &o)*48 - 560;
                double xx = j + o;
                int y = static_cast<int>(255-yy);
                if (y < 0)
                    y = 0;
                if (y > 255)
                    y = 255;
                int x = static_cast<int>(xx*32 + 0.5) & 0xff;
                Vector gv = (b.vector()<<4) + Vector(x, y);
                if (_graphs[gv] < 255)
                    ++_graphs[gv];
                if (_waveforms[i].add(xx, yy)) {
                    ++_completed;
                    if (_completed == 4096)
                        _doneCapture = true;
                }
            }
        }
        newGraph();
        invalidate();

        if (!_doneCapture)
            return false;

        integrateCaptures();
        computeFitness();

        return false;
    }

    void integrateCaptures()
    {
        for (int i = 0; i < 4096; ++i)
            _captures[i] = integrateCapture(i);
    }

    ColourHF integrate(double* s)
    {
        ColourHF c;
        if (_eight) {
            //c.y = (s[0] + s[1] + s[2] + s[3] + s[4] + s[5] + s[6] + s[7])/8;
            //c.iq.x = ((s[0] - s[4]) + _rsqrt2*((s[1]+s[7])-(s[3]+s[5])))/8;
            //c.iq.y = ((s[2] - s[6]) + _rsqrt2*((s[1]+s[3])-(s[5]+s[7])))/8;
            //c.hf.x = ((s[0] + s[4]) - (s[2] + s[6]))/8;
            //c.hf.y = ((s[1] + s[5]) - (s[3] + s[7]))/8;
            c.y = (s[0] + s[2] + s[4] + s[6])/4;
            c.iq.x = (s[0] - s[4])/4;
            c.iq.y = (s[2] - s[6])/4;
            c.hf.x = ((s[0] + s[4]) - (s[2] + s[6]))/4;
            c.hf.y = 0;
        }
        else {
            c.y = (s[0] + s[2] + s[4] + s[6])/4;
            c.iq.x = (s[0] - s[4])/4;
            c.iq.y = (s[2] - s[6])/4;
            c.hf.x = 0;
            c.hf.y = 0;
        }
        return c;
    }

    void getComputeSamples(Block b, double* s)
    {
        for (int t = 0; t < 4; ++t) {
            s[t*2] = _tSamples[b.transition(t).index()*2]*256;
            s[t*2] += _iSamples[b.iState(t)*2]*256;
            s[t*2 + 1] = _tSamples[b.transition(t).index()*2 + 1]*256;
            s[t*2 + 1] += _iSamples[b.iState(t)*2 + 1]*256;
        }
    }

    void getCaptureSamples(int i, double* s)
    {
        double tt = _transition*2 - 1;
        Waveform* w = &_waveforms[i];
        for (int t = 0; t < 8; ++t) {
            double x = t*32 + tt*32 + 256;
            int xx = static_cast<int>(x);
            double y0 = w->getValue(xx & 0xff);
            double y1 = w->getValue((xx + 1) & 0xff);
            x -= xx;
            s[t] = y0*(1-x)+y1*x;
        }
    }

    ColourHF integrate(Block b)
    {
        double s[8];
        getComputeSamples(b, s);
        return integrate(s);
    }

    ColourHF integrateCapture(int i)
    {
        double s[8];
        getCaptureSamples(i, s);
        return integrate(s);
    }

    void computeFitness()
    {
        _fitness = 0;
        for (int bn = 0; bn < 4096; ++bn) {
            Block block(bn);
            if (_optimizing[bn]) {
                ColourHF capture = _captures[bn];
                capture.hf *= _sharpness;
                capture.iq *= _saturation;
                ColourHF compute = integrate(block);
                _computes[bn] = compute;
                ColourHF delta = compute - capture;
                _fitnesses[bn] = delta.modulus2();
            }
            _fitness += _fitnesses[bn];
        }
        _fitness /= 4096;
    }

    Waveform _waveforms[4096];
    int _completed;

    Array<Byte> _buffer;
    Byte* _b;
    AutoStream _vbiCapPipe;

    bool _doneCapture;
    NTSCCaptureDecoder _decoder;

    Bitmap<Byte> _graphs;
    Vector _graph;
    bool _entireAttribute;

    double _rsqrt2;

    Bitmap<DWORD> _bitmap;

    Bitmap<SRGB> _output;
    ColourSpace _rgb;
    ColourSpace _srgb;

    double _ab;
    Complex<double> _qamAdjust;

    MySlider _sliders[48];
    int _slider;
    int _sliderCount;
    int _baseSliders;

    Vector _dragStart;
    int _dragStartX;

    double _saturation;
    double _hue;
    double _brightness;
    double _contrast;
    double _displaySharpness;
    double _displaySaturation;
    double _sharpness;
    double _transition;
    bool _eight;

    ColourHF _captures[4096];
    ColourHF _computes[4096];
    SRGB _captureColours[4096];
    double _fitness;

    Block _clicked;
    Vector _graphTL;
    Vector _attribute;
    bool _optimizing[4096];
    double _fitnesses[4096];

    bool _paused;

    double _tSamples[2048];
    double _iSamples[8];

    int _aPower;

    int _buttons;
    Vector _position;

    int _iteration;

    bool _doneClimb;

    CalibrateWindow* _calibrateWindow;

    AutoStream _outputStream;
};

typedef CalibrateBitmapWindowT<void> CalibrateBitmapWindow;

class CalibrateWindow : public RootWindow
{
public:
    CalibrateWindow()
    {
        _bitmap.setCalibrateWindow(this);

        add(&_bitmap);
        add(&_animated);

        _animated.setDrawWindow(&_bitmap);
        _animated.setRate(1);
    }
    void restart() { _animated.restart(); }
    void create()
    {
        setText("CGA Calibration");
        setInnerSize(Vector(1536, 1024));
        _bitmap.setTopLeft(Vector(0, 0));
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
