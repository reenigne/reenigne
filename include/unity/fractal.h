#ifndef INCLUDED_FRACTAL_H
#define INCLUDED_FRACTAL_H

#include "unity/complex.h"
#include "unity/user.h"
#include "unity/fix.h"
#include "unity/thread.h"

typedef Fixed<16, Int32, Int64> Fix16p16;

class Region
{
public:
    Region(Vector2<double> min, Vector2<double> max, Vector2<int> size)
      : _min(min),
        _max(max),
        _size(size)
    {
        recalculate();
    }
    void resize(Vector2<int> size)
    {
        _size = size;
        recalculate();
    }

    // Zoom function for ZoomableWindow (zoom box)
    void zoom(Vector2<int> topLeft, Vector2<int> bottomRight, bool in)
    {
        if (topLeft.x==bottomRight.x || topLeft.y==bottomRight.y)
            return;
        if (in) {
            _max = cFromS(bottomRight);
            _min = cFromS(topLeft);
        }
        else {
            Vector2<double> tl = Vector2<double>(topLeft) / _size;
            Vector2<double> br = Vector2<double>(bottomRight) / _size;
            Vector2<double> s = br-tl;
            _min = Complex<double>(
                (br*Vector2<double>(_min) - tl*Vector2<double>(_max))/s);
            _max = _min + Complex<double>(_range/s);
        }
        recalculate();
    }

    // Zoom function for ZoomingWindow (position and magnification)
    void zoom(Region original, Fix16p16 zoomLevel, Vector2<int> s,
        Vector2<double> zoomPoint)
    {
        if (_size.x>1 && _size.y>1) {
            double z = sqrt(
                original._range.modulus2()) * exp(-zoomLevel.toDouble());
            double p = sqrt(static_cast<double>(_size.modulus2()));
            _range = Vector2<double>(_size)*z/p;
            recalculateDelta();
            _min = zoomPoint - Vector2<double>(s)*_delta;
            _max = _min + _range;
        }
    }

    Complex<double> cFromS(Vector2<int> s) const
    {
        return Complex<double>(Vector2<double>(s)*_delta + _min);
    }
    Complex<double> cFromS(Vector2<double> s) const
    {
        return Complex<double>(s*_delta + _min);
    }
    Complex<double> cFromP(Vector2<double> p) const
    {
        return Complex<double>(p*_range + _min);
    }

    Vector2<int> sFromC(Complex<double> c) const
    {
        return Vector2Cast<int>((Vector2<double>(c)-_min)/_delta);
    }
    Vector2<double> getDelta() const { return _delta; }
    int pixels() const { return _size.x*_size.y; }
    Vector2<int> getSize() const { return _size; }
    Vector2<double> getMin() const { return _min; }
    Vector2<double> getMax() const { return _max; }
    int syFromCy(double y) const
    {
        return static_cast<int>((y-_min.y)/_delta.y);
    }
    double cxFromSx(double x) const { return _delta.x*x + _min.x; }

private:
    void recalculateDelta()
    {
        if (_size.x>0 && _size.y>0)
            _delta = _range/_size;
    }
    void recalculate()
    {
        _range = _max-_min;
        recalculateDelta();
    }

    Vector2<double> _min;
    Vector2<double> _max;
    Vector2<int> _size;
    Vector2<double> _range;
    Vector2<double> _delta;
};

class PointGenerator
{
public:
    PointGenerator()
      : _c(0, 0),
        _dc(sqrt(2.0)-1.0, pow(3.0, 1.0/3.0)-1.0)
    { }
    Complex<double> getC(const Region& region)
    {
        _c += _dc;
        if (_c.x >= 1.0)
            _c.x -= 1;
        if (_c.y >= 1.0)
            _c.y -= 1;
        return region.cFromP(_c);
    }
private:
    Vector2<double> _c;
    Vector2<double> _dc;
};

