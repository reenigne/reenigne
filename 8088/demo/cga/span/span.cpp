#include "alfe/main.h"
#include "alfe/vectors.h"
#include "alfe/user.h"
#include "alfe/bitmap.h"
#include "alfe/cga.h"
#include "alfe/fix.h"

typedef Fixed<8, Word> UFix8p8;
typedef Fixed<8, Int16> SFix8p8;

typedef Vector3<SFix8p8> Point3;

struct Face
{
    Face() { }
    Face(int colour, std::initializer_list<int> vertices)
    {
        _colour = colour;
        _nVertices = vertices.size();
        _vertices.allocate(_nVertices);
        _vertex0 = &_vertices[0];
        for (int i = 0; i < _nVertices; ++i)
            _vertex0[i] = vertices.begin()[i];
    }
    int _colour;
    int* _vertex0;
    int _nVertices;
private:
    Array<int> _vertices;
};

struct Shape
{
    Shape(std::initializer_list<Point3> vertices, float scale,
        std::initializer_list<Face> faces)
    {
        _nVertices = vertices.size();
        _vertices.allocate(_nVertices);
        _vertex0 = &_vertices[0];
        float distance = (256.0 / 200.0)*(5.0 / 6.0);
        scale = distance/(scale * sqrt(1 + distance*distance));
        for (int i = 0; i < _nVertices; ++i) {
            _vertex0[i] = vertices.begin()[i] * scale;
            _vertex0[i].x = SFix8p8::fromRepresentation(adjust(_vertex0[i].x.representation()));
            _vertex0[i].y = SFix8p8::fromRepresentation(adjust(_vertex0[i].y.representation()));
            _vertex0[i].z = SFix8p8::fromRepresentation(adjust(_vertex0[i].z.representation()));
        }

        _nFaces = faces.size();
        _faces.allocate(_nFaces);
        _face0 = &_faces[0];
        for (int i = 0; i < _nFaces; ++i)
            _face0[i] = faces.begin()[i];
    }
    Point3* _vertex0;
    int _nVertices;
    Face* _face0;
    int _nFaces;
private:
    Array<Point3> _vertices;
    Array<Face> _faces;

    int adjust(int r)
    {
        if (r < 0)
            return -adjust(-r);
        switch (r) {
            case 0: return 0;
            case 66: return 66;
            case 67: return 66;
            case 97: return 98;
            case 107: return 107;
            case 156: return 158;
            case 157: return 158;
            case 173: return 174;
            case 174: return 174;
            case 186: return 186;
            default:
                printf("%i\n",r);
                return r;
        }
    }
};

Byte colours[][2] = {
    {0x00, 0x00},
    {0x55, 0x55},
    {0xaa, 0xaa},
    {0xff, 0xff},
    {0x66, 0x99},
    {0x77, 0xdd},
    {0xbb, 0xee},
    {0x11, 0x44},
    {0x22, 0x88},
    {0x33, 0xcc}};

static const float phi = (sqrt(5.0f) + 1)/2;

