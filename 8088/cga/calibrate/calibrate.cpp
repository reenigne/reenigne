#include "alfe/main.h"
#include "alfe/bitmap_png.h"
#include "alfe/terminal6.h"
#include "alfe/complex.h"

class CalibrateImage;

typedef RootWindow<Window> RootWindow2;
typedef ImageWindow<RootWindow2, CalibrateImage> ImageWindow2;

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

class Slider
{
public:
    Slider() { }
    Slider(double low, double high, double initial, String caption, double* p, bool use = true)
      : _low(low), _high(high), _caption(caption), _p(p), _max(512), _use(use)
    {
        *p = initial;
        //_dragStartX = static_cast<int>((initial - low)*_max/(high - low));
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
private:
    int _index;
};

class CalibrateImage : public Image
{
public:
    void setWindow(ImageWindow2* window)
    {
        _window = window;

        double brightness = -0.124;
        double contrast = 1.052;

        _top = Bitmap<SRGB>(Vector(760, 240));
        _bottom = Bitmap<SRGB>(Vector(760, 240));
        Stream topStream = File("q:\\top_decoded.raw", true).openRead();
        Stream bottomStream = File("q:\\bottom_decoded.raw", true).openRead();
        topStream.read(_top.data(), 760*240*3);
        bottomStream.read(_bottom.data(), 760*240*3);
        _topRaw.allocate(450*1024);
        _bottomRaw.allocate(450*1024);
        Stream topRawStream = File("q:\\top.raw", true).openRead();
        oStream bottomRawStream = File("q:\\bottom.raw", true).openRead();
        topRawStream.read(&_topRaw[0], 450*1024);
        bottomRawStream.read(&_bottomRaw[0], 450*1024);

        _output = Bitmap<SRGB>(Vector(1536, 1024));
        _rgb = ColourSpace::rgb();
        _srgb = ColourSpace::srgb();

        for (int i = 0; i < 4096; ++i) {
            Block b(i);
            Colour rgb = getPixel4(b.foreground(), b.background(), b.bits());
            _captures[i] = rgb;
            SRGB c = _rgb.toSrgb24(rgb);
            Vector p = b.vector();
            for (int xx = 0; xx < 16; ++xx)
                for (int yy = 0; yy < 8; ++yy)
                    _output[p + Vector(xx, yy)] = c;
        }

        Colour directColours[16];
        Colour directColoursSRGB[16];
        double dcVoltage[16];
        for (int c = 0; c < 16; ++c) {
            directColours[c] = Colour(0, 0, 0);
            for (int b = 0; b < 16; ++b)
                directColours[c] += _captures[Block(c, c, b).index()];
            directColours[c] /= 16;
            Colour srgb = _rgb.toSrgb(directColours[c]);
            directColoursSRGB[c] = srgb;
            dcVoltage[c] = (srgb.x*0.299 + srgb.y*0.587 + srgb.z*0.114)/256.0;
        }
        _voltages[0] = dcVoltage[0];
        _voltages[1] = dcVoltage[8];
        _voltages[2] = dcVoltage[7];
        _voltages[3] = dcVoltage[15];

        _sampleScale = (_voltages[3] - _voltages[0])*256/(161 - 38);
        _sampleOffset = _voltages[0]*256 - _sampleScale*38;

        _topPhase = 0;
        _topPhase = drawSamples2(Vector(-1, 0), false).argument()*8/tau;
        _bottomPhase = 0;
        _bottomPhase = drawSamples2(Vector(-1, 100), false).argument()*8/tau;

        _sliderCount = 25;
        _sliders[0] = Slider(0, 1, 0.5655, "saturation", &_saturation);
        _sliders[1] = Slider(-180, 180, 0, "hue", &_hue, false);
        _sliders[2] = Slider(-1, 1, 0, "brightness", &_brightness, false);
        _sliders[3] = Slider(0, 2, 1, "contrast", &_contrast, false);
        _sliders[4] = Slider(0, 70, 3.882, "U6F rise", &_u6f.x);
        _sliders[5] = Slider(0, 70, 6.699, "U6F fall", &_u6f.y, false);
        _sliders[6] = Slider(0, 70, 8.066, "U45 data rise", &_u45Data.x);
        _sliders[7] = Slider(0, 70, 1.234, "U45 data fall", &_u45Data.y);
        _sliders[8] = Slider(0, 70, 29.5, "U45 select rise", &_u45Select.x);
        _sliders[9] = Slider(0, 70, 15.7, "U45 select fall", &_u45Select.y);
        _sliders[10] = Slider(0, 70, 7.109, "U68B rise", &_u68b.x);
        _sliders[11] = Slider(0, 70, 4.852, "U68B fall", &_u68b.y);
        _sliders[12] = Slider(0, 70, 7.039, "U68A rise", &_u68a.x);
        _sliders[13] = Slider(0, 70, 4.812, "U68A fall", &_u68a.y);
        _sliders[14] = Slider(0, 70, 0, "74S174 rise", &_ic74s174.x);
        _sliders[15] = Slider(0, 70, 4.192, "74S174 fall", &_ic74s174.y);
        _sliders[16] = Slider(0, 70, 6.493, "U43B rise", &_u43b.x);
        _sliders[17] = Slider(0, 70, 1.997, "U43B fall", &_u43b.y);
        _sliders[18] = Slider(0, 70, 2.244, "U44A rise", &_u44a.x);
        _sliders[19] = Slider(0, 70, 4.175, "U44A fall", &_u44a.y);
        _sliders[20] = Slider(0, 70, 7.069, "U44B rise", &_u44b.x);
        _sliders[21] = Slider(0, 70, 4.172, "U44B fall", &_u44b.y);
        _sliders[22] = Slider(0, 70, 0, "Decay time", &_decayTime);
        _sliders[23] = Slider(0, 70, 0, "U45 output rise", &_u45Output.x);
        _sliders[24] = Slider(0, 70, 0, "U45 output fall", &_u45Output.y);

        for (int i = 0; i < _sliderCount; ++i) {
            _sliders[i].setBitmap(_output.subBitmap(Vector(1032, i*8), Vector(512, 8)));
            _sliders[i].draw();
        }
        _slider = -1;

        _p14MHz = Signal::p14MHz();
        _chromas[0] = Signal::fromBits(false, false, false, false);
        _chromas[7] = Signal::fromBits(true, true, true, true);

        computeFitness();

        _clicked = Block(0);
        escapePressed();

        SRGB white(255, 255, 255);
        _waveformTL = Vector(1032, (_sliderCount + 3)*8);
        for (int x = -1; x <= 256; ++x) {
            _output[_waveformTL + Vector(-1, x)] = white;
            _output[_waveformTL + Vector(256, x)] = white;
            _output[_waveformTL + Vector(x, -1)] = white;
            //_output[_waveformTL + Vector(x, 256)] = white;
        }

        _paused = false;
    }