template<class Base> class ZoomableWindow : public Base
{
public:
    class Params
    {
        friend class ZoomableWindow;
    public:
        Params(typename Base::Params bp) : _bp(bp) { }
    private:
        typename Base::Params _bp;
    };

    ZoomableWindow(Params p)
      : Base(p._bp),
        _region(_image->region()),
        _zoomingIn(false),
        _zoomingOut(false),
        _bandDrawn(false)
    { }

protected:
    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg) {
            case WM_LBUTTONDOWN:
                _zoomingIn = true;
                SetCapture(_hWnd);
                _zoomPosition = vectorFromLParam(lParam);
                break;
            case WM_LBUTTONUP:
                ReleaseCapture();
                if (_zoomingIn) {
                    _zoomingIn = _zoomingOut = false;
                    doZoom(_zoomPosition, vectorFromLParam(lParam), true);
                    eraseBand();
                }
                break;
            case WM_RBUTTONDOWN:
                _zoomingOut = true;
                SetCapture(_hWnd);
                _zoomPosition = vectorFromLParam(lParam);
                break;
            case WM_RBUTTONUP:
                ReleaseCapture();
                if (_zoomingOut) {
                    _zoomingIn = _zoomingOut = false;
                    doZoom(_zoomPosition, vectorFromLParam(lParam), false);
                    eraseBand();
                }
                break;
            case WM_MOUSEMOVE:
                eraseBand();
                if (_zoomingIn || _zoomingOut)
                    drawBand(vectorFromLParam(lParam));
                break;
            case WM_SIZE:
                onResize(vectorFromLParam(lParam));
                return 0;
            case WM_KILLFOCUS:
                ReleaseCapture();
                if (_zoomingIn || _zoomingOut)
                    eraseBand();
                _zoomingIn = _zoomingOut = false;
                break;
        }

        return Base::handleMessage(uMsg, wParam, lParam);
    }

private:
    static Vector2<int> vectorFromLParam(LPARAM lParam)
    {
        return Vector2<int>(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    }

    void onResize(Vector2<int> s)
    {
        if (s.zeroArea())
            return;
        _region.resize(s);
        _image->changeCoords(_region);
        IF_ZERO_THROW(InvalidateRect(_hWnd, NULL, FALSE));
    }

    void doZoom(Vector2<int> topLeft, Vector2<int> bottomRight, bool in)
    {
        if (bottomRight.x<topLeft.x) {
            int z = topLeft.x;
            topLeft = Vector2<int>(bottomRight.x, topLeft.y);
            bottomRight = Vector2<int>(z, bottomRight.y);
        }
        if (bottomRight.y<topLeft.y) {
            int z = topLeft.y;
            topLeft = Vector2<int>(topLeft.x, bottomRight.y);
            bottomRight = Vector2<int>(bottomRight.x, z);
        }
        _region.zoom(topLeft, bottomRight, in);
        _image->changeCoords(_region);
        IF_ZERO_THROW(InvalidateRect(_hWnd, NULL, FALSE));
    }

    void invalidateBand()
    {
        RECT r;
        r.left = _bandPosition.x;
        r.right = _bandPosition.x;
        r.top = min(_bandPosition.y, _zoomPosition.y);
        r.bottom = max(_bandPosition.y, _zoomPosition.y);
        IF_ZERO_THROW(InvalidateRect(_hWnd, &r, FALSE));
        r.left = _zoomPosition.x;
        r.right = _zoomPosition.x;
        r.top = min(_bandPosition.y, _zoomPosition.y);
        r.bottom = max(_bandPosition.y, _zoomPosition.y);
        IF_ZERO_THROW(InvalidateRect(_hWnd, &r, FALSE));
        r.left = min(_bandPosition.x, _zoomPosition.x);
        r.right = max(_bandPosition.x, _zoomPosition.x);
        r.top = _bandPosition.y; r.bottom = _bandPosition.y;
        IF_ZERO_THROW(InvalidateRect(_hWnd, &r, FALSE));
        r.left = min(_bandPosition.x, _zoomPosition.x);
        r.right = max(_bandPosition.x, _zoomPosition.x);
        r.top = _zoomPosition.y; r.bottom = _zoomPosition.y;
        IF_ZERO_THROW(InvalidateRect(_hWnd, &r, FALSE));
    }

    void drawBand(DeviceContext* dc)
    {
        Pen pen(PS_DOT, 0, BLACK_PEN);
        SelectedPen sp(dc, &pen);

        POINT points[5];
        points[0].x = _zoomPosition.x; points[0].y = _zoomPosition.y;
        points[1].x = _zoomPosition.x; points[1].y = _bandPosition.y;
        points[2].x = _bandPosition.x; points[2].y = _bandPosition.y;
        points[3].x = _bandPosition.x; points[3].y = _zoomPosition.y;
        points[4].x = _zoomPosition.x; points[4].y = _zoomPosition.y;

        dc->SetROP2(R2_XORPEN);
        dc->Polyline(points, 5);
    }

    void eraseBand()
    {
        if (!_bandDrawn)
            return;
        _bandDrawn = false;
        WindowDeviceContext dc(*this);
        drawBand(&dc);
    }

    void drawBand(Vector2<int> position)
    {
        eraseBand();
        _bandPosition = position;
        _bandDrawn = true;
        WindowDeviceContext dc(*this);
        drawBand(&dc);
    }

    void doPaint(PaintHandle* paint)
    {
        //if (_bandDrawn)
        //    drawBand(paint);
        Base::doPaint(paint);
        if (_bandDrawn)
            drawBand(paint);
    }

    Region _region;
    Vector2<int> _bandPosition;
    Vector2<int> _zoomPosition;
    bool _bandDrawn;
    bool _zoomingIn;
    bool _zoomingOut;
};

