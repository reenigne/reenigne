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

class Transition
{
public:
    Transition() : _index(0) { }
    Transition(int i) : _index(i) { }
    Transition(int left, int right, int position)
      : _index((left << 6) | (right << 2) | position) { }
    int left() const { return _index >> 6; }
    int right() const { return (_index >> 2) & 0x0f; }
    int position() const { return _index & 3; }
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
        AutoHandle topHandle = File("q:\\top_decoded.raw", true).openRead();
        AutoHandle bottomHandle = File("q:\\bottom_decoded.raw", true).openRead();
        topHandle.read(_top.data(), 760*240*3);
        bottomHandle.read(_bottom.data(), 760*240*3);
        _topRaw.allocate(450*1024);
        _bottomRaw.allocate(450*1024);
        AutoHandle topRawHandle = File("q:\\top.raw", true).openRead();
        AutoHandle bottomRawHandle = File("q:\\bottom.raw", true).openRead();
        topRawHandle.read(&_topRaw[0], 450*1024);
        bottomRawHandle.read(&_bottomRaw[0], 450*1024);

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
        _topPhase = getSamples2(Vector(-1, 0)).argument()*8/tau;
        _bottomPhase = 0;
        _bottomPhase = getSamples2(Vector(-1, 100)).argument()*8/tau;

        _sliderCount = 10;
        _sliders[0] = Slider(0, 2, 0.5655, "saturation", &_saturation);
        _sliders[1] = Slider(-180, 180, 0, "hue", &_hue, false);
        _sliders[2] = Slider(-1, 1, 0, "brightness", &_brightness, false);
        _sliders[3] = Slider(0, 2, 1, "contrast", &_contrast, false);
        _sliders[4] = Slider(-5, 5, 1, "7.16MHz gain", &_716MHzGain);
        _sliders[5] = Slider(0, 1, 0.5, "3.58MHz gain", &_358MHzGain);
        _sliders[6] = Slider(0, 1, 0.5, "7.16MHz phase", &_716MHzPhase);
        _sliders[7] = Slider(0, 1, 0.5, "3.58MHz phase", &_358MHzPhase);
        _sliders[8] = Slider(0, 2, 1, "DC gain", &_dcGain);
        _sliders[9] = Slider(-1, 1, 0, "DC offset", &_dcOffset);

        for (int i = 0; i < _sliderCount; ++i) {
            _sliders[i].setBitmap(_output.subBitmap(Vector(1032, i*8), Vector(512, 8)));
            _sliders[i].draw();
        }
        _slider = -1;

        computeFitness();

        _clicked = Block(0);
        escapePressed();

        SRGB white(255, 255, 255);
        _waveformTL = Vector(1032, (_sliderCount + 4)*8);
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
        write(_output.subBitmap(Vector(1024, (_sliderCount + 2)*8), Vector(512, 8)), "Variance", _variance);

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
    class Bucket
    {
    public:
        Bucket() : _n(0) { }
        void reset() { _n = 0; }
        void add(double v) { _data[_n] = v; ++_n; }
        double mean()
        {
            double r = 0;
            for (int i = 0; i < _n; ++i)
                r += _data[i];
            return r / _n;
        }
        double variance()
        {
            double m = mean();
            double r = 0;
            for (int i = 0; i < _n; ++i)
                r += (_data[i] - m)*(_data[i] - m);
            return r; // / _n;
        }
    private:
        double _data[72];
        int _n;
    };

    void integrate(Block b, double* dc, Complex<double>* iq, double* hf)
    {
        int bits = b.bits();
        int fg = b.foreground();
        int bg = b.background();
        double s[4];
        for (int t = 0; t < 4; ++t) {
            int leftBit = t;
            int rightBit = (t + 1)&3;
            bool left = ((bits << leftBit) & 8) != 0;
            bool right = ((bits << rightBit) & 8) != 0;
            int leftColour = (left ? fg : bg);
            int rightColour = (right ? fg : bg);
            Transition transition(leftColour, rightColour, t);
            s[t] = _buckets[transition.index()].mean();
        }
        *dc = (s[0] + s[1] + s[2] + s[3])/4;
        iq->x = (s[0] - s[2])/2;
        iq->y = (s[1] - s[3])/2;
        *hf = ((s[0] + s[2]) - (s[1] + s[3]))/4;
    }

