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
        for (int i = 0; i < _nVertices; ++i)
            _vertex0[i] = vertices.begin()[i] * scale;

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
        _xx = s.x*sines.sin(theta);
        _xy = -s.y*sines.coscos(theta, phi);
        _xz = -s.z*sines.cossin(theta, phi);
        _yx = s.x*sines.cos(theta);
        _yy = s.y*sines.sincos(theta, phi);
        _yz = s.z*sines.sinsin(theta, phi);
        _zy = s.y*sines.sin(phi);
        _zz = -s.z*sines.cos(phi);
        _distance = distance;
        _offset = offset;
    }
    TransformedPoint modelToScreen(Point3 model)
    {
        Fix16p16 x = lmul(_xx, model.x) + lmul(_yx, model.y);
            /*+ lmul(_zx, model.z)*/
        Fix16p16 y = lmul(_xy, model.x) + lmul(_yy, model.y) +
            lmul(_zy, model.z);
        Fix16p16 z = lmul(_xz, model.x) + lmul(_yz, model.y) +
            lmul(_zz, model.z);
        Int16 d = (z.representation() >> 8) + _distance.representation();
        TransformedPoint r;
        r._z = SFix8p8::fromRepresentation(d);
        r._xy = Vector2<Fix24p8>(
            Fix24p8::fromRepresentation(x.representation() / d +
                _offset.x.representation()),
            Fix24p8::fromRepresentation(y.representation() / d +
                _offset.y.representation()));
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
        _animated.setRate(60);
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

        for (int i = 0; i < shape->_nVertices; ++i) {
            TransformedPoint s = p.modelToScreen(shape->_vertex0[i]);
            _corners[i] = Point2(s._xy.x, s._xy.y);
        }

        memset(&_vram[0], 0, 0x4000);
        memset(&_vram2[0], 0, 0x4000);
        //printf("%i %i ",_theta,_phi);
        for (int i = 0; i < shape->_nFaces; ++i) {
            const Face* face = &shape->_face0[i];
            Point2 p0 = _corners[face->_vertex0[0]];
            Point2 p1 = _corners[face->_vertex0[1]];
            Point2 p2 = _corners[face->_vertex0[2]];

            if (p1.x > p0.x) {
                if (p1.y > p0.y) {
                    if (p2.x > p0.x) {
                        if (p2.y > p0.y && lmul(p1.x - p0.x, p2.y - p0.y) > lmul(p1.y - p0.y, p2.x - p0.x))
                            continue;
                    }
                    else {
                        if (p2.y > p0.y || lmul(p1.x - p0.x, p0.y - p2.y) < lmul(p1.y - p0.y, p0.x - p2.x))
                            continue;
                    }
                }
                else {
                    if (p2.x > p0.x) {
                        if (p2.y > p0.y || lmul(p1.x - p0.x, p0.y - p2.y) < lmul(p0.y - p1.y, p2.x - p0.x))
                            continue;
                    }
                    else {
                        if (p2.y > p0.y && lmul(p1.x - p0.x, p2.y - p0.y) > lmul(p0.y - p1.y, p0.x - p2.x))
                            continue;
                    }
                }
            }
            else {
                if (p1.y > p0.y) {
                    if (p2.x > p0.x) {
                        if (p2.y < p0.y && lmul(p0.x - p1.x, p0.y - p2.y) > lmul(p1.y - p0.y, p2.x - p0.x))
                            continue;
                    }
                    else {
                        if (p2.y < p0.y || lmul(p0.x - p1.x, p2.y - p0.y) < lmul(p1.y - p0.y, p0.x - p2.x))
                            continue;
                    }
                }
                else {
                    if (p2.x > p0.x) {
                        if (p2.y < p0.y || lmul(p0.x - p1.x, p2.y - p0.y) < lmul(p0.y - p1.y, p2.x - p0.x))
                            continue;
                    }
                    else {
                        if (p2.y < p0.y && lmul(p0.x - p1.x, p0.y - p2.y) > lmul(p0.y - p1.y, p0.x - p2.x))
                            continue;
                    }
                }
            }
            int c = face->_colour;
            _count = 0;
            fillTriangle(p0, p1, p2, c);
            //printf("%i ", _count);
            //if (_count == 0)
            //    printf("Empty!");
            for (int i = 0; i < face->_nVertices - 3; ++i) {
                Point2 p3 = _corners[face->_vertex0[3 + i]];
                fillTriangle(p0, p2, p3, c);
                p2 = p3;
            }
        }
        //printf("\n");
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

        int l = ((y & 1) << 13) + (y >> 1)*80 + 8;
        c = colours[c][y & 1];

        DWORD* p =
            reinterpret_cast<DWORD*>(_bitmap.data() + y*_bitmap.stride()) + xL;
        for (int x = xL; x < xR; ++x) {
            int a = l + (x >> 2);
            int s = (x & 3) << 1;
            Byte m = 0xc0 >> s;
            _vram[a] = (_vram[a] & ~m) | (c & m);
            ++_count;
        }
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
    UFix8p8 slope(UFix8p8 ux, UFix8p8 vx, UFix8p8 dy, UFix8p8 x0, UFix8p8 y0,
        UFix8p8* x)
    {
        if (ux > vx)
            return slopeRight(ux - vx, dy, x0, y0, x);
        return -slopeLeft(vx - ux, dy, x0, y0, x);
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
            fillTrapezoid(yab, yc, slope(c.x, a.x, yac, a.x, yaa, &_xL), slope(c.x, b.x, yac, b.x, yaa, &_xR), colour);
            return;
        }
        int ya = (a.y + 1).intFloor();
        UFix8p8 yab = b.y - a.y;
        if (b.y == c.y) {
            if (b.x > c.x)
                swap(b, c);
            int ybc = (b.y + 1).intFloor();
            UFix8p8 yaa = ya - a.y;
            fillTrapezoid(ya, ybc, slope(b.x, a.x, yab, a.x, yaa, &_xL), slope(c.x, a.x, yab, a.x, yaa, &_xR), colour);
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
                    fillTrapezoid(yb, yc, slope(c.x, b.x, ybc, b.x, ybb, &_xL), dac, colour);
                }
                else {
                    _xL = xc;
                    _xR = xb;
                    fillTrapezoid(ya, yb, dac, dab, colour);
                    fillTrapezoid(yb, yc, dac, slope(c.x, b.x, ybc, b.x, ybb, &_xR), colour);
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
                    fillTrapezoid(yb, yc, slope(c.x, b.x, ybc, b.x, ybb, &_xL), -dca, colour);
                }
                else {
                    _xL = xc;
                    _xR = xb;
                    fillTrapezoid(ya, yb, -dca, -dba, colour);
                    fillTrapezoid(yb, yc, -dca, slope(c.x, b.x, ybc, b.x, ybb, &_xR), colour);
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
    Byte _vram2[0x4000];
    UFix8p8 _xL;
    UFix8p8 _xR;
    bool _fp;
    int _shape;
    Array<Point2> _corners;
    int _count;
};

class Program : public WindowProgram<SpanWindow>
{
};
