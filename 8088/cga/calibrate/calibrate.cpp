#include "alfe/main.h"
#include "alfe/bitmap_png.h"
#include "alfe/terminal6.h"
#include "alfe/complex.h"

class CalibrateImage;

typedef RootWindow<Window> RootWindow2;
typedef ImageWindow<RootWindow2, CalibrateImage> ImageWindow2;

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

class CalibrateImage : public Image
{
public:
    void setWindow(ImageWindow2* window)
    {
        _window = window;

        double brightness = -0.124;
        double contrast = 1.052;

        BitmapFileFormat<SRGB> png = PNGFileFormat();
        _top1 = png.load(File("../../../../top1.png"));
        _top2 = png.load(File("../../../../top2.png"));
        _bottom1 = png.load(File("../../../../bottom1.png"));
        _bottom2 = png.load(File("../../../../bottom2.png"));
        _output = Bitmap<SRGB>(Vector(1536, 1024));
        _rgb = ColourSpace::rgb();

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

        Colour directColours[16];
        Colour directColoursSRGB[16];
        double dcVoltage[16];
        for (int c = 0; c < 16; ++c) {
            directColours[c] = Colour(0, 0, 0);
            for (int b = 0; b < 16; ++b)
                directColours[c] += _captures[Vector(c*16 + c, b)];
            directColours[c] /= 16;
            Colour srgb = _rgb.toSrgb(directColours[c]);
            directColoursSRGB[c] =  srgb;
            dcVoltage[c] = (srgb.x*0.299 + srgb.y*0.587 + srgb.z*0.114)/256.0;
        }
        _voltages[0] = dcVoltage[0];
        _voltages[1] = dcVoltage[8];
        _voltages[2] = dcVoltage[7];
        _voltages[3] = dcVoltage[15];

        for (int i = 1; i <= 6; ++i)
            _chromas[i] = deriveChroma(directColoursSRGB[i]);

        _sliderCount = 19;
        _sliders[0] = Slider(0, 1, 0.35, "saturation", &_saturation);
        _sliders[1] = Slider(-180, 180, 0, "hue", &_hue);
        _sliders[2] = Slider(-1, 1, 0 /*-0.124*/, "brightness", &_brightness);
        _sliders[3] = Slider(0, 2, 1 /*1.052*/, "contrast", &_contrast);
        _sliders[4] = Slider(0, 70, 8, "U6F rise", &_u6fRise);
        _sliders[5] = Slider(0, 70, 8, "U6F fall", &_u6fFall);
        _sliders[6] = Slider(0, 70, 0 /*17*/, "U45 data rise", &_u45DataRise);
        _sliders[7] = Slider(0, 70, 0 /*15*/, "U45 data fall", &_u45DataFall);
        _sliders[8] = Slider(0, 70, 22, "U45 select rise", &_u45SelectRise);
        _sliders[9] = Slider(0, 70, 16.5, "U45 select fall", &_u45SelectFall);
        _sliders[10] = Slider(0, 70, 10, "U68B rise", &_u68bRise);
        _sliders[11] = Slider(0, 70, 9.5, "U68B fall", &_u68bFall);
        _sliders[12] = Slider(0, 70, 10, "U68A rise", &_u68aRise);
        _sliders[13] = Slider(0, 70, 9.5, "U68A fall", &_u68aFall);
        _sliders[14] = Slider(0, 1, 0.72, "Chroma V", &_chromaV);
        _sliders[15] = Slider(0, 70, 6.5, "74S174 rise", &_ic74s174Rise);
        _sliders[16] = Slider(0, 70, 8.5, "74S174 fall", &_ic74s174Fall);
        _sliders[17] = Slider(0, 70, 4.5, "74S74 rise", &_ic74s74Rise);
        _sliders[18] = Slider(0, 70, 4.5, "74S74 fall", &_ic74s74Fall);

        for (int i = 0; i < _sliderCount; ++i) {
            _sliders[i].setBitmap(_output.subBitmap(Vector(1024, i*8), Vector(512, 8)));
            _sliders[i].draw();
        }
        _slider = -1;
    }