class CalcThread : public Thread
{
public:
    CalcThread(Region region)
      : _ending(false),
        _region(region),
        _newRegion(region)
    { }

    void initialize()
    {
        doRestart();
    }

    Region region() const { return _region; }

    void changeCoords(Region region)
    {
        _newRegion = region;
        if (!_restartRequested) {
            _restartRequested = true;
            _event.wait();
        }
    }

    void end()
    {
        _ending = true;
        join();
    }

protected:
    Region _region;

private:
    void doRestart()
    {
        _region = _newRegion;
        restart();
        _restartRequested = false;
    }

    void threadProc()
    {
        do {
            if (_restartRequested) {
                doRestart();
                _event.signal();
            }
            if (_region.pixels() == 0)
                Sleep(100);
            else
                calculate();
        } while (!_ending);
    }

    virtual void restart() = 0;
    virtual void calculate() = 0;

    Region _newRegion;
    bool _restartRequested;
    bool _ending;
    Event _event;
};

template<class Base> class ZoomingWindow : public Base
{
public:
    class Params
    {
        friend class ZoomingWindow;
    public:
        Params(typename Base::Params bp) : _bp(bp) { }
    private:
        typename Base::Params _bp;
    };

    ZoomingWindow(Params p)
      : Base(p._bp),
        _region(_image->region()),
        _region0(_image->region()),
        _zoomPosition(0,0),
        _zoomPoint(0,0),
        _lbutton(false),
        _rbutton(false),
        _mbutton(false),
        _zoomLevel(0),
        _zoomVelocity(0),
        _maxZoomVelocity(10000/65536.0),
        _zoomAcceleration(1000/65536.0)
    { }

protected:
    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg) {
            case WM_LBUTTONDOWN:
                buttonDown(&_lbutton, lParam);
                break;
            case WM_LBUTTONUP:
                buttonUp(&_lbutton);
                break;
            case WM_RBUTTONDOWN:
                buttonDown(&_rbutton, lParam);
                break;
            case WM_RBUTTONUP:
                buttonUp(&_rbutton);
                break;
            case WM_MBUTTONDOWN:
                buttonDown(&_mbutton, lParam);
                break;
            case WM_MBUTTONUP:
                buttonUp(&_mbutton);
                break;
            case WM_MOUSEMOVE:
                if (_lbutton || _rbutton || _mbutton)
                    zoomPos(vectorFromLParam(lParam));
                break;
            case WM_SIZE:
                {
                    Vector size = vectorFromLParam(lParam);
                    if (!size.zeroArea()) {
                        _region.resize(size);
                        _zoomPosition = size/2;
                        _zoomPoint = _region.cFromS(_zoomPosition);
                        recalculateRegion();
                    }
                }
                return 0;
            case WM_KILLFOCUS:
                _lbutton = _rbutton = _mbutton = false;
                ReleaseCapture();
                break;
        }

        return Base::handleMessage(uMsg, wParam, lParam);
    }