    void paint(const PaintHandle& paint)
    {
        draw();
        Image::paint(paint);
    }

    virtual void draw()
    {
        for (int i = 0; i < _sliderCount; ++i)
            _sliders[i].draw();

        for (int i = 0; i < 4096; ++i) {
            Block b(i);
            Vector p = b.vector();
            SRGB c = _srgb.toSrgb24(_computes[i]);
            for (int xx = 0; xx < 16; ++xx)
                for (int yy = 0; yy < 8; ++yy)
                    _output[p + Vector(xx, yy + 8)] = c;
        }

        write(_output.subBitmap(Vector(1024, (_sliderCount + 1)*8), Vector(512, 8)), "Fitness", _fitness);

        Vector p = _clicked.vector();
        SRGB white(255, 255, 255);
        for (int x = 0; x < 16; ++x) {
            _output[p + Vector(0, x)] = white;
            _output[p + Vector(15, x)] = white;
            _output[p + Vector(x, 0)] = white;
            _output[p + Vector(x, 15)] = white;
        }
        for (int y = 0; y < 256; ++y)
            for (int x = 0; x < 256; ++x)
                _output[_waveformTL + Vector(x, y)] = SRGB(0, 0, 0);

        // Draw waveform for the clicked block
        Signal chroma, intensity;
        //if (_clicked.index() != 0)
        //    printf("Non-zero\n");
        generate(_clicked, &chroma, &intensity, true);
        double y;
        Complex<double> iq;
        int x = 0;
        if (intensity._initiallyHigh)
            x ^= 1;
        if (chroma._initiallyHigh)
            x ^= 2;
        double vInitial = _voltages[x];
        double v;
        bool output = false;
        int k = 0;
        do {
            int t = 0;
            v = vInitial;
            y = 0;
            iq = 0;
            int ii = 0, ic = 0;
            do {
                int ot = t;
                int ti = -1;
                int tc = -1;
                t = Signal::cycle();
                if (ii < intensity._n) {
                    ti = intensity._t[ii];
                    if (ti < t)
                        t = ti;
                }
                if (ic < chroma._n) {
                    tc = chroma._t[ic];
                    if (tc < t)
                        t = tc;
                }
                int ox = x;
                if (t == ti) {
                    ++ii;
                    x ^= 1;
                }
                if (t == tc) {
                    ++ic;
                    x ^= 2;
                }
                double vNew = _voltages[ox];

                // Between ot and t, voltage goes from v to vNew
                // at time tt, voltage vv(tt) = vNew + (v - vNew)*exp(-(tt - ot)/tDecay)
                double tD = _decayTime*63*tau/17600;  // Convert nanoseconds to radians
                double tF = t*tau/Signal::cycle();
                double tI = ot*tau/Signal::cycle();

                if (output) {
                    int first = static_cast<int>(tI*256/tau);
                    int last = static_cast<int>(tF*256/tau);
                    for (int xx = first; xx < last; ++xx) {
                        double tt = clamp(tI, xx*tau/256, tF);
                        double vv;
                        if (tD == 0)
                            vv = vNew;
                        else
                            vv = vNew + (v - vNew)*exp(-(tt - tI)/tD);
                        int yy = 256 - static_cast<int>(vv*256);
                        if (yy < 0 || yy >= 256)
                            printf("Waveform out of bounds!\n");
                        _output[_waveformTL + Vector(xx, yy)] = white;
                    }
                }
                if (tD == 0)
                    v = vNew;
                else
                    v = vNew + (v - vNew)*exp(-(tF - tI)/tD);
            } while (t < Signal::cycle());
            ++k;
            if (output)
                break;
            if (abs(v - vInitial) <= 1/256.0 || k == 9)
                output = true;
        } while (true);

        drawSamples4(_clicked.foreground(), _clicked.background(), _clicked.bits());

        // Copy the _output bitmap to the Image
        Vector zero(0, 0);
        Vector sz = size();
        if (sz.x > 1536)
            sz.x = 1536;
        if (sz.y > 1024)
            sz.y = 1024;

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
            Slider* slider = &_sliders[_slider];
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
            Vector p = _clicked.vector();
            SRGB c = _rgb.toSrgb24(_captures[_clicked.index()]);
            for (int xx = 0; xx < 16; ++xx)
                for (int yy = 0; yy < 8; ++yy)
                    _output[p + Vector(xx, yy)] = c;
            _clicked = Block(position);
            if ((GetKeyState(VK_CONTROL) & 0x8000) == 0)
                for (int i = 0; i < 4096; ++i)
                    _optimizing[i] = 0;
            _optimizing[_clicked.index()] = true;
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
            _window->invalidate();
        }
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

