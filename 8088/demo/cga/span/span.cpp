#include "alfe/main.h"
#include "alfe/vectors.h"
#include "alfe/user.h"
#include "alfe/bitmap.h"
#include "alfe/cga.h"
#include "alfe/fix.h"

typedef Vector3<float> Point3;

Point3 cubeCorners[8] = {
    Point3(-1, -1, -1),
    Point3(-1, -1,  1),
    Point3(-1,  1, -1),
    Point3(-1,  1,  1),
    Point3( 1, -1, -1),
    Point3( 1, -1,  1),
    Point3( 1,  1, -1),
    Point3( 1,  1,  1)};

class Quad
{
public:
    Quad(int p0, int p1, int p2, int p3, int colour)
      : _colour(colour)
    {
        _points[0] = p0;
        _points[1] = p1;
        _points[2] = p2;
        _points[3] = p3;
    }
    int _points[4];
    int _colour;
};

Quad cubeFaces[6] = {
    Quad(0, 4, 6, 2, 1),
    Quad(4, 5, 7, 6, 2),
    Quad(5, 1, 3, 7, 1),
    Quad(1, 0, 2, 3, 2),
    Quad(2, 6, 7, 3, 3),
    Quad(0, 1, 5, 4, 3)
};

typedef Fixed<8, Word> Fix8p8;
//typedef Fixed<16, Int32> Fix16p16;
//typedef float Fix8p8;
typedef Vector2<Fix8p8> Point2;

class Projection
{
public:
    void init(float theta, float phi, float distance, Vector3<float> scale,
        Vector2<float> offset)
    {
        float st = sin(theta);
        float ct = cos(theta);
        float sp = sin(phi);
        float cp = cos(phi);
        Vector3<float> s(scale.x*distance, scale.y*distance, scale.z);
        _xx = s.x*st;
        _xy = -s.y*cp*ct;
        _xz = -s.z*sp*ct;
        _yx = s.x*ct;
        _yy = s.y*cp*st;
        _yz = s.z*sp*st;
        _zy = s.y*sp;
        _zz = -s.z*cp;
        _distance = distance;
        _offset = offset;
    }
    Point2 modelToScreen(Point3 model)
    {
        Point3 r(
            _xx*model.x + _yx*model.y /*+ _zx*model.z*/,
            _xy*model.x + _yy*model.y + _zy*model.z,
            _xz*model.x + _yz*model.y + _zz*model.z + _distance);
        return Point2(r.x/r.z + _offset.x, r.y/r.z + _offset.y); //, r.z);
    }
private:
    float _distance;
    Vector2<float> _offset;
    float _xx;
    float _xy;
    float _xz;
    float _yx;
    float _yy;
    float _yz;
    //float _zx;
    float _zy;
    float _zz;
};

class SpanWindow : public RootWindow
{
public:
    SpanWindow()
      : _wisdom(File("wisdom")), _output(&_data, &_sequencer, &_bitmap),
        _theta(0), _phi(0)
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
        _theta += 0.01f;
        float tauf = static_cast<float>(tau);
        if (_theta >= tauf)
            _theta -= tauf;
        _phi += 0.01f*(sqrt(5.0f) + 1)/2;
        if (_phi >= tauf)
            _phi -= tauf;
        float distance = 2.0547091212166765333819588893295; //5;
        float zs = distance/sqrt((distance*distance + 1)*3);
        float ys = 99.5f*zs;
        float xs = 6*ys/5;
        p.init(_theta, _phi, distance, Vector3<float>(xs, ys, zs),
            Vector2<float>(127.5, 99.5));

        Point2 corners[8];
        for (int i = 0; i < 8; ++i)
            corners[i] = p.modelToScreen(cubeCorners[i]);

        memset(&_vram[0], 0, 0x4000);
        memset(&_vram2[0], 0, 0x4000);
        for (int i = 0; i < 6; ++i) {
            Quad* face = &cubeFaces[i];
            Point2 p0 = corners[face->_points[0]];
            Point2 p1 = corners[face->_points[1]];
            Point2 p2 = corners[face->_points[2]];
            Point2 p3 = corners[face->_points[3]];

            if (p1.x > p0.x) {
                if (p1.y > p0.y) {
                    if (p2.x > p0.x) {
                        if (p2.y > p0.y && dmul(p1.x - p0.x, p2.y - p0.y) > dmul(p1.y - p0.y, p2.x - p0.x))
                            continue;
                    }
                    else {
                        if (p2.y > p0.y || dmul(p1.x - p0.x, p0.y - p2.y) < dmul(p1.y - p0.y, p0.x - p2.x))
                            continue;
                    }
                }
                else {
                    if (p2.x > p0.x) {
                        if (p2.y > p0.y || dmul(p1.x - p0.x, p0.y - p2.y) < dmul(p0.y - p1.y, p2.x - p0.x))
                            continue;
                    }
                    else {
                        if (p2.y > p0.y && dmul(p1.x - p0.x, p2.y - p0.y) > dmul(p0.y - p1.y, p0.x - p2.x))
                            continue;
                    }
                }
            }
            else {
                if (p1.y > p0.y) {
                    if (p2.x > p0.x) {
                        if (p2.y < p0.y && dmul(p0.x - p1.x, p0.y - p2.y) > dmul(p1.y - p0.y, p2.x - p0.x))
                            continue;
                    }
                    else {
                        if (p2.y < p0.y || dmul(p0.x - p1.x, p2.y - p0.y) < dmul(p1.y - p0.y, p0.x - p2.x))
                            continue;
                    }
                }
                else {
                    if (p2.x > p0.x) {
                        if (p2.y < p0.y || dmul(p0.x - p1.x, p2.y - p0.y) < dmul(p0.y - p1.y, p2.x - p0.x))
                            continue;
                    }
                    else {
                        if (p2.y < p0.y && dmul(p0.x - p1.x, p0.y - p2.y) > dmul(p0.y - p1.y, p0.x - p2.x))
                            continue;
                    }
                }
            }
            int c = face->_colour;
            fillTriangle(p0, p1, p2, c);
            fillTriangle(p2, p3, p0, c);
        }
        _data.change(0, 0, 0x4000, &_vram[0]);
        _output.restart();
        _animated.restart();
    }
