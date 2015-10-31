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

class Matrix
{
public:
    Matrix()
    {
        _rows = 1024 + 24;
        _columns = 1024 + 3;
        _data.allocate(_rows*_columns);
        for (int row = 0; row < _rows; ++row)
            for (int column = 0; column < _columns; ++column)
                _data[row*_columns + column] = 0;
        _nextRow = 1024;
    }
    void add(int row, int column, double value) { element(row, column) += value; }

    void solve()
    {
        {
            AutoStream h = File("matrix.raw").openWrite();
            for (int row = 0; row < _rows; ++row) {
                for (int column = 0; column < _columns; ++column)
                    h.write<Byte>(static_cast<int>(_data[row*_columns + column]));
                    //printf("%i ", static_cast<int>(_data[row*_columns + column]));
                //printf("\n");
            }
        }

        // Solve system of linear equations via Gaussian elimination

        // 1: Put matrix into Echelon form
        for (int row = 0; row < _nextRow; ++row) {
            // 1.1: Find the best pivot.
            int r;
            int bestRow = row;
            double largest = 0;
            for (r = row; r < _nextRow; ++r) {
                double e = abs(element(r, row));
                if (e > largest) {
                    largest = e;
                    bestRow = r;
                }
            }
            if (largest <= 1e-10) {
                printf("Matrix has no information about variable %i!\n", row);
                break;
            }

            // 1.2: Swap the found row with the one we're working on
            if (bestRow != row)
                swap(bestRow, row);

            // 1.3: Reduce the row so its leading element is 1
            scale(row, 1.0/element(row, row));

            // 1.4: Eliminate the variable from all the rows below
            for (r = row + 1; r < _nextRow; ++r) {
                // 1.4.1: Find the scale factor
                double s = element(r, row);

                // 1.4.2: Eliminate the row
                bool nonZero = false;
                for (int column = row; column < _columns; ++column) {
                    element(r, column) -= s*element(row, column);
                    if (!nonZero && abs(element(r, column)) < 1e-10)
                        nonZero = true;
                }
                if (!nonZero) {
                    printf("Removing row %i",r);
                    if (r < _nextRow - 1) {
                        swap(r, _nextRow - 1);
                        --r;
                    }
                    --_nextRow;
                }
            }
        }

        // 2: Back-substitute
        for (int row = _nextRow - 1; row >= 0; --row) {
            for (int r = 0; r < row; ++r) {
                int s = element(r, row);
                for (int column = 1024; column < 1027; ++column)
                    element(r, column) -= s*element(row, column);
                element(r, row) = 0;
            }
        }

        AutoStream h = File("matrix2.raw").openWrite();
        for (int row = 0; row < _rows; ++row) {
            for (int column = 0; column < _columns; ++column)
                h.write<Byte>(static_cast<int>(_data[row*_columns + column]));
                //printf("%i ", static_cast<int>(_data[row*_columns + column]));
            //printf("\n");
        }
    }
    void set(int variable, Colour value)
    {
        if (_nextRow == _rows) {
            printf("Not enough rows!\n");
            return;
        }
        for (int column = 0; column < _columns; ++column)
            element(_nextRow, column) = 0;
        element(_nextRow, 1024) = value.x;
        element(_nextRow, 1025) = value.y;
        element(_nextRow, 1026) = value.z;
        element(_nextRow, variable) = 1;
        for (int row = 0; row < _nextRow; ++row) {
            int s = element(row, variable);
            element(row, variable) = 0;
            element(row, 1024) -= s*value.x;
            element(row, 1025) -= s*value.y;
            element(row, 1026) -= s*value.z;
        }
        ++_nextRow;
    }

    Colour getResult(int row)
    {
        double* p = &element(row, 1024);
        return Colour(p[0], p[1], p[2]);
    }

private:
    double& element(int row, int column) { return _data[row*_columns + column]; }
    void scale(int row, double factor)
    {
        for (int column = 0; column < _columns; ++column)
            element(row, column) *= factor;
    }
    void add(int destination, int source)
    {
        for (int column = 0; column < _columns; ++column)
            element(destination, column) += element(source, column);
    }
    void swap(int row1, int row2)
    {
        for (int column = 0; column < _columns; ++column)
            ::swap(element(row1, column), element(row2, column));
    }