    void idle()
    {
        if (_paused)
            return;
        double oldFitness = _fitness;
        Colour oldComputes[4096];
        for (int i = 0; i < 4096; ++i)
            oldComputes[i] = _computes[i];
        // Pick a random slider
        Slider* slider = 0;
        do {
            slider = &_sliders[rand()%_sliderCount];
            if (slider->use())
                break;
        } while (true);

        // Pick a direction to move it in
        bool lower = (rand()%2 == 0);

        // Save old slider value
        double oldValue = slider->value();

        // Move the slider
        double amount = (slider->high() - slider->low())/1000;
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

        computeFitness();

        // If old fitness was better, restore _fitness, slider value and
        // computed colours.
        if (oldFitness < _fitness) {
            _fitness = oldFitness;
            slider->setValue(oldValue);
            for (int i = 0; i < 4096; ++i)
                _computes[i] = oldComputes[i];
        }
    }

private:
    void computeFitness()
    {
        _n14MHz = _p14MHz.invert().delay(_u6f);
        dFlipFlop(Signal::p3_58MHz(), _p14MHz, &_chromas[6], _u43b);
        _chromas[1] = _chromas[6].invert();
        dFlipFlop(_chromas[6], _p14MHz, &_chromas[4], _u44a);
        _chromas[3] = _chromas[4].invert();
        dFlipFlop(_chromas[4], _n14MHz, &_chromas[5], _u44b);
        _chromas[2] = _chromas[5].invert();
        Signal burst = _chromas[6].delay(_u45Data);
        for (int i = 1; i < 7; ++i)
            _chromas[i] = _chromas[i].delay(_u45Data).delay(_u45Output);
        Complex<double> qamBurst;
        //burst.integrate(&_ab, &qamBurst);
        integrate(burst, _chromas[0], &qamBurst);

        _qamAdjust = -phaseQam(static_cast<int>((33 + 90 + _hue)*Signal::cycle()/360))*
            qamBurst.conjugate()*_saturation*_contrast*0.4/qamBurst.modulus();

        _fitness = 0;
        int fitCount = 0;
        for (int i = 0; i < 4096; ++i) {
            Block b(i);
            Signal chroma, intensity;
            generate(b, &chroma, &intensity);
            //if (i == _clicked.index())
            //    printf("Clicked\n");
            Colour c = integrate(chroma, intensity);

            _computes[i] = c;

            // I think these colours are not captured quite so
            // accurately as the others since we only managed an
            // 11-hdot wide swath instead of a full 16 hdots like the
            // others.
            if (_optimizing[i]) {
                _fitness += (c - _rgb.toSrgb(_captures[i])).modulus2();
                ++fitCount;
            }
        }
        _fitness /= fitCount;
    }
    // A Signal represents a waveform with digital voltages but analogue
    // timings. A Signal is periodic with the period of the color carrier
    // signal p = 17600/63 ns, and therefore always has an even number of
    // transitions. The _t are kept sorted and < p. All transitions over the
    // period are enumerated, even if the signal's lowest frequency component
    // is higher than 1/p.
    class Signal
    {
    public:
        // +14MHz, a square wave that is high initially.
        static Signal p14MHz()
        {
            Signal s;
            s._initiallyHigh = false;
            for (int i = 0; i < 8; ++i)
                s._t[i] = i*cycle()/8;
            s._n = 8;
            return s;
        }
        static Signal p3_58MHz()
        {
            Signal s;
            s._initiallyHigh = true;
            s._t[0] = 1 + cycle()/4;
            s._t[1] = 1 + 3*cycle()/2;
            s._n = 2;
            return s;
        }
        static Signal fromBits(bool b0, bool b1, bool b2, bool b3)
        {
            Signal s;
            s._initiallyHigh = b3;
            int n = 0;
            if (b0 != b3) {
                s._t[n] = 0;
                ++n;
            }
            if (b1 != b0) {
                s._t[n] = cycle()/4;
                ++n;
            }
            if (b2 != b1) {
                s._t[n] = cycle()/2;
                ++n;
            }
            if (b3 != b2) {
                s._t[n] = 3*cycle()/4;
                ++n;
            }
            s._n = n;
            return s;
        }
        Signal invert() const
        {
            Signal s = *this;
            s._initiallyHigh = !s._initiallyHigh;
            return s;
        }
        Signal delay(Vector2<double> risefall) const
        {
            int r = static_cast<int>(risefall.x*ns());
            int f = static_cast<int>(risefall.y*ns());
            Signal s = *this;
            for (int i = 0; i < s._n; ++i)
                s._t[i] += (((i & 1) != 0) != _initiallyHigh ? f : r);
            s.normalize();
            return s;
        }
        const Signal& operator=(const Signal& other)
        {
            _n = other._n;
            _initiallyHigh = other._initiallyHigh;
            for (int i = 0; i < _n; ++i)
                _t[i] = other._t[i];
            return *this;
        }