Shape shapes[] {
    {{{   -1,    -1,    -1},  // Cube
      {   -1,    -1,     1},
      {   -1,     1,    -1},
      {   -1,     1,     1},
      {    1,    -1,    -1},
      {    1,    -1,     1},
      {    1,     1,    -1},
      {    1,     1,     1}},
      sqrt(3.0f),
     {{1, { 0,  4,  6,  2}},
      {2, { 4,  5,  7,  6}},
      {1, { 5,  1,  3,  7}},
      {2, { 1,  0,  2,  3}},
      {3, { 2,  6,  7,  3}},
      {3, { 0,  1,  5,  4}}}},

    {{{    1,     0,     0},  // Octahedron
      {   -1,     0,     0},
      {    0,     1,     0},
      {    0,    -1,     0},
      {    0,     0,     1},
      {    0,     0,    -1}},
      1,
     {{3, { 4,  2,  0}},
      {2, { 5,  0,  2}},
      {2, { 4,  0,  3}},
      {1, { 5,  3,  0}},
      {2, { 4,  1,  2}},
      {1, { 5,  2,  1}},
      {1, { 4,  3,  1}},
      {3, { 5,  1,  3}}}},

    {{{    1,     1,     1},  // Tetrahedron
      {    1,    -1,    -1},
      {   -1,     1,    -1},
      {   -1,    -1,     1}},
      sqrt(3.0f),
     {{4, { 1,  2,  3}},
      {1, { 0,  3,  2}},
      {2, { 3,  0,  1}},
      {3, { 2,  1,  0}}}},

    {{{  phi,     1,     0},  // Icosahedron
      { -phi,     1,     0},
      {  phi,    -1,     0},
      { -phi,    -1,     0},
      {    1,     0,   phi},
      {    1,     0,  -phi},
      {   -1,     0,   phi},
      {   -1,     0,  -phi},
      {    0,   phi,     1},
      {    0,  -phi,     1},
      {    0,   phi,    -1},
      {    0,  -phi,    -1}},
      sqrt(phi*phi + 1)*1.01f,
     {{1, { 4,  8,  0}},
      {2, {10,  5,  0}},
      {2, { 9,  4,  2}},
      {3, { 5, 11,  2}},
      {2, { 8,  6,  1}},
      {6, { 7, 10,  1}},
      {5, { 6,  9,  3}},
      {2, {11,  7,  3}},
      {3, { 8, 10,  0}},
      {5, {10,  8,  1}},
      {1, {11,  9,  2}},
      {6, { 9, 11,  3}},
      {5, { 0,  2,  4}},
      {6, { 2,  0,  5}},
      {1, { 3,  1,  6}},
      {3, { 1,  3,  7}},
      {6, { 4,  6,  8}},
      {3, { 6,  4,  9}},
      {1, { 7,  5, 10}},
      {5, { 5,  7, 11}}}},

    {{{    1,     1,     1},  // Dodecahedron
      {    1,     1,    -1},
      {    1,    -1,     1},
      {    1,    -1,    -1},
      {   -1,     1,     1},
      {   -1,     1,    -1},
      {   -1,    -1,     1},
      {   -1,    -1,    -1},
      {phi-1,   phi,     0},
      {1-phi,   phi,     0},
      {phi-1,  -phi,     0},
      {1-phi,  -phi,     0},
      {  phi,     0, phi-1},
      {  phi,     0, 1-phi},
      { -phi,     0, phi-1},
      { -phi,     0, 1-phi},
      {    0, phi-1,   phi},
      {    0, 1-phi,   phi},
      {    0, phi-1,  -phi},
      {    0, 1-phi,  -phi}},
      sqrt(3.0f),
     {{1, {13, 12,  0,  8,  1}},
      {1, {14, 15,  5,  9,  4}},
      {4, {12, 13,  3, 10,  2}},
      {2, {15, 14,  6, 11,  7}},
      {2, {17, 16,  0, 12,  2}},
      {2, {18, 19,  3, 13,  1}},
      {4, {16, 17,  6, 14,  4}},
      {3, {19, 18,  5, 15,  7}},
      {3, { 9,  8,  0, 16,  4}},
      {3, {10, 11,  6, 17,  2}},
      {4, { 8,  9,  5, 18,  1}},
      {1, {11, 10,  3, 19,  7}}}}};


typedef Fixed<16, Int32> Fix16p16;
typedef Fixed<8, Int32> Fix24p8;
typedef Vector2<UFix8p8> Point2;

class SineTable
{
public:
    SineTable()
    {
        for (int i = 0; i < 2560; ++i) {
            double s = ::sin(i*tau/2048.0);
            _table[i] = s;
            _halfTable[i] = s/2.0;
        }
    }
    SFix8p8 sin(int a) { return _table[a]; }
    SFix8p8 cos(int a) { return _table[a + 512]; }
    SFix8p8 coscos(int a, int b)
    {
        return _halfTable[(a+b & 0x7ff) + 512] +
            _halfTable[(a-b & 0x7ff) + 512];
    }
    SFix8p8 sinsin(int a, int b)
    {
        return _halfTable[(a-b & 0x7ff) + 512] -
            _halfTable[(a+b & 0x7ff) + 512];
    }
    SFix8p8 sincos(int a, int b)
    {
        return _halfTable[a+b & 0x7ff] + _halfTable[a-b & 0x7ff];
    }
    SFix8p8 cossin(int a, int b)
    {
        return _halfTable[a+b & 0x7ff] - _halfTable[a-b & 0x7ff];
    }
private:
    SFix8p8 _table[2560];
    SFix8p8 _halfTable[2560];
};

struct TransformedPoint
{
    Vector3<Fix16p16> _xyz;
    Vector2<Fix24p8> _xy;
    SFix8p8 _z;
};

SineTable sines;