private:
    static Vector2<int> vectorFromLParam(LPARAM lParam)
    {
        return Vector2<int>(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    }

    void buttonDown(bool* button, LPARAM lParam)
    {
        zoomPos(vectorFromLParam(lParam));
        *button = true;
        SetCapture(_hWnd);
    }
    void buttonUp(bool* button)
    {
        *button = false;
        if (!_lbutton && !_rbutton && !_mbutton)
            ReleaseCapture();
    }

    // Recalculate the region - called whenever we zoom or pan.
    void recalculateRegion()
    {
        _region.zoom(_region0, _zoomLevel, _zoomPosition, _zoomPoint);
        _image->changeCoords(_region);
    }

    // Recalculate the zoom level and (if necessary) the region
    void recalculateZoom()
    {
        if (!_lbutton)
            if (!_rbutton) {
                // No buttons pressed - slow to a stop
                if (_zoomVelocity > static_cast<Fix16p16>(0)) {
                    // Decelerate from zooming in
                    _zoomVelocity -= _zoomAcceleration;
                }
                if (_zoomVelocity < static_cast<Fix16p16>(0)) {
                    // Decelerate from zooming out
                    _zoomVelocity += _zoomAcceleration;
                }
            }
            else {
                // Right button pressed only - zoom out
                if (_zoomVelocity > -_maxZoomVelocity) {
                    // Accelerate to zoom out
                    _zoomVelocity -= _zoomAcceleration;
                }
            }
        else
            if (!_rbutton) {
                // Left button pressed only - zoom in
                if (_zoomVelocity < _maxZoomVelocity) {
                    // Accelerate to zoom in
                    _zoomVelocity += _zoomAcceleration;
                }
            }
            else {
                // Both buttons pressed, keep same velocity
            }
        _zoomLevel += _zoomVelocity;
        recalculateRegion();
    }

    // Change the zoom position. Called whenever the mouse is moved.
    void zoomPos(Vector2<int> s)
    {
        if (s == _zoomPosition)
            return;
        _zoomPosition = s;

        // If the middle button is held down we want to pan, which means we
        // want to move _zoomPoint to the new mouse position, i.e. don't
        // change it as we move the mouse.
        if (!_mbutton)
            _zoomPoint = _region.cFromS(s);
    }

    virtual void doPaint(PaintHandle* paint)
    {
        // Render the fractal data to a bitmap (only done when the timer has
        // expired, not on every redraw). This needs to be done before the
        // paint operation so that we paint the most up-to-date possible image.
        //if (_timerExpired) {
            // Update the coordinates for the next frame. This will result in
            // restart() getting called, which will render and then change
            // coordinates.
            recalculateZoom();
        //}

        // Paint the image to the window
        Base::doPaint(paint);

        //if (_timerExpired)
        //    resetTimer();

        //return painted;
    }

    Region _region;
    Region _region0;
    Vector2<int> _zoomPosition;
    Complex<double> _zoomPoint;
    bool _lbutton, _rbutton, _mbutton;
    Fix16p16 _zoomLevel;
    Fix16p16 _zoomVelocity;
    Fix16p16 _maxZoomVelocity;
    Fix16p16 _zoomAcceleration;

    // _zoomLevel is a number describing how zoomed in we are. It increases by
    // a constant whenever you zoom in by a constant factor. So it is related
    // to zoomFactor by zoomFactor = A*exp(k*_zoomLevel) for some constants A
    // and k.

    // A is easy to set because at zoom level 0 we want zoomFactor = 1 so A=1

    // We want increasing _zoomLevel by 1 to be around the smallest noticable
    // zoom, say 1/16 of a pixel in a 4096 pixel wide image. This works out as
    // a zoomFactor of 1/65536.

    // To compute k, look at the ratio of zoomFactors for two consecutive zoom
    // levels zL1 = zL2+1
    // zF1/zF2 = exp(k*zL1) / exp(k*zL2) = exp(k*(zL1-zL2)) = exp(k)
    // so k = log(1+1/65536) ~= 1/65536

    // If your maximum zoom level is 0 it means your precision at 2 is 1/65536

    // float has a precision at 1 of 1/8388607 = 1/2^23 (1/2^(mantissa bits-1))
    // so precision at 2 is 1/2^22 so can support a zoom factor of 2^6
    // (2^(mantissa bits-18)). This is a zoom level of ln(2^6) = 4.16 =
    // l_2(2^6) / l_2(e) = 6/l_2(e) = 6 * log(2)
    //
    // double has a max zoom level of (53-18) * log(2) = 24.3
    //
    // long double has a max zoom level of (65-18) * log(2) = 32.6

    // 32-bit integer arithmetic: need to go from -512 to 511.9999 - 10 bits to
    // the left of the point, 22 bits to the right, so precision is 1/2^22 -
    // same as float at 2.

    // In fractint, precision is 1/2^29 because the bailout radius is fixed at
    // 2 and there are clever bailout optimizations to minimize magnitude of
    // numbers needed.

    // type         bits lg(zl) zoomLevel
    // float          22      6      4.16
    // int            22      6      4.16
    // fractint       29     13      9.01
    // double         51     35     24.3
    // 2*int          54     38     26.3
    // long double    63     47     32.6
    // 3*int          86     70     48.5
    // 4*int         118    102     70.7
    // 1477*int    47254  47238  32743

    // So, to max out the zoom level we need 47264-bit integers (5.7Kb for a
    // single number)! Clearly such numbers will take too long to calculate
    // with, so this limit is unlikely to be reached in practice.

    // Also, at 60Hz with a zoomLevel velocity of 2048/65536 per frame, it will
    // take us nearly 5 hours of zooming!
};

