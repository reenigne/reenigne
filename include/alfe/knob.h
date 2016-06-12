#include "alfe/main.h"

#ifndef INCLUDED_KNOB_H
#define INCLUDED_KNOB_H

const COLORREF chromaKey = RGB(0xff, 0x0, 0xff);

template<class T> class KnobSliderT;
typedef KnobSliderT<void> KnobSlider;

template<class T> class KnobSlidersT;
typedef KnobSlidersT<void> KnobSliders;

template<class T> class KnobSlidersT : public WindowsWindow
{
public:
    KnobSlidersT()
      : _delta(0, -1), _hdcScreen(NULL),
        _hdcSrc(CreateCompatibleDC(_hdcScreen)), _renderTask(this),
        _useChromaKey(false), _sliding(false)
    {
        setStyle(WS_POPUP);
        setExtendedStyle(WS_EX_LAYERED);
        _hbmBackBuffer =
            GDIObject(CreateCompatibleBitmap(_hdcScreen, 100, 100));
        _hbmOld = SelectedObject(&_hdcSrc, _hbmBackBuffer);
    }
protected:
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
        if (uMsg == WM_PAINT) {
            Lock lock(&_mutex);
            if (_useChromaKey) {
                PaintHandle p(this);
                setDIBits(p, p.topLeft(), p.bottomRight(), _s);
                return 0;
            }
        }
        return WindowsWindow::handleMessage(uMsg, wParam, lParam);
    }