class Projection
{
public:
    void init(int theta, int phi, SFix8p8 distance, Vector3<SFix8p8> scale,
        Vector2<SFix8p8> offset)
    {
        Vector3<SFix8p8> s(scale.x*distance, scale.y*distance, scale.z);
        s.x = SFix8p8::fromRepresentation(0x7f5c);
        s.y = SFix8p8::fromRepresentation(0x6a22);
        _xx = s.x*sines.sin(theta);
        _xy = -s.y*(sines.cos(theta)*sines.cos(phi)); //sines.coscos(theta, phi);
        _xz = -s.z*(sines.cos(theta)*sines.sin(phi)); //sines.cossin(theta, phi);
        _yx = s.x*sines.cos(theta);
        _yy = s.y*(sines.sin(theta)*sines.cos(phi)); //sines.sincos(theta, phi);
        _yz = s.z*(sines.sin(theta)*sines.sin(phi)); //sines.sinsin(theta, phi);
        _zy = s.y*sines.sin(phi);
        _zz = -s.z*sines.cos(phi);
        _distance = distance;
        _offset = offset;
    }
    TransformedPoint modelToScreen(Point3 model)
    {
        TransformedPoint r;
        r._xyz.x = lmul(_xx, model.x) + lmul(_yx, model.y);
            /*+ lmul(_zx, model.z)*/
        r._xyz.y = lmul(_xy, model.x) + lmul(_yy, model.y) +
            lmul(_zy, model.z);
        r._xyz.z = lmul(_xz, model.x) + lmul(_yz, model.y) +
            lmul(_zz, model.z);
        Int16 d = ((r._xyz.z.representation() >> 8) + _distance.representation() + 0x500)/5;
        r._z = SFix8p8::fromRepresentation(d);
        r._xy = Vector2<Fix24p8>(
            Fix24p8::fromRepresentation((r._xyz.x.representation() / d +
                _offset.x.representation()) & 0xffffffff),
            Fix24p8::fromRepresentation((r._xyz.y.representation() / d +
                _offset.y.representation()) & 0xffffffff));
        return r;
    }
private:
    SFix8p8 _distance;
    Vector2<SFix8p8> _offset;
    SFix8p8 _xx;
    SFix8p8 _xy;
    SFix8p8 _xz;
    SFix8p8 _yx;
    SFix8p8 _yy;
    SFix8p8 _yz;
    //SFix8p8 _zx;
    SFix8p8 _zy;
    SFix8p8 _zz;
};

int globalCount = 0;

