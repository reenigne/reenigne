#ifndef INCLUDED_USER_H
#define INCLUDED_USER_H

// TODO: Xlib port

#include "alfe/swap.h"
#include "alfe/string.h"
#include "alfe/vectors.h"
#include "alfe/bitmap.h"
#include <windows.h>
#include <WindowsX.h>
#include <vector>

template<class T> class WindowTemplate;
typedef WindowTemplate<void> Window;

template<class T> class WindowsTemplate;
typedef WindowsTemplate<void> Windows;

template<class T> class WindowsTemplate : Uncopyable
{
    friend class WindowTemplate<T>;
public:
    WindowsTemplate() : _classAtom(0) { }

    void initialize(HINSTANCE hInst)
    {
        _hInst = hInst;
        WNDCLASS wc;
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = wndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hInst;
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = L"Unity Window";
        _classAtom = RegisterClass(&wc);
        IF_ZERO_THROW(_classAtom);
    }

    LPCWSTR className() const
    {
        return reinterpret_cast<LPCWSTR>(static_cast<UINT_PTR>(_classAtom));
    }

    HINSTANCE instance() const { return _hInst; }

    ~WindowsTemplate() { UnregisterClass(className(), _hInst); }

    static void check()
    {
        if (_failed)
            throw _exception;
    }

private:
    static LRESULT CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam,
        LPARAM lParam)
    {
        BEGIN_CHECKED {
            Window* window = 0;
            switch (uMsg) {
                case WM_NCCREATE:
                    {
                        LPCREATESTRUCT lpcs =
                            reinterpret_cast<LPCREATESTRUCT>(lParam);
                        window =
                            reinterpret_cast<Window *>(lpcs->lpCreateParams);
                        window->_hWnd = hWnd;
                        setContext(hWnd, window);
                    }
                    break;
                default:
                    SetLastError(0);
                    LONG_PTR r = GetWindowLongPtr(hWnd, GWLP_USERDATA);
                    IF_ZERO_CHECK_THROW_LAST_ERROR(r);
                    window = reinterpret_cast<Window *>(r);
                    break;
            }
            LRESULT result;
            if (window != 0)
                result = window->handleMessage(uMsg, wParam, lParam);
            if (uMsg == WM_NCDESTROY)
                setContext(hWnd, 0);
            if (window != 0)
                return result;
        } END_CHECKED(Exception& e) {
            PostQuitMessage(0);
            _exception = e;
            _failed = true;
        }

        // The first message sent to a window is WM_GETMINMAXINFO rather than
        // WM_NCCREATE. Since we don't have a context at that point, delegate
        // this message to DefWindowProc.
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    static void setContext(HWND hWnd, void* p)
    {
        SetLastError(0);
        IF_ZERO_CHECK_THROW_LAST_ERROR(SetWindowLongPtr(hWnd, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(p)));
    }

    HINSTANCE _hInst;
    ATOM _classAtom;

    static Exception _exception;
    static bool _failed;
};

bool Windows::_failed = false;
Exception Windows::_exception;


template<class T> class WindowTemplate
{
    friend class WindowsTemplate<T>;
public:
    class Params
    {
        friend class WindowTemplate<T>;
    public:
        Params(Windows* windows, LPCWSTR pszName)
          : _windows(windows),
            _pszName(pszName),
            _nSize(CW_USEDEFAULT, CW_USEDEFAULT)
            //,_menu(0)
        { }
        Params(Windows* windows, LPCWSTR pszName, Vector nSize)
          : _windows(windows),
            _pszName(pszName),
            _nSize(nSize)
            //,_menu(0)
        { }
        //Params(Windows* windows, LPCWSTR pszName, Vector nSize,
        //    Menu* menu = 0)
        //  : _windows(windows),
        //    _pszName(pszName),
        //    _nSize(nSize),
        //    _menu(menu) { }
    private:
        Windows* _windows;
        LPCWSTR _pszName;
        Vector _nSize;
        //Menu* _menu;
    };

    WindowTemplate(Params p)
    {
        HMENU hMenu = NULL;
        //if (p._menu != 0)
        //    hMenu = *p._menu;
        IF_NULL_THROW(CreateWindowEx(
            0,                        // dwExStyle
            p._windows->className(),  // lpClassName
            p._pszName,               // lpWindowName
            WS_OVERLAPPEDWINDOW,      // dwStyle
            CW_USEDEFAULT,            // x
            CW_USEDEFAULT,            // y
            p._nSize.x,               // nWidth
            p._nSize.y,               // nHeight
            NULL,                     // hWndParent
            hMenu,                    // hMenu
            p._windows->instance(),   // hInstance
            this));                   // lpParam
        _hdc = GetDC(_hWnd);
    }

    operator HDC() const { return _hdc; }

    ~WindowTemplate()
    {
        ReleaseDC(_hWnd, _hdc);
        DestroyWindow(_hWnd);
    }

    void show(int nShowCmd) { ShowWindow(_hWnd, nShowCmd); }

    void resize(Vector size)
    {
        IF_ZERO_THROW(SetWindowPos(
            _hWnd,                                // hWnd
            NULL,                                 // hWndInsertAfter
            0,                                    // X
            0,                                    // Y
            size.x,                               // cx
            size.y,                               // cy
            SWP_NOZORDER | SWP_NOMOVE |
            SWP_NOACTIVATE | SWP_NOREPOSITION));  // uFlags
    }

    Vector getSize()
    {
        RECT rect;
        IF_ZERO_THROW(GetClientRect(_hWnd, &rect));
        return Vector(rect.right, rect.bottom);
    }

    void setText(LPCTSTR text) { IF_ZERO_THROW(SetWindowText(_hWnd, text)); }

    operator HWND() const { return _hWnd; }

    virtual void invalidate() { }
private:
    void setHwnd(HWND hWnd) { _hWnd = hWnd; }

    virtual void destroy() { }

protected:
    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg) {
            case WM_NCDESTROY:
                destroy();
                Windows::setContext(_hWnd, NULL);
                break;
        }
        return DefWindowProc(_hWnd, uMsg, wParam, lParam);
    }

    static Vector VectorFromLParam(LPARAM lParam)
    {
        return Vector(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    }

    HWND _hWnd;
private:
    HDC _hdc;
};


