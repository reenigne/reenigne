#include "alfe/main.h"
#include "alfe/bitmap_png.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        double saturation = 0.35;
        double brightness = -0.124;
        double contrast = 1.052;
        double hue = 50;

        BitmapFileFormat<SRGB> png = PNGFileFormat();
        _top1 = png.load(File("../../../../top1.png"));
        _top2 = png.load(File("../../../../top2.png"));
        _bottom1 = png.load(File("../../../../bottom1.png"));
        _bottom2 = png.load(File("../../../../bottom2.png"));
        _output = Bitmap<SRGB>(Vector(128, 128));
        _rgb = ColourSpace::rgb();

        Signal p14MHz = Signal::p14MHz();                              
        _n14MHz = p14MHz.invert().delay(8*Signal::ns(), 8*Signal::ns()); // U6F
        Signal::dFlipFlop(Signal::p3_58MHz(), p14MHz, &_chromas[6]);  // U43B
        _chromas[1] = _chromas[6].invert();
        Signal::dFlipFlop(_chromas[6], p14MHz, &_chromas[4]);         // U44A
        _chromas[3] = _chromas[4].invert();
        Signal::dFlipFlop(_chromas[4], _n14MHz, &_chromas[5]);        // U44B
        _chromas[2] = _chromas[5].invert();
        Signal burst = _chromas[6].delay(17, 15);                     // U45
        double sb, cb;
        burst.integrate(&_ab, &sb, &cb);
        _chromas[0] = Signal::fromBits(false, false, false, false);
        _chromas[7] = Signal::fromBits(true, true, true, true);
        double s = sin((33 + hue)*M_PI/180);
        double c = cos((33 + hue)*M_PI/180);
        _cb = c*cb - s*sb;
        _sb = -(s*cb + c*sb);
        _saturation = saturation*0.4/sqrt(_sb*_sb + _cb*_cb)*contrast;
        _brightness = brightness;
        _contrast = contrast;

        for (int fg = 0; fg < 16; ++fg)
            for (int bg = 0; bg < 16; ++bg)
                for (int bits = 0; bits < 16; ++bits) {
                    int x = (fg << 3) | ((bits & 3) << 1);
                    int y = (bg << 3) | ((bits & 0xc) >> 1);
                    SRGB c = _rgb.toSrgb24(getPixel4(fg, bg, bits));
                    _output[Vector(x, y)] = c;
                    _output[Vector(x+1, y)] = c;
                    //SRGB g = generate(fg, bg, bits);
                    SRGB g = generate(2, 0, 1);
                    _output[Vector(x, y+1)] = g;
                    _output[Vector(x+1, y+1)] = g;

                }
        _output.save(png, File("output.png"));
    }                                          