// Designed for use with ZoomableWindow. With this window. We render to the
// bits right before painting (in draw()) and resize the region after calling
// changeCoords(). Otherwise when draw() is called by changeCoords(), the size
// of the bits array will be the wrong one.
template<class FractalCalcThread> class FractalImage : public Image
{
public:
    FractalImage(Region region) : _thread(region) { _thread.start(); }
    void paint(const PaintHandle& paint)
    {
        _thread.draw(getBits(), _byteWidth);
        Image::paint(paint);
    }
    void changeCoords(const Region& region)
    {
        _thread.changeCoords(region);
        resize(region.getSize());
    }
    ~FractalImage() { _thread.end(); }
    Region region() const { return _thread.region(); }
    template<class WindowType> void setImageWindow(WindowType* iw)
    {
        _thread.setImageWindow(iw);
    }
    void destroy() { }
private:
    FractalCalcThread _thread;
};

// Designed for use with ZoomingWindow. With this window, we need to resize the
// region first so that the thread has the latest data when changeCoords() is
// called. There is no draw() here, because changeCoords() will render to
// the bits.
template<class FractalCalcThread, class Base = Image> class FractalImage2
  : public Base
{
public:
    FractalImage2(Region region) : _thread(region) { _thread.start(); }
    void changeCoords(const Region& region)
    {
        resize(region.getSize());
        _thread.setImageParameters(getBits(), _byteWidth);
        _thread.changeCoords(region);
    }
    ~FractalImage2() { _thread.end(); }
    Region region() const { return _thread.region(); }
    template<class WindowType> void setImageWindow(WindowType* iw)
    {
        _thread.setImageWindow(iw);
    }
private:
    FractalCalcThread _thread;
};