    Array<double> _data;
    int _rows;
    int _columns;
    int _nextRow;
};

class CalibrateImage : public Image
{
public:
    void setWindow(ImageWindow2* window)
    {
        _window = window;

        double brightness = -0.124;
        double contrast = 1.052;

        _topDecoded = Bitmap<SRGB>(Vector(760, 240));
        _bottomDecoded = Bitmap<SRGB>(Vector(760, 240));
        //_top.allocate(450*1024);
        //_bottom.allocate(450*1024);
        AutoStream topDecodedStream = File("q:\\top_decoded.raw", true).openRead();
        AutoStream bottomDecodedStream = File("q:\\bottom_decoded.raw", true).openRead();
        topDecodedStream.read(_topDecoded.data(), 760*240*3);
        bottomDecodedStream.read(_bottomDecoded.data(), 760*240*3);
        //AutoStream topStream = File("q:\\top.raw", true).openRead();
        //AutoStream bottomStream = File("q:\\bottom.raw", true).openRead();
        //topStream.read(_topDecoded.data(), 1024*450);
        //bottomStream.read(_bottomDecoded.data(), 1024*450);
        //_topResampled.allocate(1824*240);
        //_bottomResampled.allocate(1824*240);

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
        Colour black = Colour(dcVoltage[0], dcVoltage[0], dcVoltage[0])*256;

        for (int fg = 0; fg < 16; ++fg)
            for (int bg = 0; bg < 16; ++bg)
                for (int bits = 0; bits < 16; ++bits) {
                    Colour srgb = _rgb.toSrgb(_captures[Vector(bg*16 + fg, bits)]);
                    srgb -= black;
                    int c0 = (((bits & 8) != 0) ? fg : bg);
                    int c1 = (((bits & 4) != 0) ? fg : bg);
                    int c2 = (((bits & 2) != 0) ? fg : bg);
                    int c3 = (((bits & 1) != 0) ? fg : bg);
                    int t[4];
                    t[0] = (c3 << 6) | (c0 << 2);
                    t[1] = (c0 << 6) | (c1 << 2) | 1;
                    t[2] = (c1 << 6) | (c2 << 2) | 2;
                    t[3] = (c2 << 6) | (c3 << 2) | 3;

                    for (int row = 0; row < 4; ++row) {
                        for (int column = 0; column < 4; ++column)
                            _matrix.add(t[row], t[column], 1);
                        _matrix.add(t[row], 1024, srgb.x);
                        _matrix.add(t[row], 1025, srgb.y);
                        _matrix.add(t[row], 1026, srgb.z);
                    }
                }
        Colour c(0, 0, 0);
        _matrix.set(0, c);
        _matrix.set(1, c);
        _matrix.set(2, c);
        _matrix.set(3, c);
        c = Colour(dcVoltage[7], dcVoltage[7], dcVoltage[7])*256 - black;
        _matrix.set((0x77 << 2) | 0, c);
        _matrix.set((0x77 << 2) | 1, c);
        _matrix.set((0x77 << 2) | 2, c);
        _matrix.set((0x77 << 2) | 3, c);
        c = Colour(dcVoltage[8], dcVoltage[8], dcVoltage[8])*256 - black;
        _matrix.set((0x88 << 2) | 0, c);
        _matrix.set((0x88 << 2) | 1, c);
        _matrix.set((0x88 << 2) | 2, c);
        _matrix.set((0x88 << 2) | 3, c);
        c = Colour(dcVoltage[15], dcVoltage[15], dcVoltage[15])*256 - black;
        _matrix.set((0xff << 2) | 0, c);
        _matrix.set((0xff << 2) | 1, c);
        _matrix.set((0xff << 2) | 2, c);
        _matrix.set((0xff << 2) | 3, c);
        c = _rgb.toSrgb(_captures[Vector(15, 8)]);
        _matrix.set((0x0f << 2) | 3, c/2);
        _matrix.set((0xf0 << 2) | 0, c/2);
        c = _rgb.toSrgb(_captures[Vector(15, 4)]);
        _matrix.set((0x0f << 2) | 0, c/2);
        _matrix.set((0xf0 << 2) | 1, c/2);
        c = _rgb.toSrgb(_captures[Vector(15, 2)]);
        _matrix.set((0x0f << 2) | 1, c/2);
        _matrix.set((0xf0 << 2) | 2, c/2);
        c = _rgb.toSrgb(_captures[Vector(15, 1)]);
        _matrix.set((0x0f << 2) | 2, c/2);
        _matrix.set((0xf0 << 2) | 3, c/2);



        _matrix.solve();
        for (int i = 0; i < 1024; ++i) {
            Colour c = _matrix.getResult(i);
            printf("%x->%x column %i: %lf %lf %lf\n",(i >> 6), (i >> 2) & 15, i & 3, c.x, c.y, c.z);
        }

        _sliderCount = 23;
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

        _sliders[19] = Slider(40.0/200, 50.0/200, 46.0/200, "Top frequency", &_topFrequency);
        _sliders[20] = Slider(40.0/200, 50.0/200, 46.0/200, "Bottom frequency", &_bottomFrequency);
        _sliders[21] = Slider(0, 8, 4, "Top offset", &_topOffset);
        _sliders[22] = Slider(0, 8, 4, "Bottom offset", &_bottomOffset);


        for (int i = 0; i < _sliderCount; ++i) {
            _sliders[i].setBitmap(_output.subBitmap(Vector(1024, i*8), Vector(512, 8)));
            _sliders[i].draw();
        }
        _slider = -1;
    }

