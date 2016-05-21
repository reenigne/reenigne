#include "alfe/main.h"

#ifndef INCLUDED_KNOB_H
#define INCLUDED_KNOB_H

const COLORREF chromaKey = RGB(0xff, 0x0, 0xff);

template<class T> class KnobSlider
{
public:
    KnobSlider()
      : _knob(this), _popup(this), _sliding(false), _useChromaKey(false) { }
    virtual void create()
    {
        _caption.size();
        _lightGrey = getSysColor(COLOR_BTNFACE, 0xc0c0c0);
        _darkGrey = getSysColor(COLOR_GRAYTEXT, 0x7f7f7f);
        _black = getSysColor(COLOR_WINDOWTEXT, 0x000000);
    }
    void setText(String text) { _caption.setText(text); }
    void setHost(T* host)
    {
        _host = host;
        host->add(&_caption);
        host->add(&_knob);
        host->add(&_edit);
        host->add(&_popup);
    }
    void setPositionAndSize(Vector position, Vector size)
    {
        _size = size;
        _caption.autoSize();
        _caption.setPosition(position +
            Vector(0, (size.y - _caption.size().y)/2));
        _knob.setSize(Vector(size.y, size.y));
        _knob.setPosition(Vector(_caption.right(), position.y));
        _edit.setPosition(_knob.topRight() + (size.y - _edit.size().y)/2);
    }
    Vector bottomLeft() { return _caption.bottomLeft(); }
    int right() const { return _edit.right(); }
    void setRange(double low, double high)
    {
        _min = low;
        _max = high;
    }
    virtual void valueSet(double value) = 0;
    void setValue(double value)
    {
        _value = value;
        valueSet(value);
        _knob.draw();
    }
    double getValue() { return _value; }

protected:
    T* _host;
private:
    static DWORD getSysColor(int nIndex, DWORD def)
    {
        // The values returned from GetSysColor have the red channel in the
        // low 8 bits, not the blue channel as with our Bitmap objects.
        DWORD c = GetSysColor(nIndex);
        if (c != 0) {
            return 0xff000000 | (GetRValue(c) << 16) | (GetGValue(c) << 8) |
                GetBValue(c);
        }
        return 0xff000000 | def;
    }

    void knobEvent(Vector position, bool buttonDown)
    {
        if (buttonDown) {
            if (!_sliding) {
                _dragStart = position;
                _valueStart = _value;
                _popup.show(SW_SHOW);
            }
            else
                _popup.update(position);
        }
        if (!buttonDown && _sliding)
            _popup.show(SW_HIDE);
        _sliding = buttonDown;
    }

    class KnobWindow : public BitmapWindow
    {
    public:
        KnobWindow(KnobSlider* host) : _host(host), _created(false) { }
        void draw2()
        {
            if (!_created)
                return;
            Vector s = size();
            Vector2<double> c = Vector2Cast<double>(s)/2.0;
            _bitmap.fill(_host->_lightGrey);
            fillCircle(_bitmap, _host->_darkGrey, c, s.x/2.0);
            double a = clamp(0.0, (_host->_value - _host->_min)/
                (_host->_max - _host->_min), 1.0)*3/4;
            Rotor2<double> r(-a);
            Vector2<double> o = Vector2<double>(-1, 1)*r*s.x/sqrt(8) + c;
            Vector2<double> w = Vector2<double>(1, 1)*r;
            Vector2<double> points[4];
            points[0] = c - w;
            points[1] = c + w;
            points[2] = o - w;
            points[3] = o + w;
            fillParallelogram(_bitmap, &points[0], _host->_black);
        }
        bool mouseInput(Vector position, int buttons)
        {
            if (_hWnd == 0)
                return false;
            bool lButton = (buttons & MK_LBUTTON) != 0;
            POINT point;
            point.x = position.x;
            point.y = position.y;
            IF_ZERO_THROW(ClientToScreen(_hWnd, &point));
            _host->knobEvent(Vector(point.x, point.y), lButton);
            return false; //return lButton;  // Capture when button is down
        }
        void create()
        {
            _host->create();
            BitmapWindow::create();
            _created = true;
            draw();
        }
    private:
        KnobSlider* _host;
        bool _created;
    };
    class PopupWindow : public WindowsWindow
    {
    public:
        PopupWindow(KnobSlider* host) : _host(host)
        {
            _hdcScreen = GetDC(NULL);
            _hdcSrc = CreateCompatibleDC(_hdcScreen);
            _hbmBackBuffer = CreateCompatibleBitmap(_hdcScreen, 100, 100);
            _hbmOld = SelectObject(_hdcSrc, _hbmBackBuffer);
        }
        void setWindows(Windows* windows)
        {
            WindowsWindow::setWindows(windows);
            setClassName(WC_STATIC);
            setStyle(WS_POPUP);
            setExtendedStyle(WS_EX_LAYERED);
        }
        ~PopupWindow()
        {
            SelectObject(_hdcSrc, _hbmOld);
            DeleteObject(_hbmBackBuffer);
            DeleteDC(_hdcSrc);
            ReleaseDC(NULL, _hdcScreen);
        }
        void update(Vector position)
        {
            int length = _host->_size.x;
            double valueLow = _host->_min;
            double valueHigh = _host->_max;
            double valueStart = _host->_valueStart;
            Vector dragStart = _host->_dragStart;
            double value;
            Vector2<double> a;
            if (position == dragStart) {
                value = valueStart;
                a = Vector2<double>(0, length/(valueLow - valueHigh));
            }
            else {
                Vector delta = position - _host->_dragStart;
                double distance = sqrt(delta.modulus2());
                if (delta.x < delta.y)
                    distance = -distance;
                value = valueStart + distance*(valueHigh - valueLow)/length;
                a = Vector2Cast<double>(dragStart - position)/
                    (valueStart - value);
                value = clamp(valueLow, value, valueHigh);
            }
            _host->setValue(value);
            auto b = Vector2Cast<double>(dragStart) - a*valueStart;
            Vector2<double> low = a*valueLow + b;
            Vector2<double> high = a*valueHigh + b;

            double endPadding = _host->_size.y/4;
            Vector2<double> x = (high-low)*endPadding/length;
            Vector2<double> y = 2.0*Vector2<double>(x.y, -x.x);
            Vector2<double> corners[4];
            corners[0] = low - x - y;
            corners[1] = low - x + y;
            corners[2] = high + x - y;
            corners[3] = high + x + y;

            Vector topLeft = Vector2Cast<int>(corners[0]);
            Vector bottomRight = topLeft + Vector(1, 1);
            for (int i = 1; i < 4; ++i) {
                Vector c = Vector2Cast<int>(corners[i]);
                topLeft.x = min(topLeft.x, c.x);
                topLeft.y = min(topLeft.y, c.y);
                bottomRight.x = max(bottomRight.x, c.x + 1);
                bottomRight.y = max(bottomRight.y, c.y + 1);
            }

            Vector s = bottomRight - topLeft;
            if (s.x != _bitmap.size().x || s.y > _bitmap.size().y) {
                _bitmap = Bitmap<DWORD>(Vector(s.x,
                    max(s.y, _bitmap.size().y)));
                SelectObject(_hdcSrc, _hbmOld);
                DeleteObject(_hbmBackBuffer);
                _hbmBackBuffer = CreateCompatibleBitmap(_hdcScreen,
                    _bitmap.size().x, _bitmap.size().y);
                SelectObject(_hdcSrc, _hbmBackBuffer);
            }

            for (int i = 0; i < 4; ++i)
                corners[i] -= topLeft;
            low -= topLeft;
            high -= topLeft;
            _bitmap.fill(0);  // Transparent

            // Background
            fillParallelogram(_bitmap, &corners[0], _host->_lightGrey);

            // Track
            x = (high-low)*2/length;
            y = Vector2<double>(x.y, -x.x);
            corners[0] = low - y;
            corners[1] = low + y;
            corners[2] = high - y;
            corners[3] = high + y;
            fillParallelogram(_bitmap, &corners[0], _host->_darkGrey);

            // Handle
            x = (high-low)*endPadding/(length*2);
            y = 2.0*Vector2<double>(x.y, -x.x);
            Vector2<double> p = low +
                (value - valueLow)*(high - low)/(valueHigh - valueLow);
            corners[0] = p - x - y;
            corners[1] = p - x + y;
            corners[2] = p + x - y;
            corners[3] = p + x + y;
            fillParallelogram(_bitmap, &corners[0], _host->_black);

            if (_host->_useChromaKey) {
                Byte* row = _bitmap.data();
                for (int y = 0; y < _bitmap.size().y; ++y) {
                    DWORD* p = reinterpret_cast<DWORD*>(row);
                    for (int x = 0; x < _bitmap.size().x; ++x) {
                        if (((*p) >> 24) < 0x80)
                            *p = 0xff000000 | chromaKey;
                        else
                            *p = _host->_lightGrey;
                        ++p;
                    }
                    row += _bitmap.stride();
                }
                //setPosition(topLeft);
                //setSize(s);
                invalidate();
                //UpdateWindow(_hWnd);
                return;
            }

            _bmi.bmiHeader.biWidth = s.x;
            _bmi.bmiHeader.biHeight = -s.y;
            IF_ZERO_THROW(SetDIBitsToDevice(
                _hdcSrc,
                0,
                0,
                s.x,
                s.y,
                0,
                0,
                0,
                s.y,
                _bitmap.data(),
                &_bmi,
                DIB_RGB_COLORS));
            POINT ptSrc;
            ptSrc.x = 0;
            ptSrc.y = 0;
            SIZE size;
            size.cx = s.x;
            size.cy = s.y;
            POINT ptDst;
            ptDst.x = topLeft.x;
            ptDst.y = topLeft.y;
            BLENDFUNCTION blend;
            blend.BlendOp = AC_SRC_OVER;
            blend.BlendFlags = 0;
            blend.SourceConstantAlpha = 255;
            blend.AlphaFormat = AC_SRC_ALPHA;
            BOOL r; // = UpdateLayeredWindow(_hWnd, NULL, &ptDst, &size, _hdcSrc,
            //    &ptSrc, 0, &blend, ULW_ALPHA);
            r = 0;
            if (r == 0) {
                _host->_useChromaKey = true;
                //SetLayeredWindowAttributes(_hWnd, chromaKey, 255,
                //    LWA_COLORKEY);
                update(position);
            }
        }
        void create()
        {
            ZeroMemory(&_bmi, sizeof(BITMAPINFO));
            _bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            _bmi.bmiHeader.biPlanes = 1;
            _bmi.bmiHeader.biBitCount = 32;
            _bmi.bmiHeader.biCompression = BI_RGB;
            _bmi.bmiHeader.biSizeImage = 0;
            _bmi.bmiHeader.biXPelsPerMeter = 0;
            _bmi.bmiHeader.biYPelsPerMeter = 0;
            _bmi.bmiHeader.biClrUsed = 0;
            _bmi.bmiHeader.biClrImportant = 0;
              setSize(Vector(100, 100));
              setPosition(Vector(0, 0));
            WindowsWindow::create();
            SetLayeredWindowAttributes(_hWnd, chromaKey, 255,
                LWA_COLORKEY);
        }
        virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            switch (uMsg) {
                case WM_CREATE:
                {
                    BOOL r = SetLayeredWindowAttributes(_hWnd, chromaKey, 255, LWA_COLORKEY);
                    if (r == 0)
                        printf("SetLayerWindowAttributes failed");
                }
                    return 0;
                case WM_PAINT:
                    if (!_host->_useChromaKey)
                        break;
                    {
                        PaintHandle paintHandle(this);
                        printf("%i %i %i %i\n", paintHandle.topLeft().x, paintHandle.topLeft().y, paintHandle.bottomRight().x, paintHandle.bottomRight().y);
                        //TextOut(paintHandle.hDC(), 0, 0, L"Hello, Windows!", 15);
                            //RECT rect;
                            //rect.bottom = size().y;
                            //rect.top = 0;
                            //rect.left = 0;
                            //rect.right = size().x;
                            //FillRect(paintHandle.hDC(), &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));

                        //if (!paintHandle.zeroArea()) {
                            if (!_bitmap.valid())
                                return 0;
                            Vector ptl = Vector(0, 0); // paintHandle.topLeft();
                            Vector pbr = size(); //paintHandle.bottomRight();
                            Vector br = size();
                            pbr = Vector(min(pbr.x, br.x), min(pbr.y, br.y));
                            Vector ps = pbr - ptl;
                            if (ps.x <= 0 || ps.y <= 0)
                                return 0;
                            Vector s = size();
                            _bmi.bmiHeader.biWidth = s.x;
                            _bmi.bmiHeader.biHeight = -s.y;
                            IF_ZERO_THROW(SetDIBitsToDevice(
                                paintHandle.hDC(),
                                ptl.x,
                                ptl.y,
                                ps.x,
                                ps.y,
                                ptl.x,
                                s.y - pbr.y,
                                0,
                                s.y,
                                _bitmap.data(),
                                &_bmi,
                                DIB_RGB_COLORS));
                        //}
                        return 0;
                    }
            }
            return WindowsWindow::handleMessage(uMsg, wParam, lParam);
        }

    private:

        KnobSlider* _host;
        HDC _hdcScreen;
        HDC _hdcSrc;
        HBITMAP _hbmBackBuffer;
        HGDIOBJ _hbmOld;
        Bitmap<DWORD> _bitmap;
        BITMAPINFO _bmi;
    };

    TextWindow _caption;
    KnobWindow _knob;
    EditWindow _edit;
    PopupWindow _popup;
    bool _sliding;
    double _value;
    double _min;
    double _max;
    Vector _dragStart;
    double _valueStart;
    Vector _size;
    bool _useChromaKey;

    DWORD _lightGrey;
    DWORD _darkGrey;
    DWORD _black;

    friend class KnobWindow;
    friend class PopupWindow;


    static bool insideParallelogram(Vector2<double> p, Vector2<double>* points)
    {
        double d = cross(points[1] - points[0], points[2] - points[0]);
        double ud = cross(p - points[0], points[2] - points[0]);
        double vd = cross(points[1] - points[0], p - points[0]);
        return ud >= 0 && vd >= 0 && ud <= d && vd <= d;
    }
    static bool lineSegmentsIntersect(Vector2<double> a0, Vector2<double> a1,
        Vector2<double> b0, Vector2<double> b1)
    {
        double d = cross(a1 - a0, b1 - b0);
        double ud = cross(b0 - a0, b1 - b0);
        double vd = cross(b0 - a0, a1 - a0);
        return ud >= 0 && vd >= 0 && ud <= d && vd <= d;
    }

    static void fillParallelogram(Bitmap<DWORD> bitmap,
        Vector2<double>* points, DWORD colour)
    {
        Vector topLeft = Vector2Cast<int>(points[0]);
        Vector bottomRight = topLeft + Vector(1, 1);
        for (int i = 1; i < 4; ++i) {
            topLeft.x = min(topLeft.x, static_cast<int>(points[i].x));
            topLeft.y = min(topLeft.y, static_cast<int>(points[i].y));
            bottomRight.x =
                max(bottomRight.x, static_cast<int>(points[i].x + 1));
            bottomRight.y =
                max(bottomRight.y, static_cast<int>(points[i].y + 1));
        }
        Vector s = bottomRight - topLeft;
        bitmap = bitmap.subBitmap(topLeft, s);
        Vector2<double> corners[4];
        for (int i = 0; i < 4; ++i)
            corners[i] = points[i] - topLeft;

        if (s.x > s.y) {
            int sx2 = s.x/2;
            Vector2<double> s2(sx2, s.y);
            fillParallelogram(bitmap, corners, colour, Vector2<double>(0, 0),
                s2);
            fillParallelogram(bitmap, corners, colour, Vector2<double>(sx2, 0),
                s2);
        }
        else {
            int sy2 = s.y/2;
            Vector2<double> s2(s.x, sy2);
            fillParallelogram(bitmap, corners, colour, Vector2<double>(0, 0),
                s2);
            fillParallelogram(bitmap, corners, colour, Vector2<double>(0, sy2),
                s2);
        }
    }

    static int fillParallelogram(Bitmap<DWORD> bitmap, Vector2<double>* points,
        DWORD colour, Vector2<double> tl, Vector2<double> size)
    {
        Vector2<double> rect[4];
        rect[0] = tl;
        rect[1] = tl + Vector2<double>(size.x, 0);
        rect[2] = tl + Vector2<double>(0, size.y);
        rect[3] = tl + size;
        int i;
        bool allInside = true;
        bool allOutside = true;
        for (i = 0; i < 4; ++i) {
            if (insideParallelogram(rect[i], points)) {
                allOutside = false;
                if (!allInside)
                    break;
            }
            else {
                allInside = false;
                if (!allOutside)
                    break;
            }
        }
        if (allInside) {
            if (size.x >= 1 && size.y >= 1) {
                bitmap.subBitmap(Vector2Cast<int>(tl),
                    Vector2Cast<int>(size)).fill(colour | 0xff000000);
            }
            return static_cast<int>(size.x * size.y * 256);
        }
        if (allOutside) {
            bool intersects = false;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    if (lineSegmentsIntersect(points[i], points[(i + 1) & 3],
                        rect[j], rect[(j + 1) & 3])) {
                        intersects = true;
                        i = 4;
                        j = 4;
                    }
                }
            }
            if (!intersects)
                return 0;
        }
        if (size.x <= 1.0f/16 && size.y <= 1.0f/16)
            return 1;
        if (size.x > 1 || size.y > 1) {
            Vector2<double> s;
            if (size.x > size.y) {
                s = Vector2<double>(static_cast<int>(size.x)/2, 0);
                fillParallelogram(bitmap, points, colour, tl,
                    Vector2<double>(s.x, size.y));
            }
            else {
                s = Vector2<double>(0, static_cast<int>(size.y)/2);
                fillParallelogram(bitmap, points, colour, tl,
                    Vector2<double>(size.x, s.y));
            }
            fillParallelogram(bitmap, points, colour, tl + s, size - s);
            return 0;
        }
        Vector2<double> s;
        if (size.x > size.y)
            s = Vector2<double>(size.x/2, 0);
        else
            s = Vector2<double>(0, size.y/2);
        int area = fillParallelogram(bitmap, points, colour, tl, size - s) +
            fillParallelogram(bitmap, points, colour, tl + s, size - s);
        if (size.x == 1 && size.y == 1)
            plot(&bitmap[Vector2Cast<int>(tl)], colour, area);
        return area;
    }

    // area/256 of the pixel *p is to be covered by colour
    static void plot(DWORD* p, DWORD colour, int area)
    {
        double coverage = area/256.0;
        double gamma = 2.2;
        double rLinear = pow(((colour >> 16) & 0xff)*coverage/255.0, gamma);
        double gLinear = pow(((colour >> 8) & 0xff)*coverage/255.0, gamma);
        double bLinear = pow((colour & 0xff)*coverage/255.0, gamma);
        DWORD b = *p;
        double bCoverage = ((b >> 24) & 0xff)/255.0;
        double bRLinear = pow(((b >> 16) & 0xff)/255.0, gamma);
        double bGLinear = pow(((b >> 8) & 0xff)/255.0, gamma);
        double bBLinear = pow((b & 0xff)/255.0, gamma);
        bRLinear = bRLinear*(1 - coverage) + rLinear;
        bGLinear = bGLinear*(1 - coverage) + gLinear;
        bBLinear = bBLinear*(1 - coverage) + bLinear;
        bCoverage = bCoverage*(1 - coverage) + coverage;
        int rSRGB = static_cast<int>(pow(bRLinear, 1/gamma)*255.0 + 0.5);
        int gSRGB = static_cast<int>(pow(bGLinear, 1/gamma)*255.0 + 0.5);
        int bSRGB = static_cast<int>(pow(bBLinear, 1/gamma)*255.0 + 0.5);
        int alpha = static_cast<int>(bCoverage*255.0 + 0.5);
        *p = (byteClamp(alpha) << 24) | (byteClamp(rSRGB) << 16) |
            (byteClamp(gSRGB) << 8) | byteClamp(bSRGB);
    }

    static void fillCircle(Bitmap<DWORD> bitmap, DWORD colour,
        Vector2<double> c, double r)
    {
        r *= r;
        Vector2<double> s = Vector2Cast<double>(bitmap.size());
        Vector2<double> m = Vector2Cast<double>(bitmap.size()/2);
        Vector2<double> z(0, 0);
        fillCircle(bitmap, colour, c, r, Vector2<double>(0, 0), m);
        fillCircle(bitmap, colour, c, r, Vector2<double>(m.x, 0),
            Vector2<double>(s.x - m.x, m.y));
        fillCircle(bitmap, colour, c, r, Vector2<double>(0, m.y),
            Vector2<double>(m.x, s.y - m.y));
        fillCircle(bitmap, colour, c, r, m, s - m);
    }

    static int fillCircle(Bitmap<DWORD> bitmap, DWORD colour,
        Vector2<double> c, double r2, Vector2<double> tl, Vector2<double> size)
    {
        Vector2<double> rect[4];
        rect[0] = tl;
        rect[1] = tl + Vector2<double>(size.x, 0);
        rect[2] = tl + Vector2<double>(0, size.y);
        rect[3] = tl + size;
        int i;
        bool allInside = true;
        bool allOutside = true;
        for (i = 0; i < 4; ++i) {
            if ((rect[i] - c).modulus2() < r2) {
                allOutside = false;
                if (!allInside)
                    break;
            }
            else {
                allInside = false;
                if (!allOutside)
                    break;
            }
        }
        if (allInside) {
            if (size.x >= 1 && size.y >= 1) {
                bitmap.subBitmap(Vector2Cast<int>(tl),
                    Vector2Cast<int>(size)).fill(colour | 0xff000000);
            }
            return static_cast<int>(size.x * size.y * 256);
        }
        if (allOutside)
            return 0;
        if (size.x <= 1.0f/16 && size.y <= 1.0f/16)
            return 1;
        if (size.x > 1 || size.y > 1) {
            Vector2<double> s;
            if (size.x > size.y) {
                s = Vector2<double>(static_cast<int>(size.x)/2, 0);
                fillCircle(bitmap, colour, c, r2, tl,
                    Vector2<double>(s.x, size.y));
            }
            else {
                s = Vector2<double>(0, static_cast<int>(size.y)/2);
                fillCircle(bitmap, colour, c, r2, tl,
                    Vector2<double>(size.x, s.y));
            }
            fillCircle(bitmap, colour, c, r2, tl + s, size - s);
            return 0;
        }
        Vector2<double> s;
        if (size.x > size.y)
            s = Vector2<double>(size.x/2, 0);
        else
            s = Vector2<double>(0, size.y/2);
        int area = fillCircle(bitmap, colour, c, r2, tl, size - s) +
            fillCircle(bitmap, colour, c, r2, tl + s, size - s);
        if (size.x == 1 && size.y == 1)
            plot(&bitmap[Vector2Cast<int>(tl)], colour, area);
        return area;
    }
};

#endif // INCLUDED_KNOB_H