private:
    bool knobEvent(KnobSlider* slider, Vector drag, bool buttonDown)
    {
        _slider = slider;
        if (!slider->_enabled)
            buttonDown = false;
        if (buttonDown) {
            if (!_sliding) {
                _dragStart = drag;
                _positionStart = slider->position();
                show(SW_SHOW);
                update(drag);
            }
            else
                update(drag);
        }
        if (!buttonDown && _sliding)
            show(SW_HIDE);
        _sliding = buttonDown;
        return buttonDown;  // Capture when button is down
    }
    void drawKnob(KnobSlider* slider)
    {
        _slider = slider;
        _renderTask.restart();
        _renderTask.join();
    }
    void update(Vector drag)
    {
        {
            Lock lock(&_mutex);
            int length = _slider->_popupLength;
            Vector2<double> delta = Vector2Cast<double>(drag - _dragStart);
            double distance = sqrt(delta.modulus2());
            double position;
            if (distance < 40) {
                position = _positionStart + dot(delta, _delta)/length;
                _a = _delta*length;
            }
            else {
                _delta = delta/sqrt(delta.modulus2());
                if (delta.x < delta.y) {
                    distance = -distance;
                    _delta = -_delta;
                }
                position = _positionStart + distance/length;
                _a = delta/(position - _positionStart);
            }
            _slider->setPosition(clamp(0.0, position, 1.0));
        }

        _renderTask.restart();
    }
    void render()
    {
        Vector2<float> corners[4];
        Vector2<double> low;
        Vector2<double> high;
        Vector topLeft;
        int length;
        double position;
        bool sliding;
        {
            Lock lock(&_mutex);
            position = _slider->position();
            sliding = _sliding;
            if (sliding) {
                length = _slider->_popupLength;

                low = Vector2Cast<double>(_dragStart) - _a*_positionStart;
                high = _a + low;

                double endPadding = _slider->outerSize().y/4;
                Vector2<double> x = (high - low)*endPadding/length;
                Vector2<double> y = 2.0*Vector2<double>(x.y, -x.x);
                corners[0] = Vector2Cast<float>(low - x - y);
                corners[1] = Vector2Cast<float>(low - x + y);
                corners[2] = Vector2Cast<float>(high + x - y);
                corners[3] = Vector2Cast<float>(high + x + y);

                topLeft = Vector2Cast<int>(corners[0]);
                Vector bottomRight = topLeft + Vector(1, 1);
                for (int i = 1; i < 4; ++i) {
                    Vector c = Vector2Cast<int>(corners[i]);
                    topLeft.x = min(topLeft.x, c.x);
                    topLeft.y = min(topLeft.y, c.y);
                    bottomRight.x = max(bottomRight.x, c.x + 1);
                    bottomRight.y = max(bottomRight.y, c.y + 1);
                }

                _s = bottomRight - topLeft;
                if (_s.x > _bitmap.size().x || _s.y > _bitmap.size().y) {
                    _bitmap = Bitmap<DWORD>(Vector(max(_s.x, _bitmap.size().x),
                        max(_s.y, _bitmap.size().y)));
                    _hbmOld = SelectedObject();
                    _hbmBackBuffer = GDIObject(CreateCompatibleBitmap(
                        _hdcScreen, _bitmap.size().x, _bitmap.size().y));
                    _hbmOld = SelectedObject(&_hdcSrc, _hbmBackBuffer);
                }
            }
        }

        KnobSlider::KnobWindow* knob = &_slider->_knob;
        Vector ks = knob->innerSize();
        Vector2<double> c = Vector2Cast<double>(ks)/2.0;
        Bitmap<DWORD> b = knob->bitmap();
        b.fill(_slider->_lightGrey);
        fillCircle(b,
            _slider->_enabled ? _slider->_darkGrey : _slider->_disabledGrey,
            Vector2Cast<float>(c), static_cast<float>(ks.x/2.0));
        Rotor2<double> r(-clamp(0.0, position, 1.0)*3/4);
        Vector2<double> o = Vector2<double>(-1, 1)*r*ks.x/sqrt(8) + c;
        Vector2<double> w = Vector2<double>(1, 1)*r;
        Vector2<float> points[4];
        points[0] = Vector2Cast<float>(c - w);
        points[1] = Vector2Cast<float>(c + w);
        points[2] = Vector2Cast<float>(o - w);
        points[3] = Vector2Cast<float>(o + w);
        fillParallelogram(b, &points[0],
            _slider->_enabled ? _slider->_black : _slider->_darkGrey);
        knob->invalidate();

        if (!sliding)
            return;

        for (int i = 0; i < 4; ++i)
            corners[i] -= Vector2Cast<float>(topLeft);
        low -= topLeft;
        high -= topLeft;
        // Transparent
        if (_useChromaKey)
            _bitmap.fill(0xff000000 | chromaKey);
        else
            _bitmap.fill(0);

        // Background
        fillParallelogram(_bitmap, &corners[0], _slider->_lightGrey,
            !_useChromaKey);

        // Track
        Vector2<double> x = (high - low)*2/length;
        Vector2<double> y = Vector2<double>(x.y, -x.x);
        corners[0] = Vector2Cast<float>(low - y);
        corners[1] = Vector2Cast<float>(low + y);
        corners[2] = Vector2Cast<float>(high - y);
        corners[3] = Vector2Cast<float>(high + y);
        fillParallelogram(_bitmap, &corners[0], _slider->_darkGrey);

        // Handle
        double endPadding = _slider->outerSize().y/4;
        x = (high - low)*endPadding/(length*2);
        y = 2.0*Vector2<double>(x.y, -x.x);
        Vector2<double> p = low + position*(high - low);
        corners[0] = Vector2Cast<float>(p - x - y);
        corners[1] = Vector2Cast<float>(p - x + y);
        corners[2] = Vector2Cast<float>(p + x - y);
        corners[3] = Vector2Cast<float>(p + x + y);
        fillParallelogram(_bitmap, &corners[0], _slider->_black);

        setInnerSize(_s);
        if (_useChromaKey) {
            setTopLeft(topLeft);
            invalidate();
            return;
        }

        setDIBits(_hdcSrc, Vector(0, 0), _s, _s);
        POINT ptSrc;
        ptSrc.x = 0;
        ptSrc.y = 0;
        SIZE size;
        size.cx = _s.x;
        size.cy = _s.y;
        POINT ptDst;
        ptDst.x = topLeft.x;
        ptDst.y = topLeft.y;
        BLENDFUNCTION blend;
        blend.BlendOp = AC_SRC_OVER;
        blend.BlendFlags = 0;
        blend.SourceConstantAlpha = 255;
        blend.AlphaFormat = AC_SRC_ALPHA;
        BOOL result = UpdateLayeredWindow(_hWnd, NULL, &ptDst, &size, _hdcSrc,
            &ptSrc, 0, &blend, ULW_ALPHA);
        if (result == 0) {
            {
                Lock lock(&_mutex);
                _useChromaKey = true;
            }
            IF_ZERO_THROW(SetLayeredWindowAttributes(_hWnd, chromaKey, 255,
                LWA_COLORKEY));
            _renderTask.restart();
        }
    }
    class RenderTask : public ThreadTask
    {
    public:
        RenderTask(KnobSliders* window) : _window(window) { }
        void run() { _window->render(); }
    private:
        KnobSliders* _window;
    };
    void setDIBits(HDC hdc, Vector ptl, Vector pbr, Vector s)
    {
        pbr = Vector(min(pbr.x, s.x), min(pbr.y, s.y));
        Vector ps = pbr - ptl;
        if (ps.x <= 0 || ps.y <= 0 || !_bitmap.valid())
            return;
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

    WindowDeviceContext _hdcScreen;
    OwnedDeviceContext _hdcSrc;
    GDIObject _hbmBackBuffer;
    SelectedObject _hbmOld;
    Bitmap<DWORD> _bitmap;
    BITMAPINFO _bmi;
    Vector2<double> _delta;
    RenderTask _renderTask;
    Vector _s;
    Mutex _mutex;
    bool _useChromaKey;
    Vector _dragStart;
    double _positionStart;
    KnobSlider* _slider;
    bool _sliding;
    Vector2<double> _a;

    template<class T> friend class KnobSliderT;

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

template<class T> class KnobSliderT : public ContainerWindow
{
public:
    KnobSliderT()
      : _config(0), _knobDiameter(24), _popupLength(301), _captionWidth(112),
        _logarithmic(false), _settingEdit(false), _enabled(true)
    {
        add(&_caption);
        add(&_knob);
        add(&_edit);
        _lightGrey = getSysColor(COLOR_BTNFACE, 0xc0c0c0);
        _darkGrey = getSysColor(COLOR_GRAYTEXT, 0x7f7f7f);
        _disabledGrey = (((_lightGrey & 0xff) + (_darkGrey & 0xff))/2) |
            (((_lightGrey & 0xff00) + (_darkGrey & 0xff00))/2) |
            (((_lightGrey & 0xff0000) + (_darkGrey & 0xff0000))/2);
        _black = getSysColor(COLOR_WINDOWTEXT, 0x000000);
    }
    void setSliders(KnobSliders* sliders) { _sliders = sliders; }
    void setText(String text) { _caption.setText(text); }
    void setCaptionWidth(int width) { _captionWidth = width; }
    void layout()
    {
        _knob.setInnerSize(Vector(_knobDiameter, _knobDiameter));
        int height = max(_caption.outerSize().y, max(_knobDiameter,
            _edit.outerSize().y));
        _caption.setTopLeft(Vector(0, (height - _caption.outerSize().y)/2));
        _knob.setTopLeft(Vector(
            max(_captionWidth, _caption.right() + _knobDiameter/2),
            (height - _knobDiameter)/2));
        _edit.setTopLeft(Vector(_knob.right() + _knobDiameter/2,
            (height - _edit.outerSize().y)/2));
        setInnerSize(Vector(_edit.right(), height));
    }
    void setTopLeft(Vector topLeft)
    {
        ContainerWindow::setTopLeft(topLeft);
        repositionChildren();
    }
    void create()
    {
        ContainerWindow::create();
        setValue(_value);
    }
    void setRange(double low, double high)
    {
        _min = low;
        _max = high;
    }
    virtual void valueSet(double value) { _valueSet(value); }
    void setValueSet(std::function<void(double)> valueSet)
    {
        _valueSet = valueSet;
    }
    void setValue(double value)
    {
        setValueInternal(value, true);
    }
    double getValue() const { return _value; }
    void setConfig(ConfigFile* config) { _config = config; }
    virtual double positionFromValue(double value)
    {
        return _logarithmic ? log(value) : value;
    }
    virtual double valueFromPosition(double position)
    {
        return _logarithmic ? exp(position) : position;
    }
    void setLogarithmic(bool logarithmic) { _logarithmic = logarithmic; }
    void enableWindow(bool enabled)
    {
        if (enabled != _enabled) {
            _enabled = enabled;
            _knob.draw();
        }
        ContainerWindow::enableWindow(enabled);
    }
    void changeValue(double amount)
    {
        setPosition(clamp(0.0, position() + amount, 1.0));
    }
private:
    void setValueInternal(double value, bool drawKnob)
    {
        double dp;
        if (_logarithmic)
            dp = log(value/_popupLength)/log(10);
        else
            dp = log((_max - _min)/_popupLength)/log(10);
        int dps = max(0, static_cast<int>(1 - dp));
        if (_popupLength < 0)
            dps = 1;
        _settingEdit = true;
        _edit.setText(format("%.*f", dps, value));
        _settingEdit = false;
        setValueFromEdit(value, drawKnob);
    }
    void setValueFromEdit(double value, bool drawKnob = true)
    {
        _value = value;
        valueSet(value);
        if (drawKnob)
            _knob.draw();
    }
    double position()
    {
        double m = positionFromValue(_min);
        return (positionFromValue(_value) - m)/(positionFromValue(_max) - m);
    }
    void setPosition(double p)
    {
        double m = positionFromValue(_min);
        setValueInternal(
            valueFromPosition(p*(positionFromValue(_max) - m) + m), false);
    }

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

    class KnobWindow : public BitmapWindow
    {
    public:
        void draw2()
        {
            if (_hWnd == NULL)
                return;
            host()->_sliders->drawKnob(host());
        }
        bool mouseInput(Vector position, int buttons, int wheel)
        {
            if (_hWnd == 0)
                return false;
            bool lButton = (buttons & MK_LBUTTON) != 0;
            POINT point;
            point.x = position.x;
            point.y = position.y;
            IF_ZERO_THROW(ClientToScreen(_hWnd, &point));
            return host()->_sliders->knobEvent(host(),
                Vector(point.x, point.y), lButton);
        }
    private:
        KnobSlider* host() { return static_cast<KnobSlider*>(parent()); }
    };

    class EditControl : public EditWindow
    {
    public:
        EditControl()
        {
            setExtendedStyle(WS_EX_CLIENTEDGE);
            setStyle(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP);
        }
        void changed()
        {
            if (host()->_settingEdit)
                return;
            String t = getText();
            double v = host()->_config->evaluate<double>(t, host()->_value);
            host()->setValueFromEdit(v);
        }
        virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            if (uMsg == WM_KEYDOWN) {
                switch (wParam) {
                    case VK_UP:
                        host()->changeValue(0.01);
                        return 0;
                    case VK_DOWN:
                        host()->changeValue(-0.01);
                        return 0;
                    case VK_PRIOR:
                        host()->changeValue(0.1);
                        return 0;
                    case VK_NEXT:
                        host()->changeValue(-0.1);
                        return 0;
                }
            }
            return EditWindow::handleMessage(uMsg, wParam, lParam);
        }
    private:
        KnobSlider* host() { return static_cast<KnobSlider*>(parent()); }
    };

    TextWindow _caption;
    KnobWindow _knob;
    EditControl _edit;
    KnobSliders* _sliders;
    double _value;
    double _min;
    double _max;
    int _knobDiameter;
    int _popupLength;
    int _captionWidth;
    DWORD _disabledGrey;
    DWORD _lightGrey;
    DWORD _darkGrey;
    DWORD _black;
    ConfigFile* _config;
    bool _logarithmic;
    bool _settingEdit;
    std::function<void(double)> _valueSet;
    bool _enabled;

    friend class KnobWindow;
    template<class T> friend class KnobSlidersT;
    friend class EditWindow;
};

#endif // INCLUDED_KNOB_H