template<class Base> class RootWindow : public Base
{
public:
    class Params
    {
        friend class RootWindow;
    public:
        Params(typename Base::Params bp) : _bp(bp) { }
    private:
        typename Base::Params _bp;
    };

    RootWindow(Params p) : Base(p._bp) { }

protected:
    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg) {
            case WM_KEYDOWN:
                if (wParam == VK_ESCAPE)
                    destroy();
        }

        return Base::handleMessage(uMsg, wParam, lParam);
    }

    void destroy()
    {
        // Death of the root window ends the thread
        PostQuitMessage(0);
    }
};


class DeviceContext
{
public:
    void NoFailSelectObject(HGDIOBJ hObject) { ::SelectObject(_hdc, hObject); }
    void SelectObject(HGDIOBJ hObject)
    {
        IF_NULL_THROW(::SelectObject(_hdc, hObject));
    }
    void SetROP2(int fnDrawMode)
    {
        IF_ZERO_THROW(::SetROP2(_hdc, fnDrawMode));
    }
    void Polyline(CONST POINT* lppt, int cPoints)
    {
        IF_ZERO_THROW(::Polyline(_hdc, lppt, cPoints));
    }
    operator HDC() const { return _hdc; }
protected:
    HDC _hdc;
};


class WindowDeviceContext : public DeviceContext
{
public:
    WindowDeviceContext(const Window& window)
      : _window(&window)
    {
        _hdc = GetDC(window);
        IF_NULL_THROW(_hdc);
    }
    ~WindowDeviceContext() { ReleaseDC(*_window, _hdc); }
private:
    const Window* _window;
};


class PaintHandle : public DeviceContext
{
public:
    PaintHandle(const Window& window) : _window(window)
    {
        IF_NULL_THROW(BeginPaint(_window, &_ps));
        _hdc = _ps.hdc;
    }
    ~PaintHandle()
    {
        EndPaint(_window, &_ps);
    }
    operator HDC() const { return _ps.hdc; }
    Vector topLeft() const
    {
        return Vector(_ps.rcPaint.left, _ps.rcPaint.top);
    }
    Vector bottomRight() const
    {
        return Vector(_ps.rcPaint.right, _ps.rcPaint.bottom);
    }
    bool zeroArea() const { return (topLeft()-bottomRight()).zeroArea(); }
private:
    const Window& _window;
    PAINTSTRUCT _ps;
};


template<class Base, class ImageType> class ImageWindow : public Base
{
public:
    class Params
    {
        friend class ImageWindow;
    public:
        Params(typename Base::Params bp, ImageType* image)
          : _bp(bp),
            _image(image) { }
    private:
        typename Base::Params _bp;
        ImageType* _image;
    };

    ImageWindow(Params p) : Base(p._bp), _image(p._image), _resizing(false) { }

    void invalidate() { IF_ZERO_THROW(InvalidateRect(_hWnd, NULL, FALSE)); }

    // doPaint is called only when the area is non-zero. Subclasses can get
    // zero-area WM_PAINT notifications by handling WM_PAINT in their
    // handleMessage() overrides.
    virtual void doPaint(PaintHandle* paint)
    {
        _image->paint(*paint);
    }
    virtual void destroy() { _image->destroy(); Base::destroy(); }

protected:
    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg) {
            case WM_PAINT:
                {
                    PaintHandle paint(*this);
                    if (!paint.zeroArea())
                        doPaint(&paint);
                }
                return 0;
            case WM_SIZE:
                {
                    Vector size = VectorFromLParam(lParam);
                    if (!size.zeroArea()) {
                        _image->resize(size);
                        invalidate();
                        _resizing = true;
                    }
                }
                break;
            case WM_EXITSIZEMOVE:
                if (_resizing)
                    _image->doneResize();
                _resizing = false;
                break;
        }
        return Base::handleMessage(uMsg, wParam, lParam);
    }

    ImageType* _image;