Byte sinewave[0x400] = {
    128,128,129,130,131,131,132,133,134,135,135,136,137,138,138,139,
    140,141,142,142,143,144,145,145,146,147,148,149,149,150,151,152,
    152,153,154,155,155,156,157,158,158,159,160,161,162,162,163,164,
    165,165,166,167,167,168,169,170,170,171,172,173,173,174,175,176,
    176,177,178,178,179,180,181,181,182,183,183,184,185,186,186,187,
    188,188,189,190,190,191,192,192,193,194,194,195,196,196,197,198,
    198,199,200,200,201,202,202,203,203,204,205,205,206,207,207,208,
    208,209,210,210,211,211,212,213,213,214,214,215,215,216,217,217,
    218,218,219,219,220,220,221,221,222,222,223,224,224,225,225,226,
    226,227,227,228,228,228,229,229,230,230,231,231,232,232,233,233,
    234,234,234,235,235,236,236,236,237,237,238,238,238,239,239,240,
    240,240,241,241,241,242,242,242,243,243,243,244,244,244,245,245,
    245,246,246,246,246,247,247,247,248,248,248,248,249,249,249,249,
    250,250,250,250,250,251,251,251,251,251,252,252,252,252,252,252,
    253,253,253,253,253,253,253,254,254,254,254,254,254,254,254,254,
    254,254,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,254,
    254,254,254,254,254,254,254,254,254,254,253,253,253,253,253,253,
    253,252,252,252,252,252,252,251,251,251,251,251,250,250,250,250,
    250,249,249,249,249,248,248,248,248,247,247,247,246,246,246,246,
    245,245,245,244,244,244,243,243,243,242,242,242,241,241,241,240,
    240,240,239,239,238,238,238,237,237,236,236,236,235,235,234,234,
    234,233,233,232,232,231,231,230,230,229,229,228,228,228,227,227,
    226,226,225,225,224,224,223,222,222,221,221,220,220,219,219,218,
    218,217,217,216,215,215,214,214,213,213,212,211,211,210,210,209,
    208,208,207,207,206,205,205,204,203,203,202,202,201,200,200,199,
    198,198,197,196,196,195,194,194,193,192,192,191,190,190,189,188,
    188,187,186,186,185,184,183,183,182,181,181,180,179,178,178,177,
    176,176,175,174,173,173,172,171,170,170,169,168,167,167,166,165,
    165,164,163,162,162,161,160,159,158,158,157,156,155,155,154,153,
    152,152,151,150,149,149,148,147,146,145,145,144,143,142,142,141,
    140,139,138,138,137,136,135,135,134,133,132,131,131,130,129,128,
    128,127,126,125,124,124,123,122,121,120,120,119,118,117,117,116,
    115,114,113,113,112,111,110,110,109,108,107,106,106,105,104,103,
    103,102,101,100,100, 99, 98, 97, 97, 96, 95, 94, 93, 93, 92, 91,
     90, 90, 89, 88, 88, 87, 86, 85, 85, 84, 83, 82, 82, 81, 80, 79,
     79, 78, 77, 77, 76, 75, 74, 74, 73, 72, 72, 71, 70, 69, 69, 68,
     67, 67, 66, 65, 65, 64, 63, 63, 62, 61, 61, 60, 59, 59, 58, 57,
     57, 56, 55, 55, 54, 53, 53, 52, 52, 51, 50, 50, 49, 48, 48, 47,
     47, 46, 45, 45, 44, 44, 43, 42, 42, 41, 41, 40, 40, 39, 38, 38,
     37, 37, 36, 36, 35, 35, 34, 34, 33, 33, 32, 31, 31, 30, 30, 29,
     29, 28, 28, 27, 27, 27, 26, 26, 25, 25, 24, 24, 23, 23, 22, 22,
     21, 21, 21, 20, 20, 19, 19, 19, 18, 18, 17, 17, 17, 16, 16, 15,
     15, 15, 14, 14, 14, 13, 13, 13, 12, 12, 12, 11, 11, 11, 10, 10,
     10,  9,  9,  9,  9,  8,  8,  8,  7,  7,  7,  7,  6,  6,  6,  6,
      5,  5,  5,  5,  5,  4,  4,  4,  4,  4,  3,  3,  3,  3,  3,  3,
      2,  2,  2,  2,  2,  2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,
      1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,
      1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,
      2,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,  5,
      5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9,  9,
     10, 10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 14, 15,
     15, 15, 16, 16, 17, 17, 17, 18, 18, 19, 19, 19, 20, 20, 21, 21,
     21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 27, 28, 28,
     29, 29, 30, 30, 31, 31, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37,
     37, 38, 38, 39, 40, 40, 41, 41, 42, 42, 43, 44, 44, 45, 45, 46,
     47, 47, 48, 48, 49, 50, 50, 51, 52, 52, 53, 53, 54, 55, 55, 56,
     57, 57, 58, 59, 59, 60, 61, 61, 62, 63, 63, 64, 65, 65, 66, 67,
     67, 68, 69, 69, 70, 71, 72, 72, 73, 74, 74, 75, 76, 77, 77, 78,
     79, 79, 80, 81, 82, 82, 83, 84, 85, 85, 86, 87, 88, 88, 89, 90,
     90, 91, 92, 93, 93, 94, 95, 96, 97, 97, 98, 99,100,100,101,102,
    103,103,104,105,106,106,107,108,109,110,110,111,112,113,113,114,
    115,116,117,117,118,119,120,120,121,122,123,124,124,125,126,127};