private:
    // A Signal represents a waveform with digital voltages but analogue
    // timings. A Signal is periodic with the period of the color carrier
    // signal p = 17600/63 ns, and therefore always has an even number of
    // transitions. If n is even then _t[n] is a rising edge, otherwise it is a
    // falling edge. The _t are kept sorted and <= p; if the signal is high at
    // the start of the cycle and there is no transition there, extra
    // transitions at 0 and p are included. All transitions over the period are
    // enumerated, even if the signal's lowest frequency component is higher
    // than 1/p.
    class Signal
    {
    public:
        // +14MHz, a square wave that is high initially.
        static Signal p14MHz()
        {
            Signal s;
            for (int i = 0; i < 8; ++i)
                s._t[i] = i*cycle()/8;
            s._n = 8;
            return s;
        }
        static Signal p3_58MHz()
        {
            Signal s;
            s._t[0] = 0;
            s._t[1] = 1;
            s._t[2] = 1 + cycle()/2;
            s._t[3] = cycle();
            s._n = 4;
            return s;
        }
        static void dFlipFlop(const Signal& input, const Signal& clock,
            Signal* output, bool ic74s174 = false)
        {
            Signal o;
            int n = 0;
            bool os = false;
            int cn = clock._n;
            if (cn > 0)
                os = input.high(clock._t[cn - 2]);
            if (os) {
                o._t[n] = 0;
                ++n;
            }
            for (int i = 0; i < cn; i += 2) {
                int t = clock._t[i];
                bool ns = input.high(t);
                if (ns != os) {
                    o._t[n] = t;
                    ++n;
                }
                os = ns;
            }
            if ((n & 1) != 0) {
                o._t[n] = cycle();
                ++n;
            }
            o._n = n;
            if (ic74s174)
                *output = o.delay(13*ns()/2, 17*ns()/2);
            else
                *output = o.delay(9*ns()/2, 9*ns()/2);  // 74S74
        }
        static Signal fromBits(bool b0, bool b1, bool b2, bool b3)
        {
            Signal s;
            int n = 0;
            if (b0) {
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
            if (b3) {
                s._t[n] = cycle();
                ++n;
            }
            s._n = n;
            return s;
        }
        static Signal multiplex(const Signal& a, const Signal& b,
            const Signal& c, const Signal* d)
        {
            int ia = 0, ib = 0, ic = 0, id = 0;
            Signal s;
            int n = 0;
            int x = 0;
            bool needInvert = false;
            if (a.high(0)) {
                x ^= 1;
                ++ia;
            }
            if (b.high(0)) {
                x ^= 2;
                ++ib;
            }
            if (c.high(0)) {
                x ^= 4;
                ++ic;
            }
            bool h = false;
            if (d[x].high(0)) {
                s._t[n] = 0;
                ++n;
                h = true;
                ++id;
            }
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
                if (t >= cycle())
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
                    int tp = t + (h ? 17*ns() : 15*ns());
                    if (tp >= cycle())
                        s._t[0] = tp - cycle();
                    else {
                        s._t[n] = tp;
                        ++n;
                    }
                }
                if (x != ox) {
                    h = d[x].high(t);
                    id = d[x].iFromT(t);
                    if (h != oh) {
                        int tp = t + (h ? 22*ns() : 33*ns()/2);
                        if (tp >= cycle()) {
                            tp -= cycle();
                            int i;
                            for (i = n - 1; s._t[i] > tp; --i)
                                s._t[i + 1] = s._t[i];
                            ++n;
                            s._t[i + 1] = tp;
                            if (!h)
                                needInvert = true;
                        }
                        else {
                            s._t[n] = tp;
                            ++n;
                        }
                    }
                }
                if ((t == td) && (x != ox))
                    throw Exception("Select and data changed at same time.");
            } while (true);
            if (h) {
                s._t[n] = cycle();
                ++n;
            }
            s._n = n;
            for (int i = 0; i < n - 1; ++i)
                if (s._t[i + 1] <= s._t[i])
                    throw Exception("multiplex changed order of events.");
            if (needInvert)
                return s.invert();
            return s;
        }
        Signal invert() const
        {
            Signal s;
            if (_n == 0) {
                s._n = 2;
                s._t[0] = 0;
                s._t[1] = cycle();
                return s;
            }
            if (_t[0] == 0) {
                if (_t[_n - 1] == cycle()) {
                    s._n = _n - 2;
                    for (int i = 0; i < _n - 2; ++i)
                        s._t[i] = _t[i + 1];
                    return s;
                }
                s._n = _n;
                for (int i = 0; i < _n - 1; ++i)
                    s._t[i] = _t[i + 1];
                s._t[_n - 1] = cycle();
                return s;
            }
            if (_t[_n - 1] == cycle()) {
                s._n = _n;
                s._t[0] = 0;
                for (int i = 0; i < _n - 1; ++i)
                    s._t[i + 1] = _t[i];
                return s;
            }
            if (_n == 10)
                throw Exception("Too many transitions in waveform.");
            s._n = _n + 2;
            s._t[0] = 0;
            for (int i = 0; i < _n; ++i)
                s._t[i + 1] = _t[i];
            s._t[_n + 1] = cycle();
            return s;
        }
        Signal delay(int rise, int fall) const
        {
            Signal s = *this;
            bool needToInvert = false;
            if (_t[_n - 1] == cycle()) {
                s = s.invert();
                needToInvert = true;
            }

            for (int i = 0; i < s._n; ++i) {
                s._t[i] += ((i & 1) != 0 ? fall : rise);
                if (i > 0) {
                    if (s._t[i] <= s._t[i - 1])
                        throw Exception("delay changed order of events.");
                }
            }
            do {
                int t = s._t[s._n - 1];
                if (t > cycle()) {
                    for (int i = s._n - 1; i >= 1; --i)
                        s._t[i] = s._t[i - 1];
                    s._t[0] = t - cycle();
                    needToInvert = !needToInvert;
                }
                else
                    break;
            } while (true);
            if (needToInvert)
                return s.invert();
            return s;
        }
        const Signal& operator=(const Signal& other)
        {
            _n = other._n;
            for (int i = 0; i < _n; ++i)
                _t[i] = other._t[i];
            return *this;
        }
        void integrate(double* ap, double* sp, double* cp) const
        {
            double a = 0, s = 0, c = 0;
            for (int i = 0; i < _n; i += 2) {
                a += static_cast<double>(_t[i + 1] - _t[i])/cycle();
                s += cos(angle(i)) - cos(angle(i + 1));
                c += sin(angle(i + 1)) - sin(angle(i));
            }
            *ap = a;
            *sp = s;
            *cp = c;
        }

        static int ns() { return 126; }
    private:
        static int cycle() { return 35200; }
        double angle(int i) const { return _t[i]*2*M_PI/cycle(); }
        bool high(int t) const
        {
            for (int i = 0; i < _n; i += 2) {
                if (t < _t[i])
                    return false;
                if (t < _t[i + 1])
                    return true;
            }
            return false;
        }
        int iFromT(int t) const
        {
            for (int i = 0; i < _n; ++i)
                if (t < _t[i])
                    return i;
            return _n;
        }

        int _t[10];
        int _n;
    };

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
        Signal::dFlipFlop(iIn, _n14MHz, &i, true);  // U101
        Signal::dFlipFlop(rIn, _n14MHz, &r, true);  // U101
        Signal::dFlipFlop(gIn, _n14MHz, &g, true);  // U101
        Signal::dFlipFlop(bIn, _n14MHz, &b, true);  // U101
        Signal rx = r.delay(10*Signal::ns(), 19*Signal::ns()/2);  // U68B
        Signal gx = g.delay(10*Signal::ns(), 19*Signal::ns()/2);  // U68A
        Signal chroma = Signal::multiplex(b, gx, rx, _chromas);  // U45
        double ac, sc, cc;
        chroma.integrate(&ac, &sc, &cc);
        double ai, si, ci;
        i.integrate(&ai, &si, &ci);
        double c = 0.72*cc + 0.28*ci;
        double s = 0.72*sc + 0.28*si;

        double yy = (0.72*ac + 0.28*ai)*_contrast + _brightness;
        double ii = (c*_cb - s*_sb)*_saturation;
        double qq = (s*_cb + c*_sb)*_saturation;

        int rf = clamp(0, static_cast<int>(255*(yy + 0.9563*ii + 0.6210*qq)), 255);
        int gf = clamp(0, static_cast<int>(255*(yy - 0.2721*ii - 0.6474*qq)), 255);
        int bf = clamp(0, static_cast<int>(255*(yy - 1.1069*ii + 1.7046*qq)), 255);
        return SRGB(rf, gf, bf);
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

    Bitmap<SRGB> _top1;
    Bitmap<SRGB> _top2;
    Bitmap<SRGB> _bottom1;
    Bitmap<SRGB> _bottom2;
    Bitmap<SRGB> _output;
    ColourSpace _rgb;

    Signal _n14MHz;
    Signal _chromas[8];
    double _ab, _sb, _cb;
    double _saturation;
    double _brightness;
    double _contrast;
};