private:
    bool _resizing;
};


class Image : public Bitmap<DWord>
{
public:
    Image() { }
    Image(Vector size) : Bitmap(size)
    {
        if (size.x == 0 || size.y == 0)
            return;

        ZeroMemory(&_bmi, sizeof(BITMAPINFO));
        _bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        _bmi.bmiHeader.biWidth = size.x;
        _bmi.bmiHeader.biHeight = -size.y;
        _bmi.bmiHeader.biPlanes = 1;
        _bmi.bmiHeader.biBitCount = 32;
        _bmi.bmiHeader.biCompression = BI_RGB;
        _bmi.bmiHeader.biSizeImage = 0;
        _bmi.bmiHeader.biXPelsPerMeter = 0;
        _bmi.bmiHeader.biYPelsPerMeter = 0;
        _bmi.bmiHeader.biClrUsed = 0;
        _bmi.bmiHeader.biClrImportant = 0;

        draw();
    }

    void resize(Vector newSize)
    {
        if (newSize != size()) {
            Image image(newSize);
            swap(image, *this);
            doResize();
            draw();
        }
    }

    void paint(const PaintHandle& paint)
    {
        if (!valid())
            return;
        Vector topLeft = paint.topLeft();
        Vector bottomRight = paint.bottomRight();
        Vector paintSize = bottomRight - topLeft;
        Vector s = size();
        IF_ZERO_THROW(SetDIBitsToDevice(
            paint,
            topLeft.x,
            topLeft.y,
            paintSize.x,
            paintSize.y,
            topLeft.x,
            (s - bottomRight).y,
            0,
            s.y,
            data(),
            &_bmi,
            DIB_RGB_COLORS));
    }

    virtual void draw() { }

    virtual void doneResize() { }
private:
    BITMAPINFO _bmi;

protected:
    virtual void doResize() { }
};


template<class Base> class AnimatedWindow : public Base
{
public:
    class Params
    {
        friend class AnimatedWindow;
    public:
        Params(typename Base::Params bp, float rate = 20.0f)
          : _bp(bp),
            _rate(rate)
        { }
    private:
        typename Base::Params _bp;
        float _rate;
    };

    AnimatedWindow(Params p)
      : Base(p._bp),
        _period(1000.0f/p._rate),
        _timerExpired(true),
        _delta(0),
        _lastTickCount(0)
    { }

protected:
    virtual void doPaint(PaintHandle* paint)
    {
        if (_timerExpired) {
            DWORD tickCount = GetTickCount();
            _delta += static_cast<float>(tickCount - _lastTickCount);
            _lastTickCount = tickCount;
            int ms = static_cast<int>(-_delta);
            _delta -= _period;
            if (ms < -2.0f*_period) {
                // Rather than trying to catch up by generating lots of
                // events, we'll reset the clock if we're running too
                // far behind.
                _delta = 0;
                ms = 0;
            }
            if (ms > 2.0f*max(
                _period, static_cast<float>(USER_TIMER_MINIMUM))) {
                // Chances are we're starting up and the tick count
                // wrapped.
                _delta = 0;
                ms = static_cast<int>(_period) + 1;
            }
            if (ms < 0)
                ms = 0;
            if (ms >= USER_TIMER_MINIMUM) {
                _timer =
                    SetTimer(_hWnd, 1, static_cast<UINT>(ms), NULL);
                IF_ZERO_THROW(_timer);
            }
            else
                PostMessage(_hWnd, WM_TIMER, 1, 0);
            _timerExpired = false;
        }
        Base::doPaint(paint);
    }

    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg) {
            case WM_TIMER:
                if (wParam == 1) {
                    invalidate();
                    KillTimer(_hWnd, _timer);
                    _timerExpired = true;
                }
                break;
        }
        return Base::handleMessage(uMsg, wParam, lParam);
    }

private:
    UINT_PTR _timer;
    bool _timerExpired;
    float _period;
    float _delta;
    DWORD _lastTickCount;
};


int pumpMessages()
{
    BOOL bRet;
    do {
        MSG msg;
        bRet = GetMessage(&msg, NULL, 0, 0);
        IF_MINUS_ONE_THROW(bRet);
        if (bRet != 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
            return static_cast<int>(msg.wParam);
    } while (bRet != 0);
    return 0;
}


class Pen
{
public:
    Pen(int fnPenStyle, int nWidth, COLORREF crColor)
    {
        m_hPen = CreatePen(fnPenStyle, nWidth, crColor);
        IF_NULL_THROW(m_hPen);
    }
    ~Pen() { DeleteObject(m_hPen); }
    operator HPEN() { return m_hPen; }
private:
    HPEN m_hPen;
};


class SelectedPen
{
public:
    SelectedPen(DeviceContext* dc, Pen* pen) : _dc(dc)
    {
        _dc->SelectObject(*pen);
    }
    ~SelectedPen()
    {
        _dc->NoFailSelectObject(GetStockObject(BLACK_PEN));
    }
private:
    DeviceContext* _dc;
};


#endif // INCLUDED_USER_H