        static int ns() { return 126; }
    //private:
        void normalize()
        {
            if ((_n & 1) != 0)
                throw Exception("Odd number of transitions.");
            bool finished;
            do {
                finished = true;
                for (int i = 0; i < _n - 1; ++i)
                    if (_t[i + 1] <= _t[i]) {
                        for (int j = i; j < _n - 2; ++j)
                            _t[j] = _t[j + 2];
                        _n -= 2;
                        finished = false;
                        break;
                    }
                if (_n == 0)
                    return;
                do {
                    int t = _t[_n - 1] - cycle();
                    if (t >= 0) {
                        for (int i = _n - 1; i >= 1; --i)
                            _t[i] = _t[i - 1];
                        _t[0] = t;
                        _initiallyHigh = !_initiallyHigh;
                        finished = false;
                    }
                    else
                        break;
                } while (true);
            } while (!finished);
        }
        static int cycle() { return 35200; }
        bool high(int t) const
        {
            for (int i = 0; i < _n; i += 2) {
                if (t < _t[i])
                    return _initiallyHigh;
                if (t < _t[i + 1])
                    return !_initiallyHigh;
            }
            return _initiallyHigh;
        }
        int iFromT(int t) const
        {
            for (int i = 0; i < _n; ++i)
                if (t < _t[i])
                    return i;
            return _n;
        }