    void computeFitness()
    {
        _variance = 0;
        for (int i = 0; i < 1024; ++i)
            _buckets[i].reset();
        for (int bg = 0; bg < 8; ++bg)
            for (int fg = 0; fg < 8; ++fg)
                for (int bits = 0; bits < 16; ++bits) {
                    if (bits == 4 || bits == 0x0b)
                        continue;
                    Byte samples[8];
                    double offset;
                    getSamples4(fg, bg, bits, samples, &offset);
                    double dc = 0;
                    Complex<double> iq = 0;
                    double hf = 0;
                    for (int s = 0; s < 8; ++s) {
                        double x = (s + offset)/8;
                        double v = (samples[s]*_sampleScale + _sampleOffset)/256.0;
                        dc += v;
                        iq += unit(_358MHzPhase + x)*v;
                        hf += unit(_716MHzPhase + x*2).x*v;
                    }
                    dc = dc*_dcGain/8 + _dcOffset;
                    iq *= _358MHzGain/8;
                    hf *= _716MHzGain/8;
                    double resampled[4];
                    resampled[0] = dc + iq.x + hf;
                    resampled[1] = dc + iq.y - hf;
                    resampled[2] = dc - iq.x + hf;
                    resampled[3] = dc - iq.y - hf;
                    for (int t = 0; t < 4; ++t) {
                        int leftBit = t;
                        int rightBit = (t + 1)&3;
                        bool left = ((bits << leftBit) & 8) != 0;
                        bool right = ((bits << rightBit) & 8) != 0;
                        int leftColour = (left ? fg : bg);
                        int rightColour = (right ? fg : bg);
                        Transition transition(leftColour, rightColour, t);
                        _buckets[transition.index()].add(resampled[t]);
                    }
                }
        for (int left = 0; left < 8; ++left)
            for (int right = 0; right < 8; ++right)
                for (int position = 0; position < 4; ++position) {
                    Transition t(left, right, position);
                    _variance += _buckets[t.index()].variance();
                }
        double dc;
        Complex<double> iqBurst;
        double hf;
        integrate(Block(6, 6, 0), &dc, &iqBurst, &hf);
        Complex<double> iqAdjust = iqBurst.conjugate()*unit(-(33 + _hue)/360.0)*_saturation*_contrast/iqBurst.modulus();
        
        _fitness = 0;
        int fitCount = 0;
        for (int bg = 0; bg < 8; ++bg)
            for (int fg = 0; fg < 8; ++fg)
                for (int bits = 0; bits < 16; ++bits) {
                    Complex<double> iq;
                    Block block(fg, bg, bits);
                    integrate(block, &dc, &iq, &hf);
                    double y = dc*_contrast + _brightness;
                    iq *= iqAdjust;

                    double r = clamp(0.0, 255*(y + 0.9563*iq.x + 0.6210*iq.y), 255.0);
                    double g = clamp(0.0, 255*(y - 0.2721*iq.x - 0.6474*iq.y), 255.0);
                    double b = clamp(0.0, 255*(y - 1.1069*iq.x + 1.7046*iq.y), 255.0);
                    Colour c(r, g, b);
                    _computes[Block(fg, bg, bits).index()] = c;

                    int i = block.index();
                    if (_optimizing[i]) {
                        _fitness += (c - _rgb.toSrgb(_captures[i])).modulus2();
                        ++fitCount;
                    }
                }
        _fitness /= fitCount;
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
    //void getSample1(Vector p)
    //{
    //    SRGB red(255, 0, 0);
    //    _output[_waveformTL + p] = red;
    //    _output[_waveformTL + p + Vector(-1, -1)] = red;
    //    _output[_waveformTL + p + Vector(-2, -2)] = red;
    //    _output[_waveformTL + p + Vector(-1, 1)] = red;
    //    _output[_waveformTL + p + Vector(-2, 2)] = red;
    //    _output[_waveformTL + p + Vector(1, -1)] = red;
    //    _output[_waveformTL + p + Vector(2, -2)] = red;
    //    _output[_waveformTL + p + Vector(1, 1)] = red;
    //    _output[_waveformTL + p + Vector(2, 2)] = red;
    //}
    Complex<double> getSamples2(Vector p)
    {
        double offset;
        Byte samples[8];
        getSamples2(p, samples, &offset);
        Complex<double> iq = 0;
        for (int i = 0; i < 8; ++i)
            iq += unit((i + offset)/8)*samples[i];
        return iq;
    }
    void getSamples2(Vector p, Byte* destination, double* offset)
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
        int a = static_cast<int>(adjust);
        adjust -= a;
        //Complex<double> iq = 0;
        *offset = adjust - phase;
        for (int i = 0; i < 8; ++i) {
            destination[i] = b[i];
            //double xx = i + adjust - phase;
            //iq += exp(Complex<double>(0, 1)*xx*tau/8)*static_cast<double>(b[i]);
            //int x = static_cast<int>(xx*32 + 32) & 0xff;
            //if (draw)
            //    getSample1(Vector(x, 256 - static_cast<int>(_sampleScale*b[i] + _sampleOffset)));
        }
        //return iq;
    }
    void getSamples3(int patch, int line, int set, Byte* destination, double* offset)
    {
        int y = (set/3)*2;
        bool firstHalf = (patch < 3);
        patch += line*10;
        switch (set % 3) {
            case 0: getSamples2(Vector(patch, y), destination, offset); break;
            case 1:
                if (firstHalf)
                    getSamples2(Vector(patch + 6, y), destination, offset);
                else
                    getSamples2(Vector(patch - 3, y + 1), destination, offset);
                break;
            case 2: getSamples2(Vector(patch + 3, y + 1), destination, offset); break;
        }
    }
    void getSamples4(int fg, int bg, int bits, Byte* destination, double* offset)
    {
        switch (bits) {
            case 0x00: getSamples3(2, 0, (bg << 4) | bg, destination, offset); break;
            case 0x01: getSamples3(0, 1, (fg << 4) | bg, destination, offset); break;
            case 0x02: getSamples3(1, 0, (bg << 4) | fg, destination, offset); break;
            case 0x03: getSamples3(2, 0, (fg << 4) | bg, destination, offset); break;
            case 0x04: getSamples3(5, 3, (fg << 4) | bg, destination, offset); break;
            case 0x05: getSamples3(3, 0, (bg << 4) | fg, destination, offset); break;
            case 0x06: getSamples3(4, 0, (bg << 4) | fg, destination, offset); break;
            case 0x07: getSamples3(1, 1, (fg << 4) | bg, destination, offset); break;
            case 0x08: getSamples3(1, 1, (bg << 4) | fg, destination, offset); break;
            case 0x09: getSamples3(4, 0, (fg << 4) | bg, destination, offset); break;
            case 0x0a: getSamples3(3, 0, (fg << 4) | bg, destination, offset); break;
            case 0x0b: getSamples3(5, 3, (bg << 4) | fg, destination, offset); break;
            case 0x0c: getSamples3(2, 0, (bg << 4) | fg, destination, offset); break;
            case 0x0d: getSamples3(1, 0, (fg << 4) | bg, destination, offset); break;
            case 0x0e: getSamples3(0, 1, (bg << 4) | fg, destination, offset); break;
            case 0x0f: getSamples3(2, 0, (fg << 4) | fg, destination, offset); break;
        }
    }

    Array<Byte> _topRaw;
    Array<Byte> _bottomRaw;
    Bitmap<SRGB> _top;
    Bitmap<SRGB> _bottom;
    Bitmap<SRGB> _output;
    ColourSpace _rgb;
    ColourSpace _srgb;

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
    double _transitionOffset;
    double _dcGain;
    double _dcOffset;
    double _358MHzGain;
    double _716MHzGain;
    double _358MHzPhase;
    double _716MHzPhase;
    
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

    Bucket _buckets[16*16*4];

    double _variance;
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