private:
    void horizontalLine(int xL, int xR, int y, int c)
    {
        if (y < 0 || y >= 200 || xL < 0 || xR > 320)
            printf("Error\n");

        int l = ((y & 1) << 13) + (y >> 1)*80 + 8;
        c <<= 6;

        DWORD* p =
            reinterpret_cast<DWORD*>(_bitmap.data() + y*_bitmap.stride()) + xL;
        for (int x = xL; x < xR; ++x) {
            int a = l + (x >> 2);
            int s = (x & 3) << 1;
            _vram[a] = (_vram[a] & ~(0xc0 >> s)) | (c >> s);
        }
    }
    void fillTrapezoid(int yStart, int yEnd, Fix8p8 dL, Fix8p8 dR, int c)
    {
        for (int y = yStart; y < yEnd; ++y) {
            horizontalLine(static_cast<int>(floor(_xL)),
                static_cast<int>(floor(_xR)), y, c);
            _xL += dL;
            _xR += dR;
        }
    }
    Fix8p8 slopeLeft(Fix8p8 dx, Fix8p8 dy, Fix8p8 x0, Fix8p8 y0, Fix8p8* x)
    {
        if (dy < 1) {
            *x = x0 - muld(y0, dx, dy);
            return Fix8p8::fromT(0xffff);
        }
        else {
            Fix8p8 dxdy = dx/dy;
            *x = x0 - y0*dxdy;
            return dxdy;
        }
    }
    Fix8p8 slopeRight(Fix8p8 dx, Fix8p8 dy, Fix8p8 x0, Fix8p8 y0, Fix8p8* x)
    {
        if (dy < 1) {
            *x = x0 + muld(y0, dx, dy);
            return Fix8p8::fromT(0xffff);
        }
        else {
            Fix8p8 dxdy = dx/dy;
            *x = x0 + y0*dxdy;
            return dxdy;
        }
    }
    Fix8p8 slope(Fix8p8 ux, Fix8p8 vx, Fix8p8 dy, Fix8p8 x0, Fix8p8 y0,
        Fix8p8* x)
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
            int yab = static_cast<int>(ceil(a.y));
            int yc = static_cast<int>(floor(c.y + 1));
            Fix8p8 yac = c.y - a.y;
            Fix8p8 yaa = yab - a.y;
            fillTrapezoid(yab, yc, slope(c.x, a.x, yac, a.x, yaa, &_xL), slope(c.x, b.x, yac, b.x, yaa, &_xR), colour);
            return;
        }
        int ya = static_cast<int>(floor(a.y + 1));
        Fix8p8 yab = b.y - a.y;
        if (b.y == c.y) {
            if (b.x > c.x)
                swap(b, c);
            int ybc = static_cast<int>(floor(b.y + 1));
            Fix8p8 yaa = ya - a.y;
            fillTrapezoid(ya, ybc, slope(b.x, a.x, yab, a.x, yaa, &_xL), slope(c.x, a.x, yab, a.x, yaa, &_xR), colour);
            return;
        }

        int yb = static_cast<int>(floor(b.y + 1));
        int yc = static_cast<int>(floor(c.y + 1));
        Fix8p8 xb;
        Fix8p8 yaa = ya - a.y;
        Fix8p8 ybb = yb - b.y;
        Fix8p8 yac = c.y - a.y;
        Fix8p8 ybc = c.y - b.y;
        if (b.x > a.x) {
            Fix8p8 dab = slopeRight(b.x - a.x, yab, a.x, yaa, &xb);
            if (c.x > a.x) {
                Fix8p8 xc;
                Fix8p8 dac = slopeRight(c.x - a.x, yac, a.x, yaa, &xc);
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
                Fix8p8 dca = slopeLeft(a.x - c.x, yac, a.x, yaa, &_xL);
                _xR = xb;
                fillTrapezoid(ya, yb, -dca, dab, colour);
                fillTrapezoid(yb, yc, -dca, -slopeLeft(b.x - c.x, ybc, b.x, ybb, &_xR), colour);
            }
        }
        else {
            Fix8p8 dba = slopeLeft(a.x - b.x, yab, a.x, yaa, &xb);
            if (c.x > a.x) {
                Fix8p8 dac = slopeRight(c.x - a.x, yac, a.x, yaa, &_xR);
                _xL = xb;
                fillTrapezoid(ya, yb, -dba, dac, colour);
                fillTrapezoid(yb, yc, slopeRight(c.x - b.x, ybc, b.x, ybb, &_xL), dac, colour);
            }
            else {
                Fix8p8 xc;
                Fix8p8 dca = slopeLeft(a.x - c.x, yac, a.x, yaa, &xc);
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
    float _theta;
    float _phi;
    Vector _outputSize;
    Byte _vram[0x4000];
    Byte _vram2[0x4000];
    Fix8p8 _xL;
    Fix8p8 _xR;
    bool _fp;
};

class Program : public WindowProgram<SpanWindow>
{
};