    virtual void draw()
    {
        Signal p14MHz = Signal::p14MHz();
        _n14MHz = p14MHz.invert().delay(_u6fRise*Signal::ns(), _u6fFall*Signal::ns()); // U6F
        dFlipFlop(Signal::p3_58MHz(), p14MHz, &_chromas[6]);  // U43B
        _chromas[1] = _chromas[6].invert();
        dFlipFlop(_chromas[6], p14MHz, &_chromas[4]);         // U44A
        _chromas[3] = _chromas[4].invert();
        dFlipFlop(_chromas[4], _n14MHz, &_chromas[5]);        // U44B
        _chromas[2] = _chromas[5].invert();
        Signal burst = _chromas[6].delay(_u45DataRise*Signal::ns(), _u45DataFall*Signal::ns()); // U45
        Complex<double> qamBurst;
        burst.integrate(&_ab, &qamBurst);
        _chromas[0] = Signal::fromBits(false, false, false, false);
        _chromas[7] = Signal::fromBits(true, true, true, true);
        //double s = -sin(33*M_PI/180);
        //double c = cos(33*M_PI/180);
        //_cb = c*cb - s*sb;
        //_sb = -(s*cb + c*sb);
        //_saturation = 0.4/sqrt(_sb*_sb + _cb*_cb);

        _qamAdjust = -phaseQam((33 + _hue)*Signal::cycle()/360)*
            qamBurst.conjugate()*_saturation*_contrast*0.4/qamBurst.modulus();

        for (int fg = 0; fg < 16; ++fg)
            for (int bg = 0; bg < 16; ++bg)
                for (int bits = 0; bits < 16; ++bits) {
                    int x = (fg << 3) | ((bits & 3) << 1);
                    int y = (bg << 3) | ((bits & 0xc) >> 1);
                    SRGB g = generate(fg, bg, bits);
                    for (int xx = 0; xx < 16; ++xx)
                        for (int yy = 0; yy < 8; ++yy) {
                            Vector p = (Vector(x, y)<<3) + Vector(xx, yy);
                            _output[p + Vector(0, 8)] = g;
                        }
                }

        Vector zero(0, 0);
        Vector sz = size();
        if (sz.x > 1536)
            sz.x = 1536;
        if (sz.y > 1024)
            sz.y = 1024;
        //subBitmap(zero, s).copyFrom(_output.subBitmap(zero, s));

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
            _window->invalidate();
        }
    }