class SpanBuffer
{
public:
    SpanBuffer()
    {
        _lines.allocate(200);
        _lines0 = &_lines[0];
    }
    void clear()
    {
        for (int y = 0; y < 200; ++y)
            _lines0[y].clear();
    }
    void addSpan(int c, int xL, int xR, int y)
    {
        _lines0[y].addSpan(colours[c][y & 1], xL, xR);
    }
    void renderDeltas(Byte* vram, SpanBuffer* last)
    {
        for (int y = 0; y < 200; y += 2) {
            _lines[y].renderDeltas(vram, &last->_lines0[y]);
            _lines[y + 1].renderDeltas(vram + 0x2000, &last->_lines0[y + 1]);
            vram += 80;
        }
    }
private:
    class Line
    {
    public:
        Line()
        {
            _spans.allocate(64);
            _s = &_spans[0];
            _s[0]._x = 0;
            clear();
        }
        void clear()
        {
            // Can remove this first line if we know we're going to be painting
            // the entire screen.
            _s[0]._c = 0;
            _s[1]._x = 255;
            _n = 1;
        }
        void addSpan(int c, int xL, int xR)
        {
            if (xL >= xR)
                return;
            int i;
            for (i = 1; i < _n; ++i)
                if (xL < _s[i]._x)
                    break;
            int j;
            for (j = i; j < _n; ++j)
                if (xR < _s[j]._x)
                    break;
            --i;
            --j;
            if (c == _s[i]._c)
                xL = _s[i]._x;
            else
                if (i > 0 && xL == _s[i]._x && c == _s[i - 1]._c) {
                    --i;
                    xL = _s[i]._x;
                }
            if (c == _s[j]._c)
                xR = _s[j + 1]._x;
            else
                if (j < _n - 1 && xR == _s[j + 1]._x && c == _s[j + 1]._c) {
                    ++j;
                    xR = _s[j + 1]._x;
                }
            int o = j - i;
            if (xL == _s[i]._x) {
                // Left of new span at left of left old
                if (xR == _s[j + 1]._x) {
                    // Right of new span at right of right old
                    _s[i]._c = c;
                    _n -= o;
                    for (int k = i + 1; k <= _n; ++k)
                        _s[k] = _s[k + o];
                }
                else {
                    --o;
                    _n -= o;
                    switch (o) {
                        case -1:
                            // Have to do the update after the move or we'd
                            // be stomping on data we need to keep.
                            for (int k = _n; k >= i - o; --k)
                                _s[k] = _s[k + o];
                            _s[i]._c = c;
                            _s[i + 1]._x = xR;
                            break;
                        case 0:
                            _s[i]._c = c;
                            _s[i + 1]._x = xR;
                            break;
                        default:
                            _s[i]._c = c;
                            _s[i + 1]._x = xR;
                            _s[i + 1]._c = _s[i + 1 + o]._c;
                            for (int k = i + 2; k <= _n; ++k)
                                _s[k] = _s[k + o];
                            break;
                    }
                }
                if (_s[_n]._x != 255 || _s[_n - 1]._c != 0)
                    printf("Error\n");
                return;
            }
            if (xR == _s[j + 1]._x) {
                // Right of new span at right of right old
                --o;
                _n -= o;
                switch (o) {
                    case -1:
                        // Untested
                        _s[i + 1]._x = xL;
                        _s[i + 1]._c = c;
                        for (int k = _n; k > i - o; --k)
                            _s[k] = _s[k + o];
                        break;
                    case 0:
                        _s[i + 1]._x = xL;
                        _s[i + 1]._c = c;
                        break;
                    default:
                        _s[i + 1]._x = xL;
                        _s[i + 1]._c = c;
                        for (int k = i + 2; k <= _n; ++k)
                            _s[k] = _s[k + o];
                        break;
                }
            }
            else {
                o -= 2;
                _n -= o;
                switch (o) {
                    case -2:
                        // Have to do the update after the move
                        for (int k = _n; k > i - o; --k)
                            _s[k] = _s[k + o];
                        _s[i + 2]._c = _s[i]._c;
                        _s[i + 2]._x = xR;
                        _s[i + 1]._x = xL;
                        _s[i + 1]._c = c;
                        break;
                    case -1:
                        // Have to do the update after the move
                        for (int k = _n; k > i - o; --k)
                            _s[k] = _s[k + o];
                        _s[i + 2]._x = xR;
                        _s[i + 1]._x = xL;
                        _s[i + 1]._c = c;
                        break;
                    case 0:
                        _s[i + 2]._x = xR;
                        _s[i + 1]._x = xL;
                        _s[i + 1]._c = c;
                        break;
                    default:
                        // Untested
                        _s[i + 2]._x = xR;
                        _s[i + 2]._c = _s[i + 2 + o]._c;
                        _s[i + 1]._x = xL;
                        _s[i + 1]._c = c;
                        for (int k = i + 3; k <= _n; ++k)
                            _s[k] = _s[k + o];
                }
            }
            if (_s[_n]._x != 255 || _s[_n - 1]._c != 0)
                printf("Error\n");
        }
        //void renderDeltas0(Byte* vram, const Line* o) const
        //{
        //    Span* s = _s;
        //    int xL, xR;
        //    do {
        //        xL = s->_x;
        //        int c = s->_c;
        //        ++s;
        //        xR = s->_x;
        //        for (int x = xL; x < xR; ++x) {
        //            int a = (x >> 2);
        //            int s = (x & 3) << 1;
        //            Byte m = 0xc0 >> s;
        //            vram[a] = (vram[a] & ~m) | (c & m);
        //        }
        //    } while (xR < 255);
        //}
        void renderDeltas(Byte* vram, const Line* o) const
        {
            ++globalCount;
            if (globalCount == 0x00002a47)
                printf("Break");
            Byte* vram0 = vram;

            static const Byte mask[4] = {0xff, 0x3f, 0x0f, 0x03};

            const Span* sn = _s;
            const Span* so = o->_s;
            int cn = sn->_c;
            ++sn;
            int xLn = 0;
            int xRn = sn->_x;
            int co = so->_c;
            ++so;
            int xLo = 0;
            int xRo = so->_x;

            bool havePartial = false;
            Byte partial;
            do {
                if ((xRn & 0xfc) == (xLn & 0xfc)) {
                    if (cn != co || havePartial) {
                        Byte newBits = cn & mask[xLn & 3] & ~mask[xRn & 3];
                        if (!havePartial) {
                            if ((xLn & 3) == 0)
                                partial = 0;
                            else
                                partial = vram[xLn >> 2] & ~mask[xLn & 3];
                            havePartial = true;
                        }
                        partial |= newBits;
                    }
                    else {
                        if ((xRo & 0xfc) == (xLn & 0xfc)) {
                            partial = ((vram[xLn >> 2] & ~mask[xRo & 3]) | (cn & mask[xRo & 3])) & ~mask[xRn & 3];
                            havePartial = true;
                        }
                    }
                }
                else {
                    if (cn != co) {
                        if (!havePartial)
                            partial = vram[xLn >> 2] & ~mask[xLn & 3];
                        vram[xLn >> 2] = partial | (cn & mask[xLn & 3]);
                    }
                    else {
                        if (havePartial)
                            vram[xLn >> 2] = partial | (cn & mask[xLn & 3]);
                        else {
                            if ((xRo & 0xfc) == (xLn & 0xfc)) {
                                Byte* p = vram + (xLn >> 2);
                                *p = (*p & ~mask[xLn & 3]) | (cn & mask[xLn & 3]);
                            }
                        }
                    }
                    xLn = (xLn + 3) & 0xfc;
                    int storeStart = xLn;
                    int storeEnd = xLn;
                    bool store = false;
                    do {
                        if (store) {
                            if (cn == co) {
                                int aligned = (xLo + 3) & 0xfc;
                                if (xRn >= aligned)
                                    storeEnd = aligned;
                                else
                                    storeEnd = xLo;
                                store = false;
                            }
                        }
                        else {
                            if (cn != co) {
                                if ((xLo & 0xfc) > (storeEnd & 0xfc) + 4) {
                                    if (storeStart < storeEnd) {
                                        int startByte = storeStart >> 2;
                                        memset(vram + startByte, cn, (storeEnd >> 2) - startByte);
                                    }
                                    storeStart = xLo;
                                }
                                store = true;
                            }
                        }
                        if (xRo >= xRn)
                            break;
                        xLo = xRo;
                        co = so->_c;
                        ++so;
                        xRo = so->_x;
                    } while (true);
                    if (store)
                        storeEnd = xRn;
                    if (storeStart < storeEnd) {
                        int startByte = storeStart >> 2;
                        memset(vram + startByte, cn, (storeEnd >> 2) - startByte);
                        if ((xRn & 3) != 0) {
                            partial = cn & ~mask[xRn & 3];
                            havePartial = true;
                        }
                        else
                            havePartial = false;
                    }
                    else {
                        if (co != cn && (xRn & 3) != 0) {
                            partial = cn & ~mask[xRn & 3];
                            havePartial = true;
                        }
                        else
                            havePartial = false;
                    }
                }
                xLn = xRn;
                cn = sn->_c;
                ++sn;
                xRn = sn->_x;
                if (xRo == xLn) {
                    xLo = xRo;
                    co = so->_c;
                    ++so;
                    xRo = so->_x;
                }
            } while (xLn < 0xff);

            //Byte vram2[64];
            //renderDeltas0(vram2, o);
            //for (int i = 0; i < 64; ++i)
            //    if (vram0[i] != vram2[i])
            //        printf("Error\n");

//            renderDeltas0(vram0, o);
        }
    private:
        struct Span
        {
            Byte _x;
            Byte _c;
        };
        Span* _s;
        Array<Span> _spans;
        int _n;
    };
    Line* _lines0;
    Array<Line> _lines;
};