Byte gammaSinewave[0x400] = {
     55, 56, 57, 57, 58, 59, 60, 60, 61, 62, 63, 64, 64, 65, 66, 67,
     68, 69, 69, 70, 71, 72, 73, 74, 75, 75, 76, 77, 78, 79, 80, 81,
     82, 83, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
     97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,112,
    113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,
    129,130,131,132,134,135,136,137,138,139,140,141,142,143,144,145,
    146,147,148,149,150,151,153,154,155,156,157,158,159,160,161,162,
    163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,
    179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,
    195,196,197,198,199,200,201,202,203,203,204,205,206,207,208,209,
    210,210,211,212,213,214,215,215,216,217,218,219,219,220,221,222,
    223,223,224,225,226,226,227,228,228,229,230,230,231,232,232,233,
    234,234,235,235,236,237,237,238,238,239,239,240,241,241,242,242,
    243,243,244,244,244,245,245,246,246,247,247,247,248,248,248,249,
    249,249,250,250,250,251,251,251,251,252,252,252,252,253,253,253,
    253,253,253,254,254,254,254,254,254,254,254,254,254,254,254,254,
    255,254,254,254,254,254,254,254,254,254,254,254,254,254,253,253,
    253,253,253,253,252,252,252,252,251,251,251,251,250,250,250,249,
    249,249,248,248,248,247,247,247,246,246,245,245,244,244,244,243,
    243,242,242,241,241,240,239,239,238,238,237,237,236,235,235,234,
    234,233,232,232,231,230,230,229,228,228,227,226,226,225,224,223,
    223,222,221,220,219,219,218,217,216,215,215,214,213,212,211,210,
    210,209,208,207,206,205,204,203,203,202,201,200,199,198,197,196,
    195,194,193,192,191,190,189,188,187,186,185,184,183,182,181,180,
    179,178,177,176,175,174,173,172,171,170,169,168,167,166,165,164,
    163,162,161,160,159,158,157,156,155,154,153,151,150,149,148,147,
    146,145,144,143,142,141,140,139,138,137,136,135,134,132,131,130,
    129,128,127,126,125,124,123,122,121,120,119,118,117,116,115,114,
    113,112,111,110,109,108,107,106,105,104,103,102,101,100, 99, 98,
     97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 83,
     82, 81, 80, 79, 78, 77, 76, 75, 75, 74, 73, 72, 71, 70, 69, 69,
     68, 67, 66, 65, 64, 64, 63, 62, 61, 60, 60, 59, 58, 57, 57, 56,
     55, 54, 54, 53, 52, 51, 51, 50, 49, 48, 48, 47, 46, 46, 45, 44,
     44, 43, 42, 42, 41, 41, 40, 39, 39, 38, 37, 37, 36, 36, 35, 34,
     34, 33, 33, 32, 32, 31, 31, 30, 30, 29, 29, 28, 28, 27, 27, 26,
     26, 25, 25, 24, 24, 23, 23, 22, 22, 22, 21, 21, 20, 20, 19, 19,
     19, 18, 18, 18, 17, 17, 16, 16, 16, 15, 15, 15, 14, 14, 14, 13,
     13, 13, 13, 12, 12, 12, 11, 11, 11, 11, 10, 10, 10, 10,  9,  9,
      9,  9,  8,  8,  8,  8,  7,  7,  7,  7,  7,  6,  6,  6,  6,  6,
      6,  5,  5,  5,  5,  5,  5,  4,  4,  4,  4,  4,  4,  4,  3,  3,
      3,  3,  3,  3,  3,  3,  3,  2,  2,  2,  2,  2,  2,  2,  2,  2,
      2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
      1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,
      1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,
      2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,
      3,  3,  3,  4,  4,  4,  4,  4,  4,  4,  5,  5,  5,  5,  5,  5,
      6,  6,  6,  6,  6,  6,  7,  7,  7,  7,  7,  8,  8,  8,  8,  9,
      9,  9,  9, 10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 13, 13,
     13, 13, 14, 14, 14, 15, 15, 15, 16, 16, 16, 17, 17, 18, 18, 18,
     19, 19, 19, 20, 20, 21, 21, 22, 22, 22, 23, 23, 24, 24, 25, 25,
     26, 26, 27, 27, 28, 28, 29, 29, 30, 30, 31, 31, 32, 32, 33, 33,
     34, 34, 35, 36, 36, 37, 37, 38, 39, 39, 40, 41, 41, 42, 42, 43,
     44, 44, 45, 46, 46, 47, 48, 48, 49, 50, 51, 51, 52, 53, 54, 54};

#endif // INCLUDED_FRACTAL_H
