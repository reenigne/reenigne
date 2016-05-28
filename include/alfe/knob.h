#include "alfe/main.h"

#ifndef INCLUDED_KNOB_H
#define INCLUDED_KNOB_H

const COLORREF chromaKey = RGB(0xff, 0x0, 0xff);

template<class T> class KnobSlider
{
public:
    KnobSlider()
      : _knob(this), _popup(this), _edit(this), _sliding(false),
        _useChromaKey(false), _config(0), _size(180, 24), _captionWidth(100)
    { }
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
    void setCaptionWidth(int width) { _captionWidth = width; }
    void autoSize()
    {
        _caption.autoSize();
        int newCaptionWidth = _caption.size().x;
        if (newCaptionWidth < 0 || newCaptionWidth >= 0x4000)
            newCaptionWidth = 100;
        _size.x += newCaptionWidth - _captionWidth;
        _captionWidth = newCaptionWidth;
    }
    void setTopLeft(Vector tl)
    {
        _caption.autoSize();
        _caption.setPosition(tl + Vector(0, (_size.y - _caption.size().y)/2));
        _knob.setSize(Vector(_size.y, _size.y));
        _knob.setPosition(Vector(_caption.left() + _captionWidth, tl.y));

        TEXTMETRIC metric;
        GetTextMetrics(_edit.getDC(), &metric);
        int height = metric.tmHeight;

        int editL = _knob.right() + _size.y/2;
        _edit.setSize(Vector(_size.x + tl.x - editL, height));

        RECT editRect;
        BOOL r = GetWindowRect(_edit.hWnd(), &editRect);
        if (r == 0) {
            editRect.top = 0;
            editRect.bottom = editRect.top + height;
        }

        int editT = _knob.top() +
            (_size.y - (editRect.bottom - editRect.top))/2;
        _edit.setPosition(Vector(editL, editT));

        setValue(_value);
    }
    void setRange(double low, double high)
    {
        _min = low;
        _max = high;
    }
    virtual void valueSet(double value) = 0;
    void setValue(double value)
    {
        setValueFromEdit(value);
        int dps =
            max(0, static_cast<int>(1 - log((_max - _min)/_size.x)/log(10)));
        if (_size.x < 0)
            dps = 1;
        _edit.setText(format("%.*f", dps, value));
    }
    void setValueFromEdit(double value)
    {
        _value = value;
        valueSet(value);
        _knob.draw();
    }
    void changeValue(double amount)
    {
        setValue(clamp(_min, _value + (_max - _min)*amount, _max));
    }
    double getValue() const { return _value; }
    void setConfig(ConfigFile* config) { _config = config; }
    Vector size() const { return _size; }
    int left() const { return _caption.left(); }
    int top() const { return _knob.top(); }
    int right() const { return _edit.right(); }
    int bottom() const { return _knob.bottom(); }
    Vector topLeft() const { return Vector(left(), top()); }
    Vector bottomLeft() const { return Vector(left(), bottom()); }
    Vector topRight() const { return Vector(right(), top()); }
    Vector bottomRight() const { return Vector(right(), bottom()); }

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
            fillCircle(_bitmap, _host->_darkGrey, Vector2Cast<float>(c),
                static_cast<float>(s.x/2.0));
            double a = clamp(0.0, (_host->_value - _host->_min)/
                (_host->_max - _host->_min), 1.0)*3/4;
            Rotor2<double> r(-a);
            Vector2<double> o = Vector2<double>(-1, 1)*r*s.x/sqrt(8) + c;
            Vector2<double> w = Vector2<double>(1, 1)*r;
            Vector2<float> points[4];
            points[0] = Vector2Cast<float>(c - w);
            points[1] = Vector2Cast<float>(c + w);
            points[2] = Vector2Cast<float>(o - w);
            points[3] = Vector2Cast<float>(o + w);
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
            return lButton;  // Capture when button is down
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
        PopupWindow(KnobSlider* host)
          : _host(host), _delta(0, -1), _hdcScreen(NULL),
            _hdcSrc(CreateCompatibleDC(_hdcScreen))
        {
            _hbmBackBuffer =
                GDIObject(CreateCompatibleBitmap(_hdcScreen, 100, 100));
            _hbmOld = SelectedObject(&_hdcSrc, _hbmBackBuffer);
        }
        void setWindows(Windows* windows)
        {
            WindowsWindow::setWindows(windows);
            setStyle(WS_POPUP);
            setExtendedStyle(WS_EX_LAYERED);
        }
        void update(Vector position)
        {
            int length = _host->_size.x;
            double valueLow = _host->_min;
            double valueHigh = _host->_max;
            double valueStart = _host->_valueStart;
            double valueDelta = valueHigh - valueLow;
            Vector dragStart = _host->_dragStart;
            double value;
            Vector2<double> a;
            Vector2<double> delta = Vector2Cast<double>(position - dragStart);
            double distance = sqrt(delta.modulus2());
            if (distance < 40) {
                value = valueStart + dot(delta, _delta)*valueDelta/length;
                a = _delta*length/valueDelta;
            }
            else {
                _delta = delta/sqrt(delta.modulus2());
                if (delta.x < delta.y) {
                    distance = -distance;
                    _delta = -_delta;
                }
                value = valueStart + distance*valueDelta/length;
                a = delta/(value - valueStart);
            }
            value = clamp(valueLow, value, valueHigh);
            _host->setValue(value);
            auto b = Vector2Cast<double>(dragStart) - a*valueStart;
            Vector2<double> low = a*valueLow + b;
            Vector2<double> high = a*valueHigh + b;

            double endPadding = _host->_size.y/4;
            Vector2<double> x = (high-low)*endPadding/length;
            Vector2<double> y = 2.0*Vector2<double>(x.y, -x.x);
            Vector2<float> corners[4];
            corners[0] = Vector2Cast<float>(low - x - y);
            corners[1] = Vector2Cast<float>(low - x + y);
            corners[2] = Vector2Cast<float>(high + x - y);
            corners[3] = Vector2Cast<float>(high + x + y);

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
            if (s.x > _bitmap.size().x || s.y > _bitmap.size().y) {
                _bitmap = Bitmap<DWORD>(Vector(max(s.x, _bitmap.size().x),
                    max(s.y, _bitmap.size().y)));
                _hbmOld = SelectedObject();
                _hbmBackBuffer = GDIObject(CreateCompatibleBitmap(_hdcScreen,
                    _bitmap.size().x, _bitmap.size().y));
                _hbmOld = SelectedObject(&_hdcSrc, _hbmBackBuffer);
            }

            for (int i = 0; i < 4; ++i)
                corners[i] -= Vector2Cast<float>(topLeft);
            low -= topLeft;
            high -= topLeft;
            // Transparent
            if (_host->_useChromaKey)
                _bitmap.fill(0xff000000 | chromaKey);
            else
                _bitmap.fill(0);

            // Background
            fillParallelogram(_bitmap, &corners[0], _host->_lightGrey,
                !_host->_useChromaKey);

            // Track
            x = (high-low)*2/length;
            y = Vector2<double>(x.y, -x.x);
            corners[0] = Vector2Cast<float>(low - y);
            corners[1] = Vector2Cast<float>(low + y);
            corners[2] = Vector2Cast<float>(high - y);
            corners[3] = Vector2Cast<float>(high + y);
            fillParallelogram(_bitmap, &corners[0], _host->_darkGrey);

            // Handle
            x = (high-low)*endPadding/(length*2);
            y = 2.0*Vector2<double>(x.y, -x.x);
            Vector2<double> p = low +
                (value - valueLow)*(high - low)/(valueHigh - valueLow);
            corners[0] = Vector2Cast<float>(p - x - y);
            corners[1] = Vector2Cast<float>(p - x + y);
            corners[2] = Vector2Cast<float>(p + x - y);
            corners[3] = Vector2Cast<float>(p + x + y);
            fillParallelogram(_bitmap, &corners[0], _host->_black);

            sizeSet(s);
            if (_host->_useChromaKey) {
                MoveWindow(_hWnd, topLeft.x, topLeft.y, s.x, s.y, FALSE);
                invalidate();
                updateWindow();
                return;
            }

            setDIBits(_hdcSrc, Vector(0, 0), s);
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
            BOOL r = UpdateLayeredWindow(_hWnd, NULL, &ptDst, &size, _hdcSrc,
                &ptSrc, 0, &blend, ULW_ALPHA);
            if (r == 0) {
                _host->_useChromaKey = true;
                SetLayeredWindowAttributes(_hWnd, chromaKey, 255,
                    LWA_COLORKEY);
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
            WindowsWindow::create();
        }
        virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            if (uMsg == WM_PAINT && _host->_useChromaKey) {
                PaintHandle p(this);
                setDIBits(p, p.topLeft(), p.bottomRight());
                return 0;
            }
            return WindowsWindow::handleMessage(uMsg, wParam, lParam);
        }

    private:
        void setDIBits(HDC hdc, Vector ptl, Vector pbr)
        {
            Vector br = size();
            pbr = Vector(min(pbr.x, br.x), min(pbr.y, br.y));
            Vector ps = pbr - ptl;
            if (ps.x <= 0 || ps.y <= 0 || !_bitmap.valid())
                return;
            Vector s = size();
            _bmi.bmiHeader.biWidth = _bitmap.stride() / sizeof(DWORD);
            _bmi.bmiHeader.biHeight = -s.y;
            IF_ZERO_THROW(SetDIBitsToDevice(
                hdc,
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
        }

        KnobSlider* _host;
        WindowDeviceContext _hdcScreen;
        OwnedDeviceContext _hdcSrc;
        GDIObject _hbmBackBuffer;
        SelectedObject _hbmOld;
        Bitmap<DWORD> _bitmap;
        BITMAPINFO _bmi;
        Vector2<double> _delta;
    };

    class EditControl : public EditWindow
    {
    public:
        EditControl(KnobSlider* host) : _host(host) { }
        void setWindows(Windows* windows)
        {
            EditWindow::setWindows(windows);
            setExtendedStyle(WS_EX_CLIENTEDGE);
            setStyle(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP);
        }
        void changed()
        {
            String t = getText();
            double v = _host->_config->evaluate<double>(t, _host->_value);
            _host->setValueFromEdit(v);
        }
        virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            if (uMsg == WM_KEYDOWN) {
                switch (wParam) {
                    case VK_UP:
                        _host->changeValue(0.01);
                        return 0;
                    case VK_DOWN:
                        _host->changeValue(-0.01);
                        return 0;
                    case VK_PRIOR:
                        _host->changeValue(0.1);
                        return 0;
                    case VK_NEXT:
                        _host->changeValue(-0.1);
                        return 0;
                }
            }
            return EditWindow::handleMessage(uMsg, wParam, lParam);
        }
    private:
        KnobSlider* _host;
    };

    TextWindow _caption;
    KnobWindow _knob;
    EditControl _edit;
    PopupWindow _popup;
    bool _sliding;
    double _value;
    double _min;
    double _max;
    Vector _dragStart;
    double _valueStart;
    Vector _size;
    int _captionWidth;
    bool _useChromaKey;
    DWORD _lightGrey;
    DWORD _darkGrey;
    DWORD _black;
    ConfigFile* _config;

    friend class KnobWindow;
    friend class PopupWindow;
    friend class EditWindow;


    static bool insideParallelogram(Vector2<float> p, Vector2<float>* points)
    {
        float d = cross(points[1] - points[0], points[2] - points[0]);
        float ud = cross(p - points[0], points[2] - points[0]);
        float vd = cross(points[1] - points[0], p - points[0]);
        return ud >= 0 && vd >= 0 && ud <= d && vd <= d;
    }
    static bool lineSegmentsIntersect(Vector2<float> a0, Vector2<float> a1,
        Vector2<float> b0, Vector2<float> b1)
    {
        float d = cross(a1 - a0, b1 - b0);
        float ud = cross(b0 - a0, b1 - b0);
        float vd = cross(b0 - a0, a1 - a0);
        return ud >= 0 && vd >= 0 && ud <= d && vd <= d;
    }

    static void fillParallelogram(Bitmap<DWORD> bitmap,
        Vector2<float>* points, DWORD colour, bool antiAlias = true)
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
        Vector2<float> corners[4];
        for (int i = 0; i < 4; ++i)
            corners[i] = points[i] - Vector2Cast<float>(topLeft);

        if (s.x > s.y) {
            float sx2 = static_cast<float>(s.x/2);
            Vector2<float> s2(sx2, static_cast<float>(s.y));
            fillParallelogram(bitmap, corners, colour, Vector2<float>(0, 0),
                s2, antiAlias);
            fillParallelogram(bitmap, corners, colour, Vector2<float>(sx2, 0),
                s2, antiAlias);
        }
        else {
            float sy2 = static_cast<float>(s.y/2);
            Vector2<float> s2(static_cast<float>(s.x), sy2);
            fillParallelogram(bitmap, corners, colour, Vector2<float>(0, 0),
                s2, antiAlias);
            fillParallelogram(bitmap, corners, colour, Vector2<float>(0, sy2),
                s2, antiAlias);
        }
    }

    static int fillParallelogram(Bitmap<DWORD> bitmap, Vector2<float>* points,
        DWORD colour, Vector2<float> tl, Vector2<float> size, bool antiAlias)
    {
        Vector2<float> rect[4];
        rect[0] = tl;
        rect[1] = tl + Vector2<float>(size.x, 0);
        rect[2] = tl + Vector2<float>(0, size.y);
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
        if (size.x <= 1.0f/8 && size.y <= 1.0f/8)
            return 4;
        if (size.x > 1 || size.y > 1) {
            Vector2<float> s;
            if (size.x > size.y) {
                s = Vector2<float>(
                    static_cast<float>(static_cast<int>(size.x)/2), 0);
                fillParallelogram(bitmap, points, colour, tl,
                    Vector2<float>(s.x, size.y), antiAlias);
            }
            else {
                s = Vector2<float>(0,
                    static_cast<float>(static_cast<int>(size.y)/2));
                fillParallelogram(bitmap, points, colour, tl,
                    Vector2<float>(size.x, s.y), antiAlias);
            }
            fillParallelogram(bitmap, points, colour, tl + s, size - s,
                antiAlias);
            return 0;
        }
        if (!antiAlias) {
            bitmap[Vector2Cast<int>(tl)] = colour | 0xff000000;
            return 0;
        }
        Vector2<float> s;
        if (size.x > size.y)
            s = Vector2<float>(size.x/2, 0);
        else
            s = Vector2<float>(0, size.y/2);
        int area = fillParallelogram(bitmap, points, colour, tl, size - s,
            antiAlias) + fillParallelogram(bitmap, points, colour, tl + s,
                size - s, antiAlias);
        if (size.x == 1 && size.y == 1)
            plot(&bitmap[Vector2Cast<int>(tl)], colour, area);
        return area;
    }

    // area/256 of the pixel *p is to be covered by colour
    static void plot(DWORD* p, DWORD colour, int area)
    {
        float coverage = static_cast<float>(area)/256.0f;
        float gamma = 2.2f;
        float rLinear = pow(((colour >> 16) & 0xff)*coverage/255.0f, gamma);
        float gLinear = pow(((colour >> 8) & 0xff)*coverage/255.0f, gamma);
        float bLinear = pow((colour & 0xff)*coverage/255.0f, gamma);
        DWORD b = *p;
        float bCoverage = ((b >> 24) & 0xff)/255.0f;
        float bRLinear = pow(((b >> 16) & 0xff)/255.0f, gamma);
        float bGLinear = pow(((b >> 8) & 0xff)/255.0f, gamma);
        float bBLinear = pow((b & 0xff)/255.0f, gamma);
        bRLinear = bRLinear*(1 - coverage) + rLinear;
        bGLinear = bGLinear*(1 - coverage) + gLinear;
        bBLinear = bBLinear*(1 - coverage) + bLinear;
        bCoverage = bCoverage*(1 - coverage) + coverage;
        int rSRGB = static_cast<int>(pow(bRLinear, 1/gamma)*255.0f + 0.5f);
        int gSRGB = static_cast<int>(pow(bGLinear, 1/gamma)*255.0f + 0.5f);
        int bSRGB = static_cast<int>(pow(bBLinear, 1/gamma)*255.0f + 0.5f);
        int alpha = static_cast<int>(bCoverage*255.0f + 0.5f);
        *p = (byteClamp(alpha) << 24) | (byteClamp(rSRGB) << 16) |
            (byteClamp(gSRGB) << 8) | byteClamp(bSRGB);
    }

    static void fillCircle(Bitmap<DWORD> bitmap, DWORD colour,
        Vector2<float> c, float r)
    {
        r *= r;
        Vector2<float> s = Vector2Cast<float>(bitmap.size());
        Vector2<float> m = Vector2Cast<float>(bitmap.size()/2);
        Vector2<float> z(0, 0);
        fillCircle(bitmap, colour, c, r, Vector2<float>(0, 0), m);
        fillCircle(bitmap, colour, c, r, Vector2<float>(m.x, 0),
            Vector2<float>(s.x - m.x, m.y));
        fillCircle(bitmap, colour, c, r, Vector2<float>(0, m.y),
            Vector2<float>(m.x, s.y - m.y));
        fillCircle(bitmap, colour, c, r, m, s - m);
    }

    static int fillCircle(Bitmap<DWORD> bitmap, DWORD colour,
        Vector2<float> c, float r2, Vector2<float> tl, Vector2<float> size)
    {
        Vector2<float> rect[4];
        rect[0] = tl;
        rect[1] = tl + Vector2<float>(size.x, 0);
        rect[2] = tl + Vector2<float>(0, size.y);
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
            Vector2<float> s;
            if (size.x > size.y) {
                s = Vector2<float>(
                    static_cast<float>(static_cast<int>(size.x)/2), 0);
                fillCircle(bitmap, colour, c, r2, tl,
                    Vector2<float>(s.x, size.y));
            }
            else {
                s = Vector2<float>(0,
                    static_cast<float>(static_cast<int>(size.y)/2));
                fillCircle(bitmap, colour, c, r2, tl,
                    Vector2<float>(size.x, s.y));
            }
            fillCircle(bitmap, colour, c, r2, tl + s, size - s);
            return 0;
        }
        Vector2<float> s;
        if (size.x > size.y)
            s = Vector2<float>(size.x/2, 0);
        else
            s = Vector2<float>(0, size.y/2);
        int area = fillCircle(bitmap, colour, c, r2, tl, size - s) +
            fillCircle(bitmap, colour, c, r2, tl + s, size - s);
        if (size.x == 1 && size.y == 1)
            plot(&bitmap[Vector2Cast<int>(tl)], colour, area);
        return area;
    }
};

#endif // INCLUDED_KNOB_H