class SpanWindow : public RootWindow
{
public:
    SpanWindow()
      : _wisdom(File("wisdom")), _output(&_data, &_sequencer, &_bitmap),
        _theta(0), _phi(0), _dTheta(3), _dPhi(5), _autoRotate(true), _shape(0)
    {
        _output.setConnector(0);          // RGBI
        _output.setScanlineProfile(0);    // rectangle
        _output.setHorizontalProfile(0);  // rectangle
        _output.setScanlineWidth(1);
        _output.setScanlineBleeding(2);   // symmetrical
        _output.setHorizontalBleeding(2); // symmetrical
        _output.setZoom(2);
        _output.setHorizontalRollOff(0);
        _output.setHorizontalLobes(4);
        _output.setVerticalRollOff(0);
        _output.setVerticalLobes(4);
        _output.setSubPixelSeparation(1);
        _output.setPhosphor(0);           // colour
        _output.setMask(0);
        _output.setMaskSize(0);
        _output.setAspectRatio(5.0/6.0);
        _output.setOverscan(0);
        _output.setCombFilter(0);         // no filter
        _output.setHue(0);
        _output.setSaturation(100);
        _output.setContrast(100);
        _output.setBrightness(0);
        _output.setShowClipping(false);
        _output.setChromaBandwidth(1);
        _output.setLumaBandwidth(1);
        _output.setRollOff(0);
        _output.setLobes(1.5);
        _output.setPhase(1);

        static const int regs = -CGAData::registerLogCharactersPerBank;
        Byte cgaRegistersData[regs] = { 0 };
        Byte* cgaRegisters = &cgaRegistersData[regs];
        cgaRegisters[CGAData::registerLogCharactersPerBank] = 12;
        cgaRegisters[CGAData::registerScanlinesRepeat] = 1;
        cgaRegisters[CGAData::registerHorizontalTotalHigh] = 0;
        cgaRegisters[CGAData::registerHorizontalDisplayedHigh] = 0;
        cgaRegisters[CGAData::registerHorizontalSyncPositionHigh] = 0;
        cgaRegisters[CGAData::registerVerticalTotalHigh] = 0;
        cgaRegisters[CGAData::registerVerticalDisplayedHigh] = 0;
        cgaRegisters[CGAData::registerVerticalSyncPositionHigh] = 0;
        cgaRegisters[CGAData::registerMode] = 0x0a;
        cgaRegisters[CGAData::registerPalette] = 0x30;
        cgaRegisters[CGAData::registerHorizontalTotal] = 57 - 1;
        cgaRegisters[CGAData::registerHorizontalDisplayed] = 40;
        cgaRegisters[CGAData::registerHorizontalSyncPosition] = 45;
        cgaRegisters[CGAData::registerHorizontalSyncWidth] = 10;
        cgaRegisters[CGAData::registerVerticalTotal] = 128 - 1;
        cgaRegisters[CGAData::registerVerticalTotalAdjust] = 6;
        cgaRegisters[CGAData::registerVerticalDisplayed] = 100;
        cgaRegisters[CGAData::registerVerticalSyncPosition] = 112;
        cgaRegisters[CGAData::registerInterlaceMode] = 2;
        cgaRegisters[CGAData::registerMaximumScanline] = 1;
        cgaRegisters[CGAData::registerCursorStart] = 6;
        cgaRegisters[CGAData::registerCursorEnd] = 7;
        cgaRegisters[CGAData::registerStartAddressHigh] = 0;
        cgaRegisters[CGAData::registerStartAddressLow] = 0;
        cgaRegisters[CGAData::registerCursorAddressHigh] = 0;
        cgaRegisters[CGAData::registerCursorAddressLow] = 0;
        _data.change(0, -regs, regs, &cgaRegistersData[0]);
        _data.setTotals(238944, 910, 238875);
        _data.change(0, 0, 0x4000, &_vram[0]);

        _outputSize = _output.requiredSize();

        add(&_bitmap);
        add(&_animated);

        _animated.setDrawWindow(this);
        _animated.setRate(1000); //60);

        _buffer = 0;
        _buffers[0].clear();
        _buffers[1].clear();
    }
    void create()
    {
        setText("CGA Span buffer");
        setInnerSize(_outputSize);
        _bitmap.setTopLeft(Vector(0, 0));
        _bitmap.setInnerSize(_outputSize);
        RootWindow::create();
        _animated.start();
    }
    virtual void draw()
    {
        Projection p;
        _theta = (_theta + _dTheta) & 0x7ff;
        _phi = (_phi + _dPhi) & 0x7ff;
        float zs = 1;
        float ys = 99.5f;
        float xs = 6*ys/5;
        float distance = (256.0 / 200.0)*(5.0 / 6.0);
        p.init(_theta, _phi, distance, Vector3<float>(xs, ys, zs),
            Vector2<float>(127.5, 100));

        Shape* shape = &shapes[_shape];
        _corners.ensure(shape->_nVertices);
        TransformedPoint* corners = &_corners[0];

        for (int i = 0; i < shape->_nVertices; ++i)
            corners[i] = p.modelToScreen(shape->_vertex0[i]);

        //memset(&_vram[0], 0, 0x4000);
        //printf("%i %i ",_theta,_phi);
        Face* face = shape->_face0;
        int nFaces = shape->_nFaces;
        for (int i = 0; i < nFaces; ++i, ++face) {
            int* vertices = face->_vertex0;
            TransformedPoint p0 = corners[vertices[0]];
            TransformedPoint p1 = corners[vertices[1]];
            TransformedPoint p2 = corners[vertices[2]];
            vertices += 3;

            Vector2<SInt16> s0(p0._xy.x.representation() >> 9, p0._xy.y.representation() >> 9);
            Vector2<SInt16> s1(p1._xy.x.representation() >> 9, p1._xy.y.representation() >> 9);
            Vector2<SInt16> s2(p2._xy.x.representation() >> 9, p2._xy.y.representation() >> 9);
            s1 -= s0;
            s2 -= s0;
            if (s1.x*s2.y >= s1.y*s2.x)
                continue;
            int c = face->_colour;
            _count = 0;
            fillTriangle(p0._xy, p1._xy, p2._xy, c);
            //printf("%i ", _count);
            //if (_count == 0)
            //    printf("Empty!");
            int nVertices = face->_nVertices - 3;
            for (int i = 0; i < nVertices; ++i) {
                TransformedPoint p3 = corners[*vertices];
                fillTriangle(p0._xy, p2._xy, p3._xy, c);
                p2 = p3;
                ++vertices;
            }
        }
        //printf("\n");
        _buffers[_buffer].renderDeltas(&_vram[0], &_buffers[1 - _buffer]);
        _buffer = 1 - _buffer;
        _buffers[_buffer].clear();
        _data.change(0, 0, 0x4000, &_vram[0]);
        _output.restart();
        _animated.restart();
    }
    bool keyboardEvent(int key, bool up)
    {
        if (up)
            return false;
        switch (key) {
            case VK_RIGHT:
                if (_autoRotate)
                    _dTheta += 1;
                else
                    _theta += 1;
                return true;
            case VK_LEFT:
                if (_autoRotate)
                    _dTheta -= 1;
                else
                    _theta -= 1;
                return true;
            case VK_UP:
                if (_autoRotate)
                    _dPhi -= 1;
                else
                    _phi -= 1;
                return true;
            case VK_DOWN:
                if (_autoRotate)
                    _dPhi += 1;
                else
                    _phi += 1;
                return true;
            case 'N':
                _shape = (_shape + 1) % (sizeof(shapes)/sizeof(shapes[0]));
                return true;
            case VK_SPACE:
                _autoRotate = !_autoRotate;
                if (!_autoRotate) {
                    _dTheta = 0;
                    _dPhi = 0;
                }
                return true;
        }
        return false;
    }
private:
    void horizontalLine(int xL, int xR, int y, int c)
    {
        if (y < 0 || y >= 200 || xL < 0 || xR > 320)
             printf("Error\n");

        _buffers[_buffer].addSpan(c, xL, xR, y);


        //int l = ((y & 1) << 13) + (y >> 1)*80 + 8;
        //c = colours[c][y & 1];

        //for (int x = xL; x < xR; ++x) {
        //    int a = l + (x >> 2);
        //    int s = (x & 3) << 1;
        //    Byte m = 0xc0 >> s;
        //    _vram[a] = (_vram[a] & ~m) | (c & m);
        //    ++_count;
        //}
    }
    void fillTrapezoid(int yStart, int yEnd, UFix8p8 dL, UFix8p8 dR, int c)
    {
        for (int y = yStart; y < yEnd; ++y) {
            horizontalLine(_xL.intFloor(), _xR.intFloor(), y, c);
            _xL += dL;
            _xR += dR;
        }
    }
    UFix8p8 slopeLeft(UFix8p8 dx, UFix8p8 dy, UFix8p8 x0, UFix8p8 y0,
        UFix8p8* x)
    {
        if (dy < 1) {
            *x = x0 - muld(y0, dx, dy);
            return UFix8p8::fromRepresentation(0xffff);
        }
        else {
            UFix8p8 dxdy = dx/dy;
            *x = x0 - y0*dxdy;
            return dxdy;
        }
    }
    UFix8p8 slopeRight(UFix8p8 dx, UFix8p8 dy, UFix8p8 x0, UFix8p8 y0,
        UFix8p8* x)
    {
        if (dy < 1) {
            *x = x0 + muld(y0, dx, dy);
            return UFix8p8::fromRepresentation(0xffff);
        }
        else {
            UFix8p8 dxdy = dx/dy;
            *x = x0 + y0*dxdy;
            return dxdy;
        }
    }
    UFix8p8 slope(UFix8p8 ux, UFix8p8 vx, UFix8p8 dy, UFix8p8 y0, UFix8p8* x)
    {
        if (ux > vx)
            return slopeRight(ux - vx, dy, vx, y0, x);
        return -slopeLeft(vx - ux, dy, vx, y0, x);
    }
    void fillTriangle(Point2 a, Point2 b, Point2 c, int colour)
    {
        if (a.y > b.y) swap(a, b);
        if (b.y > c.y) swap(b, c);
        if (a.y > b.y) swap(a, b);

        if (a.y == b.y) {
            if (b.y == c.y)
                return;
            if (a.x > b.x)
                swap(a, b);
            int yab = a.y.intCeiling();
            int yc = (c.y + 1).intFloor();
            UFix8p8 yac = c.y - a.y;
            UFix8p8 yaa = yab - a.y;
            fillTrapezoid(yab, yc, slope(c.x, a.x, yac, yaa, &_xL), slope(c.x, b.x, yac, yaa, &_xR), colour);
            return;
        }
        int ya = (a.y + 1).intFloor();
        UFix8p8 yab = b.y - a.y;
        if (b.y == c.y) {
            if (b.x > c.x)
                swap(b, c);
            int ybc = (b.y + 1).intFloor();
            UFix8p8 yaa = ya - a.y;
            fillTrapezoid(ya, ybc, slope(b.x, a.x, yab, yaa, &_xL), slope(c.x, a.x, yab, yaa, &_xR), colour);
            return;
        }

        int yb = (b.y + 1).intFloor();
        int yc = (c.y + 1).intFloor();
        UFix8p8 xb;
        UFix8p8 yaa = ya - a.y;
        UFix8p8 ybb = yb - b.y;
        UFix8p8 yac = c.y - a.y;
        UFix8p8 ybc = c.y - b.y;
        if (b.x > a.x) {
            UFix8p8 dab = slopeRight(b.x - a.x, yab, a.x, yaa, &xb);
            if (c.x > a.x) {
                UFix8p8 xc;
                UFix8p8 dac = slopeRight(c.x - a.x, yac, a.x, yaa, &xc);
                if (dab < dac) {
                    _xL = xb;
                    _xR = xc;
                    fillTrapezoid(ya, yb, dab, dac, colour);
                    fillTrapezoid(yb, yc, slope(c.x, b.x, ybc, ybb, &_xL), dac, colour);
                }
                else {
                    _xL = xc;
                    _xR = xb;
                    fillTrapezoid(ya, yb, dac, dab, colour);
                    fillTrapezoid(yb, yc, dac, slope(c.x, b.x, ybc, ybb, &_xR), colour);
                }
            }
            else {
                UFix8p8 dca = slopeLeft(a.x - c.x, yac, a.x, yaa, &_xL);
                _xR = xb;
                fillTrapezoid(ya, yb, -dca, dab, colour);
                fillTrapezoid(yb, yc, -dca, -slopeLeft(b.x - c.x, ybc, b.x, ybb, &_xR), colour);
            }
        }
        else {
            UFix8p8 dba = slopeLeft(a.x - b.x, yab, a.x, yaa, &xb);
            if (c.x > a.x) {
                UFix8p8 dac = slopeRight(c.x - a.x, yac, a.x, yaa, &_xR);
                _xL = xb;
                fillTrapezoid(ya, yb, -dba, dac, colour);
                fillTrapezoid(yb, yc, slopeRight(c.x - b.x, ybc, b.x, ybb, &_xL), dac, colour);
            }
            else {
                UFix8p8 xc;
                UFix8p8 dca = slopeLeft(a.x - c.x, yac, a.x, yaa, &xc);
                if (dba > dca) {
                    _xL = xb;
                    _xR = xc;
                    fillTrapezoid(ya, yb, -dba, -dca, colour);
                    fillTrapezoid(yb, yc, slope(c.x, b.x, ybc, ybb, &_xL), -dca, colour);
                }
                else {
                    _xL = xc;
                    _xR = xb;
                    fillTrapezoid(ya, yb, -dca, -dba, colour);
                    fillTrapezoid(yb, yc, -dca, slope(c.x, b.x, ybc, ybb, &_xR), colour);
                }
            }
        }
    }

    FFTWWisdom<float> _wisdom;
    CGAData _data;
    CGASequencer _sequencer;
    CGAOutput _output;
    AnimatedWindow _animated;
    BitmapWindow _bitmap;
    int _theta;
    int _phi;
    int _dTheta;
    int _dPhi;
    bool _autoRotate;
    Vector _outputSize;
    Byte _vram[0x4000];
    UFix8p8 _xL;
    UFix8p8 _xR;
    bool _fp;
    int _shape;
    Array<TransformedPoint> _corners;
    int _count;
    SpanBuffer _buffers[2];
    int _buffer;
};

class Program : public WindowProgram<SpanWindow>
{
};