private:
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
        Signal delay(int rise, int fall) const
        {
            Signal s = *this;
            for (int i = 0; i < s._n; ++i)
                s._t[i] += (((i & 1) != 0) != _initiallyHigh ? fall : rise);
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
        void integrate(double* ap, Complex<double>* qamp) const
        {
            double a = 0;
            //, s = 0, c = 0;
            Complex<double> qam(0);

            for (int i = 0; i < _n; i += 2) {
                a += static_cast<double>(_t[i + 1] - _t[i])/cycle();

                Complex<double> qamOld = phaseQam(_t[i]);
                Complex<double> qamNew = phaseQam(_t[i + 1]);
                qam += (qamNew - qamOld)*Complex<double>(0, -1);
            }
            if (_initiallyHigh) {
                a = 1 - a;
                qam = -qam;
            }
            *ap = a;
            *qamp = qam;
        }

        static int ns() { return 126; }
    //private:
        void normalize()
        {
            if ((_n & 1) != 0)
                throw Exception("Odd number of transitions.");
            for (int i = 0; i < _n - 1; ++i)
                if (_t[i + 1] <= _t[i]) {
                    for (int j = i; j < _n - 2; ++j)
                        _t[j] = _t[j + 2];
                    _n -= 2;
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
                }
                else
                    break;
            } while (true);
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
        Signal* output, bool ic74s174 = false)
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

        if (ic74s174)
            *output = o.delay(_ic74s174Rise*Signal::ns(), _ic74s174Fall*Signal::ns());
        else
            *output = o.delay(_ic74s74Rise*Signal::ns(), _ic74s74Fall*Signal::ns());  // 74S74
    }
    Signal multiplex(const Signal& a, const Signal& b,
        const Signal& c, const Signal* d)
    {
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
                s._t[n] = t + (h ? _u45DataRise*Signal::ns() : _u45DataFall*Signal::ns());
                ++n;
            }
            if (x != ox) {
                h = d[x].high(t);
                id = d[x].iFromT(t);
                if (h != oh) {
                    s._t[n] = t + (h ? _u45SelectRise*Signal::ns() : _u45SelectFall*Signal::ns());
                    ++n;
                }
            }
            //if ((t == td) && (x != ox))
            //    throw Exception("Select and data changed at same time.");
        } while (true);
        s._n = n;
        s.normalize();
        return s;
    }
    Colour integrate(const Signal& chroma, const Signal& i)
    {
        int ii = 0, ic = 0;
        double y = 0;
        Complex<double> iq = 0;

        int x = 0;
        if (i._initiallyHigh)
            x ^= 1;
        if (chroma._initiallyHigh)
            x ^= 2;
        int t = 0;
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
            double v = _voltages[ox];            
            y += v*static_cast<double>(t - ot)/Signal::cycle();
            iq += v*(phaseQam(t) - phaseQam(ot));
        } while (t < Signal::cycle());

        y = y*_contrast + _brightness;
        iq *= _qamAdjust;

        double ro = clamp(0.0, 255*(y + 0.9563*iq.x + 0.6210*iq.y), 255.0);
        double go = clamp(0.0, 255*(y - 0.2721*iq.x - 0.6474*iq.y), 255.0);
        double bo = clamp(0.0, 255*(y - 1.1069*iq.x + 1.7046*iq.y), 255.0);
        return Colour(ro, go, bo);
    }   

    SRGB generate(int foreground, int background, int bits)
    {
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
        Signal i;
        Signal r;
        Signal g;
        Signal b;
        dFlipFlop(iIn, _n14MHz, &i, true);  // U101
        dFlipFlop(rIn, _n14MHz, &r, true);  // U101
        dFlipFlop(gIn, _n14MHz, &g, true);  // U101
        dFlipFlop(bIn, _n14MHz, &b, true);  // U101
        Signal rx = r.delay(_u68bRise*Signal::ns(), _u68bFall*Signal::ns());  // U68B
        Signal gx = g.delay(_u68aRise*Signal::ns(), _u68aFall*Signal::ns());  // U68A
        Signal chroma = multiplex(b, gx, rx, _chromas);  // U45

        return Vector3Cast<UInt8>(integrate(chroma, i));
    }
    Signal deriveChroma(Colour srgb)
    {
        srgb /= 256.0;
        double y = srgb.x*0.299 + srgb.y*0.587 + srgb.z*0.114;
        double i = srgb.x*0.595716 - srgb.y*0.274453 - srgb.z*0.321263;
        double q = srgb.x*0.211456 - srgb.y*0.522591 + srgb.z*0.311135;
        int a = (atan2(q, i) + M_PI)*Signal::cycle()/(2*M_PI);
        Signal s;
        s._n = 2;
        int h = (y - _voltages[0])*Signal::cycle()/(_voltages[2] - _voltages[0]);
        s._t[0] = a + Signal::cycle() - h/2;
        s._t[1] = a + Signal::cycle() + h/2;
        s._initiallyHigh = false;
        s.normalize();
        return s;
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
    Colour getPixel1(int bitmap, Vector p)
    {
        Colour p0 = _rgb.fromSrgb(getPixel0(bitmap << 1, p));
        Colour p1 = _rgb.fromSrgb(getPixel0((bitmap << 1) | 1, p));
        return (p0 + p1)/2;
    }
    Colour getPixel2(Vector p)
    {
        int bitmap = 0;
        if (p.y >= 100) {
            bitmap = 1;
            p.y -= 100;
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
        return c/6;
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
    static Complex<double> phaseQam(int t)
    {
        return exp(Complex<double>(0, t*2*M_PI/Signal::cycle()));
    }

    Bitmap<SRGB> _top1;
    Bitmap<SRGB> _top2;
    Bitmap<SRGB> _bottom1;
    Bitmap<SRGB> _bottom2;
    Bitmap<SRGB> _output;
    ColourSpace _rgb;

    Signal _n14MHz;
    Signal _chromas[8];
    double _ab;
    Complex<double> _qamAdjust;

    ImageWindow2* _window;

    Slider _sliders[19];
    int _slider;
    int _sliderCount;

    Vector _dragStart;
    int _dragStartX;

    double _saturation;
    double _hue;
    double _brightness;
    double _contrast;
    double _u6fRise;
    double _u6fFall;
    double _u45DataRise;
    double _u45DataFall;
    double _u45SelectRise;
    double _u45SelectFall;
    double _u68bRise;
    double _u68bFall;
    double _u68aRise;
    double _u68aFall;
    double _chromaV;
    double _ic74s174Rise;
    double _ic74s174Fall;
    double _ic74s74Rise;
    double _ic74s74Fall;

    double _voltages[4];
    Bitmap<Colour> _captures;
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

class Program : public ProgramBase
{
public:
    void run()
    {
        CalibrateImage image;

        Window::Params wp(&_windows, L"CGA Calibration", Vector(1536, 1024));
        RootWindow2::Params rwp(wp);
        ImageWindow2::Params iwp(rwp, &image);
        typedef CalibrateWindow<ImageWindow2> CalibrateWindow;
        CalibrateWindow::Params cwp(iwp);
        CalibrateWindow window;
        window.create(cwp);

        window.show(_nCmdShow);
        pumpMessages();
    }
};