        bool _initiallyHigh;
        int _t[12];
        int _n;
    };
    void dFlipFlop(const Signal& input, const Signal& clock,
        Signal* output, Vector2<double> riseFall)
    {
        Signal o;
        int n = 0;
        int cn = clock._n;
        int p = (clock._initiallyHigh ? 1 : 0);
        bool os = input.high(clock._t[cn + p - 2]);
        if (cn == 0)
            throw Exception("Unclocked flip flop.");
        o._initiallyHigh = os;
        for (int i = 0; i < cn; i += 2) {
            int t = clock._t[i + p];
            bool ns = input.high(t);
            if (ns != os) {
                o._t[n] = t;
                ++n;
            }
            os = ns;
        }
        o._n = n;
        o.normalize();

        *output = o.delay(riseFall);
    }
    Signal multiplex(Signal a, Signal b, Signal c, const Signal* d, bool drawTransitions)
    {
        a = a.delay(_u45Select);
        b = b.delay(_u45Select);
        c = c.delay(_u45Select);

        int ia = 0, ib = 0, ic = 0, id = 0;
        Signal s;
        int n = 0;
        int x = 0;
        bool needInvert = false;
        if (a._initiallyHigh)
            x ^= 1;
        if (b._initiallyHigh)
            x ^= 2;
        if (c._initiallyHigh)
            x ^= 4;
        bool h = d[x]._initiallyHigh;
        s._initiallyHigh = h;
        int t;
        do {
            int ta = -1;
            int tb = -1;
            int tc = -1;
            int td = -1;
            t = 0x7fffffff;
            if (ia < a._n) {
                ta = a._t[ia];
                if (ta < t)
                    t = ta;
            }
            if (ib < b._n) {
                tb = b._t[ib];
                if (tb < t)
                    t = tb;
            }
            if (ic < c._n) {
                tc = c._t[ic];
                if (tc < t)
                    t = tc;
            }
            if (id < d[x]._n) {
                td = d[x]._t[id];
                if (td < t)
                    t = td;
            }
            if (t >= Signal::cycle())
                break;
            bool oh = h;
            int ox = x;
            if (t == ta) {
                ++ia;
                x ^= 1;
            }
            if (t == tb) {
                ++ib;
                x ^= 2;
            }
            if (t == tc) {
                ++ic;
                x ^= 4;
            }
            if (t == td) {
                ++id;
                h = !h;
                oh = h;
                s._t[n] = t;
                ++n;
            }
            if (x != ox) {
                h = d[x].high(t);
                id = d[x].iFromT(t);
                if (h != oh) {
                    s._t[n] = t;
                    ++n;
                }
            }
            //if ((t == td) && (x != ox))
            //    throw Exception("Select and data changed at same time.");
        } while (true);
        s._n = n;
        s = s.delay(_u45Output);
        if (drawTransitions) {
            int counts[256];
            for (int i = 0; i < 256; ++i)
                counts[i] = 0;
            for (int i = 0; i < n; ++i) {
                int x = (s._t[i]*256/Signal::cycle()) & 0xff;
                ++counts[x];
                SRGB c = SRGB(0, 0, 255);
                if (counts[x] > 1)
                    c = SRGB(0, 255, 0);
                int yLow = 64;
                int yHigh = 256;
                if (s._initiallyHigh == ((i & 1) != 0)) {
                    yLow = 0;
                    yHigh = 192;
                }
                for (int y = yLow; y < yHigh; ++y)
                    _output[_waveformTL + Vector(x, y)] = c;
            }
        }
        s.normalize();
        return s;
    }

