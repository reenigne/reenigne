#include "alfe/main.h"
#include "alfe/vectors.h"
#include "alfe/user.h"
#include "alfe/bitmap.h"
#include "alfe/cga.h"

typedef Vector3<float> Point3;
typedef Vector2<float> Point2;

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

class Projection
{
public:
    void init(float theta, float phi, float distance, Vector2<float> scale,
        Vector2<float> offset)
    {
        float st = sin(theta);
        float ct = cos(theta);
        float sp = sin(phi);
        float cp = cos(phi);
        Vector2<float> s = scale*distance;
        _xx = s.x*st;
        _xy = -s.y*cp*ct;
        _xz = -sp*ct;
        _yx = s.x*ct;
        _yy = s.y*cp*st;
        _yz = sp*st;
        _zy = s.y*sp;
        _zz = -cp;
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
        Vector inputSize(320, 200);
        double aspectRatio = 5.0/6.0;
        double overscan = 0.1;
        double zoom = 1.0;
        _outputSize = Vector2Cast<int>(Vector2Cast<double>(inputSize)*zoom*
            Vector2<double>(aspectRatio, 1)*(1 + 2*overscan));

        _output.setConnector(0);          // RGBI
        _output.setScanlineProfile(0);    // rectangle
        _output.setHorizontalProfile(0);  // rectangle
        _output.setScanlineWidth(1);
        _output.setScanlineBleeding(2);   // symmetrical
        _output.setHorizontalBleeding(2); // symmetrical
        _output.setZoom(zoom);
        _output.setHorizontalRollOff(0);
        _output.setHorizontalLobes(4);
        _output.setVerticalRollOff(0);
        _output.setVerticalLobes(4);
        _output.setSubPixelSeparation(1);
        _output.setPhosphor(0);           // colour
        _output.setMask(0);
        _output.setMaskSize(0);
        _output.setAspectRatio(aspectRatio);
        _output.setOverscan(overscan);
        _output.setOutputSize(_outputSize);
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
        float ys = 99.5f/sqrt(3.0f);
        float xs = 6*ys/5;
        p.init(_theta, _phi, 50, Vector2<float>(xs, ys),
            Vector2<float>(159.5, 99.5));

        Point2 corners[8];
        for (int i = 0; i < 8; ++i)
            corners[i] = p.modelToScreen(cubeCorners[i]);

        for (int i = 0; i < 6; ++i) {
            Quad* face = &cubeFaces[i];
            Point2 p0 = corners[face->_points[0]];
            Point2 p1 = corners[face->_points[1]];
            Point2 p2 = corners[face->_points[2]];
            Point2 p3 = corners[face->_points[3]];
            Point2 e1 = p1 - p0;
            Point2 e2 = p2 - p0;
            float d = e1.x*e2.y - e1.y*e2.x;
            if (d < 0) {
                int c = face->_colour;
                fillTriangle(p0, p1, p2, c);
                fillTriangle(p2, p3, p0, c);
            }
        }

        _output.restart();
        _animated.restart();
    }
private:
    void horizontalLine(int xL, int xR, int y, int c)
    {
        int l = ((y & 1) << 13) + (y >> 1)*80;
        c <<= 6;

        DWORD* p =
            reinterpret_cast<DWORD*>(_bitmap.data() + y*_bitmap.stride()) + xL;
        for (int x = xL; x < xR; ++x) {
            int a = l + (x >> 2);
            int s = (x & 3) << 1;
            _vram[a] = (_vram[a] & ~(0xc0 >> s)) | (c >> s);
        }

        _data.change(0, l, 80, &_vram[l]);
    }
    void fillTrapezoid(int yStart, int yEnd, float xL, float xR, float dL,
        float dR, int c)
    {
        for (int y = yStart; y < yEnd; ++y) {
            horizontalLine(static_cast<int>(ceil(xL)),
                static_cast<int>(ceil(xR)), y, c);
            xL += dL;
            xR += dR;
        }
    }
    void fillTriangle(Point2 a, Point2 b, Point2 c, int colour)
    {
        if (a.y > b.y) swap(a, b);
        if (b.y > c.y) swap(b, c);
        if (a.y > b.y) swap(a, b);
        float dab = (b.x - a.x)/(b.y - a.y);
        float dac = (c.x - a.x)/(c.y - a.y);
        float dbc = (c.x - b.x)/(c.y - b.y);

        int ya = static_cast<int>(ceil(a.y));
        int yb = static_cast<int>(ceil(b.y));
        int yc = static_cast<int>(ceil(c.y));
        if (dab < dac) {
            fillTrapezoid(ya, yb, (ya - a.y)*dab + a.x, (ya - a.y)*dac + a.x,
                dab, dac, colour);
            fillTrapezoid(yb, yc, (yb - b.y)*dbc + b.x, (yb - a.y)*dac + a.x,
                dbc, dac, colour);
        }
        else {
            fillTrapezoid(ya, yb, (ya - a.y)*dac + a.x, (ya - a.y)*dab + a.x,
                dac, dab, colour);
            fillTrapezoid(yb, yc, (yb - a.y)*dac + a.x, (yb - b.y)*dbc + b.x,
                dac, dbc, colour);
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
};

class Program : public WindowProgram<SpanWindow>
{
};