    virtual void draw()
    {
        //Signal p14MHz = Signal::p14MHz();
        //_n14MHz = p14MHz.invert().delay(_u6fRise*Signal::ns(), _u6fFall*Signal::ns()); // U6F
        //dFlipFlop(Signal::p3_58MHz(), p14MHz, &_chromas[6]);  // U43B
        //_chromas[1] = _chromas[6].invert();
        //dFlipFlop(_chromas[6], p14MHz, &_chromas[4]);         // U44A
        //_chromas[3] = _chromas[4].invert();
        //dFlipFlop(_chromas[4], _n14MHz, &_chromas[5]);        // U44B
        //_chromas[2] = _chromas[5].invert();
        //Signal burst = _chromas[6].delay(_u45DataRise*Signal::ns(), _u45DataFall*Signal::ns()); // U45
        //Complex<double> qamBurst;
        //burst.integrate(&_ab, &qamBurst);
        //_chromas[0] = Signal::fromBits(false, false, false, false);
        //_chromas[7] = Signal::fromBits(true, true, true, true);
        ////double s = -sin(33*tau/360);
        ////double c = cos(33*tau/360);
        ////_cb = c*cb - s*sb;
        ////_sb = -(s*cb + c*sb);
        ////_saturation = 0.4/sqrt(_sb*_sb + _cb*_cb);

        //_qamAdjust = -phaseQam((33 + _hue)*Signal::cycle()/360)*
        //    qamBurst.conjugate()*_saturation*_contrast*0.4/qamBurst.modulus();


        //double z0 = 0;
        //double scale = (1824-_topFrequency)/1824;
        //for (int o = 0; o < 1824*240; ++o) {
        //    double a = 0;
        //    double t = 0;
        //    double fk = o*scale + _topOffset;
        //    int k = fk;
        //    fk -= k;
        //    int firstInput = fk - 3;
        //    int lastInput = fk + 3;
        //    for (int j = firstInput; j <= lastInput; ++j) {
        //        double s = lanczos(j + z0);
        //        a += s*_top[j + k];
        //        t += s;
        //    }
        //    _topResampled[o] = a/t;
        //    int k1 = (o + 1)*scale;
        //    z0 += (k1 - k) - scale;
        //}

        //z0 = 0;
        //scale = (1824-_bottomFrequency)/1824;
        //for (int o = 0; o < 1824*240; ++o) {
        //    double a = 0;
        //    double t = 0;
        //    double fk = o*scale + _bottomOffset;
        //    int k = fk;
        //    fk -= k;
        //    int firstInput = fk - 3;
        //    int lastInput = fk + 3;
        //    for (int j = firstInput; j <= lastInput; ++j) {
        //        double s = lanczos(j + z0);
        //        a += s*_top[j + k];
        //        t += s;
        //    }
        //    _bottomResampled[o] = a/t;
        //    int k1 = (o + 1)*scale;
        //    z0 += (k1 - k) - scale;
        //}



        //for (int fg = 0; fg < 16; ++fg)
        //    for (int bg = 0; bg < 16; ++bg)
        //        for (int bits = 0; bits < 16; ++bits) {
        //            int x = (fg << 3) | ((bits & 3) << 1);
        //            int y = (bg << 3) | ((bits & 0xc) >> 1);
        //            SRGB g = generate(fg, bg, bits);
        //            for (int xx = 0; xx < 16; ++xx)
        //                for (int yy = 0; yy < 8; ++yy) {
        //                    Vector p = (Vector(x, y)<<3) + Vector(xx, yy);
        //                    _output[p + Vector(0, 8)] = g;
        //                }
        //        }

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

    //SRGB generate(int foreground, int background, int bits)
    //{
    //    bool b0 = ((bits & 8) != 0);
    //    bool b1 = ((bits & 4) != 0);
    //    bool b2 = ((bits & 2) != 0);
    //    bool b3 = ((bits & 1) != 0);
    //    bool fi = ((foreground & 8) != 0);
    //    bool fr = ((foreground & 4) != 0);
    //    bool fg = ((foreground & 2) != 0);
    //    bool fb = ((foreground & 1) != 0);
    //    bool bi = ((background & 8) != 0);
    //    bool br = ((background & 4) != 0);
    //    bool bg = ((background & 2) != 0);
    //    bool bb = ((background & 1) != 0);
    //    //Signal iIn = Signal::fromBits(b0 ? fi : bi, b1 ? fi : bi, b2 ? fi : bi, b3 ? fi : bi);
    //    //Signal rIn = Signal::fromBits(b0 ? fr : br, b1 ? fr : br, b2 ? fr : br, b3 ? fr : br);
    //    //Signal gIn = Signal::fromBits(b0 ? fg : bg, b1 ? fg : bg, b2 ? fg : bg, b3 ? fg : bg);
    //    //Signal bIn = Signal::fromBits(b0 ? fb : bb, b1 ? fb : bb, b2 ? fb : bb, b3 ? fb : bb);
    //    //Signal i;
    //    //Signal r;
    //    //Signal g;
    //    //Signal b;
    //    //dFlipFlop(iIn, _n14MHz, &i, true);  // U101
    //    //dFlipFlop(rIn, _n14MHz, &r, true);  // U101
    //    //dFlipFlop(gIn, _n14MHz, &g, true);  // U101
    //    //dFlipFlop(bIn, _n14MHz, &b, true);  // U101
    //    //Signal rx = r.delay(_u68bRise*Signal::ns(), _u68bFall*Signal::ns());  // U68B
    //    //Signal gx = g.delay(_u68aRise*Signal::ns(), _u68aFall*Signal::ns());  // U68A
    //    //Signal chroma = multiplex(b, gx, rx, _chromas);  // U45

    //    //return Vector3Cast<UInt8>(integrate(chroma, i));
    //}

    SRGB getDecodedPixel0(int bitmap, Vector p)
    {
        Bitmap<SRGB> b;
        switch (bitmap) {
            case 0: b = _topDecoded; break;
            case 1: b = _bottomDecoded; break;
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

    Bitmap<SRGB> _topDecoded;
    Bitmap<SRGB> _bottomDecoded;
    Array<Byte> _top;
    Array<Byte> _bottom;
    Array<Byte> _topResampled;
    Array<Byte> _bottomResampled;

    Bitmap<SRGB> _output;
    ColourSpace _rgb;

    double _ab;
    Complex<double> _qamAdjust;

    ImageWindow2* _window;

    Slider _sliders[23];
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
    double _topFrequency;
    double _topOffset;
    double _bottomFrequency;
    double _bottomOffset;

    double _voltages[4];
    Bitmap<Colour> _captures;

    Matrix _matrix;
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