    Colour integrate(const Signal& chroma, const Signal& i, Complex<double>* iqOut = 0)
    {
        static const Complex<double> j(0, 1);

        double y;
        Complex<double> iq;
        int x = 0;
        if (i._initiallyHigh)
            x ^= 1;
        if (chroma._initiallyHigh)
            x ^= 2;
        double vInitial = _voltages[x];
        double v;
        int k = 0;
        do {
            int t = 0;
            v = vInitial;
            y = 0;
            iq = 0;
            int ii = 0, ic = 0;
            do {
                int ot = t;
                int ti = -1;
                int tc = -1;
                t = Signal::cycle();
                if (ii < i._n) {
                    ti = i._t[ii];
                    if (ti < t)
                        t = ti;
                }
                if (ic < chroma._n) {
                    tc = chroma._t[ic];
                    if (tc < t)
                        t = tc;
                }
                int ox = x;
                if (t == ti) {
                    ++ii;
                    x ^= 1;
                }
                if (t == tc) {
                    ++ic;
                    x ^= 2;
                }
                double vNew = _voltages[ox];

                // Between ot and t, voltage goes from v to vNew
                // at time tt, voltage vv(tt) = vNew + (v - vNew)*exp(-(tt - ot)/tDecay)
                double tD = _decayTime*63*tau/17600;  // Convert nanoseconds to radians
                double tF = t*tau/Signal::cycle();
                double tI = ot*tau/Signal::cycle();
                y += vNew*(tF - tI) + tD*(v - vNew)*(1 - exp((tI - tF)/tD));

                iq += vNew*j*(exp(j*tI) - exp(j*tF));  // steady state part
                if (tD != 0)
                    iq += (exp(j*tF + (tI - tF)/tD) - exp(j*tI))*(v - vNew)/(j - 1/tD);    // transient part

                if (tD == 0)
                    v = vNew;
                else
                    v = vNew + (v - vNew)*exp(-(tF - tI)/tD);
            } while (t < Signal::cycle());
            ++k;
        } while (abs(v - vInitial) > 1/256.0 && k < 10);

        y = y*_contrast/tau + _brightness;
        if (iqOut != 0)
            *iqOut = iq;
        iq *= _qamAdjust;

        double ro = clamp(0.0, 255*(y + 0.9563*iq.x + 0.6210*iq.y), 255.0);
        double go = clamp(0.0, 255*(y - 0.2721*iq.x - 0.6474*iq.y), 255.0);
        double bo = clamp(0.0, 255*(y - 1.1069*iq.x + 1.7046*iq.y), 255.0);
        return Colour(ro, go, bo);
    }

    void generate(Block block, Signal* chroma, Signal* i, bool drawTransitions = false)
    {
        int bits = block.bits();
        int foreground = block.foreground();
        int background = block.background();
        bool b0 = ((bits & 8) != 0);
        bool b1 = ((bits & 4) != 0);
        bool b2 = ((bits & 2) != 0);
        bool b3 = ((bits & 1) != 0);
        bool fi = ((foreground & 8) != 0);
        bool fr = ((foreground & 4) != 0);
        bool fg = ((foreground & 2) != 0);
        bool fb = ((foreground & 1) != 0);
        bool bi = ((background & 8) != 0);
        bool br = ((background & 4) != 0);
        bool bg = ((background & 2) != 0);
        bool bb = ((background & 1) != 0);
        Signal iIn = Signal::fromBits(b0 ? fi : bi, b1 ? fi : bi, b2 ? fi : bi, b3 ? fi : bi);
        Signal rIn = Signal::fromBits(b0 ? fr : br, b1 ? fr : br, b2 ? fr : br, b3 ? fr : br);
        Signal gIn = Signal::fromBits(b0 ? fg : bg, b1 ? fg : bg, b2 ? fg : bg, b3 ? fg : bg);
        Signal bIn = Signal::fromBits(b0 ? fb : bb, b1 ? fb : bb, b2 ? fb : bb, b3 ? fb : bb);
        Signal r;
        Signal g;
        Signal b;
        dFlipFlop(iIn, _n14MHz, i, _ic74s174);  // U101
        dFlipFlop(rIn, _n14MHz, &r, _ic74s174);  // U101
        dFlipFlop(gIn, _n14MHz, &g, _ic74s174);  // U101
        dFlipFlop(bIn, _n14MHz, &b, _ic74s174);  // U101
        Signal rx = r.delay(_u68b);
        Signal gx = g.delay(_u68a);
        if (drawTransitions)
            for (int x = 0; x < 256; ++x) {
                int t = x*Signal::cycle()/256;
                if (rx.delay(_u45Select).high(t))
                    _output[_waveformTL + Vector(x, 255)] += SRGB(255, 0, 0);
                if (gx.delay(_u45Select).high(t))
                    _output[_waveformTL + Vector(x, 255)] += SRGB(0, 255, 0);
                if (b.delay(_u45Select).high(t))
                    _output[_waveformTL + Vector(x, 255)] += SRGB(0, 0, 255);
                if (_chromas[1].high(t))
                    _output[_waveformTL + Vector(x, 248)] = _srgb.toSrgb24(_computes[Block(1, 1, 0).index()]);
                if (_chromas[3].high(t))
                    _output[_waveformTL + Vector(x, 249)] = _srgb.toSrgb24(_computes[Block(3, 3, 0).index()]);
                if (_chromas[2].high(t))
                    _output[_waveformTL + Vector(x, 250)] = _srgb.toSrgb24(_computes[Block(2, 2, 0).index()]);
                if (_chromas[6].high(t))
                    _output[_waveformTL + Vector(x, 251)] = _srgb.toSrgb24(_computes[Block(6, 6, 0).index()]);
                if (_chromas[4].high(t))
                    _output[_waveformTL + Vector(x, 252)] = _srgb.toSrgb24(_computes[Block(4, 4, 0).index()]);
                if (_chromas[5].high(t))
                    _output[_waveformTL + Vector(x, 253)] = _srgb.toSrgb24(_computes[Block(5, 5, 0).index()]);
            }
        *chroma = multiplex(b, gx, rx, _chromas, drawTransitions);  // U45
    }

    SRGB getDecodedPixel0(int bitmap, Vector p)
    {
        Bitmap<SRGB> b;
        switch (bitmap) {
            case 0: b = _top; break;
            case 1: b = _bottom; break;
        }
        return b[p];
    }
    Colour getDecodedPixel1(int bitmap, Vector p)
    {
        return _rgb.fromSrgb(getDecodedPixel0(bitmap, p));
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
    void drawSample1(Vector p)
    {
        SRGB red(255, 0, 0);
        _output[_waveformTL + p] = red;
        _output[_waveformTL + p + Vector(-1, -1)] = red;
        _output[_waveformTL + p + Vector(-2, -2)] = red;
        _output[_waveformTL + p + Vector(-1, 1)] = red;
        _output[_waveformTL + p + Vector(-2, 2)] = red;
        _output[_waveformTL + p + Vector(1, -1)] = red;
        _output[_waveformTL + p + Vector(2, -2)] = red;
        _output[_waveformTL + p + Vector(1, 1)] = red;
        _output[_waveformTL + p + Vector(2, 2)] = red;
    }
    Complex<double> drawSamples2(Vector p, bool draw = true)
    {
        Byte* b;
        double phase;
        if (p.y < 100) {
            b = &_topRaw[0];
            phase = _topPhase;
        }
        else {
            b = &_bottomRaw[0];
            p.y -= 100;
            phase = _bottomPhase;
        }
        b += 18*1824 + 196;
        int o = p.x*32 + p.y*2*1824;
        b += static_cast<int>(static_cast<double>(o)*(1824 - 19.0/83)/1824);
        double adjust = o*19.0/83/1824;
        int a = adjust;
        adjust -= a;
        Complex<double> iq = 0;
        for (int i = 0; i < 8; ++i) {
            double xx = i + adjust - phase;
            iq += exp(Complex<double>(0, 1)*xx*tau/8)*static_cast<double>(b[i]);
            int x = static_cast<int>(xx*32 + 32) & 0xff;
            if (draw)
                drawSample1(Vector(x, 256 - static_cast<int>(_sampleScale*b[i] + _sampleOffset)));
        }
        return iq;
    }
    void drawSamples3(int patch, int line, int set)
    {
        int y = (set/3)*2;
        bool firstHalf = (patch < 3);
        patch += line*10;
        switch (set % 3) {
            case 0: drawSamples2(Vector(patch, y)); break;
            case 1:
                if (firstHalf)
                    drawSamples2(Vector(patch + 6, y));
                else
                    drawSamples2(Vector(patch - 3, y + 1));
                break;
            case 2: drawSamples2(Vector(patch + 3, y + 1)); break;
        }
    }
    void drawSamples4(int fg, int bg, int bits)
    {
        switch (bits) {
            case 0x00: drawSamples3(2, 0, (bg << 4) | bg); break;
            case 0x01: drawSamples3(0, 1, (fg << 4) | bg); break;
            case 0x02: drawSamples3(1, 0, (bg << 4) | fg); break;
            case 0x03: drawSamples3(2, 0, (fg << 4) | bg); break;
            case 0x04: drawSamples3(5, 3, (fg << 4) | bg); break;
            case 0x05: drawSamples3(3, 0, (bg << 4) | fg); break;
            case 0x06: drawSamples3(4, 0, (bg << 4) | fg); break;
            case 0x07: drawSamples3(1, 1, (fg << 4) | bg); break;
            case 0x08: drawSamples3(1, 1, (bg << 4) | fg); break;
            case 0x09: drawSamples3(4, 0, (fg << 4) | bg); break;
            case 0x0a: drawSamples3(3, 0, (fg << 4) | bg); break;
            case 0x0b: drawSamples3(5, 3, (bg << 4) | fg); break;
            case 0x0c: drawSamples3(2, 0, (bg << 4) | fg); break;
            case 0x0d: drawSamples3(1, 0, (fg << 4) | bg); break;
            case 0x0e: drawSamples3(0, 1, (bg << 4) | fg); break;
            case 0x0f: drawSamples3(2, 0, (fg << 4) | fg); break;
        }
    }
    static Complex<double> phaseQam(int t)
    {
        return exp(Complex<double>(0, t*tau/Signal::cycle()));
    }

    Array<Byte> _topRaw;
    Array<Byte> _bottomRaw;
    Bitmap<SRGB> _top;
    Bitmap<SRGB> _bottom;
    Bitmap<SRGB> _output;
    ColourSpace _rgb;
    ColourSpace _srgb;

    Signal _p14MHz;
    Signal _n14MHz;
    Signal _chromas[8];
    double _ab;
    Complex<double> _qamAdjust;

    ImageWindow2* _window;

    Slider _sliders[25];
    int _slider;
    int _sliderCount;

    Vector _dragStart;
    int _dragStartX;

    double _saturation;
    double _hue;
    double _brightness;
    double _contrast;
    Vector2<double> _u6f;
    Vector2<double> _u45Data;
    Vector2<double> _u45Select;
    Vector2<double> _u45Output;
    Vector2<double> _u68b;
    Vector2<double> _u68a;
    Vector2<double> _ic74s174;
    Vector2<double> _u43b;
    Vector2<double> _u44a;
    Vector2<double> _u44b;
    double _decayTime;  // nanoseconds to decay to 1/e

    double _voltages[4];
    Colour _captures[4096];
    Colour _computes[4096];
    double _fitness;

    Block _clicked;
    Vector _waveformTL;
    bool _optimizing[4096];

    double _sampleScale;
    double _sampleOffset;
    double _topPhase;
    double _bottomPhase;

    bool _paused;
};

template<class Base> class CalibrateWindow : public Base
{
public:
    class Params
    {
        friend class CalibrateWindow;
    public:
        Params(typename Base::Params bp)
          : _bp(bp)
        { }
    private:
        typename Base::Params _bp;
    };

    CalibrateWindow() { }

    void create(Params p)
    {
        Base::create(p._bp);
        _image->setWindow(this);
    }

    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg) {
            case WM_LBUTTONDOWN:
                if (_image->buttonDown(vectorFromLParam(lParam)))
                    SetCapture(_hWnd);
                break;
            case WM_LBUTTONUP:
                ReleaseCapture();
                _image->buttonUp(vectorFromLParam(lParam));
                break;
            case WM_MOUSEMOVE:
                _image->mouseMove(vectorFromLParam(lParam));
                break;
            case WM_CHAR:
                if (wParam == 'x' || wParam == 'X')
                    _image->escapePressed();
                if (wParam == 'p' || wParam == 'P')
                    _image->pause();
                break;
            case WM_KILLFOCUS:
                ReleaseCapture();
                break;
        }
        return Base::handleMessage(uMsg, wParam, lParam);
    }
private:
    static Vector vectorFromLParam(LPARAM lParam)
    {
        return Vector(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    }
};

class CalibrateIdle : public IdleProcessor
{
public:
    CalibrateIdle(CalibrateImage* image) : _image(image) { }
    void idle() { _image->idle(); }
private:
    CalibrateImage* _image;
};

class Program : public ProgramBase
{
public:
    void run()
    {
        CalibrateImage image;

        Window::Params wp(&_windows, L"CGA Calibration", Vector(1536, 1024));
        RootWindow2::Params rwp(wp);
        ImageWindow2::Params iwp(rwp, &image);
        typedef AnimatedWindow<ImageWindow2> AnimatedWindow;
        AnimatedWindow::Params awp(iwp);
        typedef CalibrateWindow<AnimatedWindow> CalibrateWindow;
        CalibrateWindow::Params cwp(awp);
        CalibrateWindow window;
        window.create(cwp);

        CalibrateIdle idle(&image);

        window.show(_nCmdShow);
        pumpMessages(&idle);
    }
};
