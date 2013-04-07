#include "alfe/main.h"

#ifndef INCLUDED_USER_H
#define INCLUDED_USER_H

// TODO: Xlib port

#include <windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>
#include <vector>

class WindowsWindow;

template<class T> class WindowsTemplate : Uncopyable
{
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
        wc.lpszClassName = caption();
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

    LPCWSTR caption() const { return L"ALFE Window"; }

private:
    static LRESULT CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam,
        LPARAM lParam)
    {
        // Avoid reentering and causing a cascade of alerts if a assert fails.
        if (alerting)
            return DefWindowProc(hWnd, uMsg, wParam, lParam);

        BEGIN_CHECKED {
            WindowsWindow* window = 0;
            switch (uMsg) {
                case WM_NCCREATE:
                    {
                        LPCREATESTRUCT lpcs =
                            reinterpret_cast<LPCREATESTRUCT>(lParam);
                        window = reinterpret_cast<WindowsWindow *>(
                            lpcs->lpCreateParams);
                        window->_hWnd = hWnd;
                        setContext(hWnd, window);
                    }
                    break;
                case WM_NCDESTROY:
                    window = getContext(hWnd);
                    window->destroy();
                    window->remove();
                    delete window;
                    window = 0;
                    setContext(hWnd, 0);
                    break;
                case WM_COMMAND:
                    switch (HIWORD(wParam)) {
                        case BN_CLICKED:
                            dynamic_cast<Button*>(getContext(reinterpret_cast<HWND>(lParam)))
                                ->clicked();
                            break;
                    }
                    break;
                default:
                    window = getContext(hWnd);
                    break;
            }
            if (window != 0)
                return window->handleMessage(uMsg, wParam, lParam);
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

    static WindowsWindow* getContext(HWND hWnd)
    {
        SetLastError(0);
        LONG_PTR r = GetWindowLongPtr(hWnd, GWLP_USERDATA);
        IF_ZERO_CHECK_THROW_LAST_ERROR(r);
        return reinterpret_cast<WindowsWindow *>(r);
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

    friend class WindowsWindow;
};

typedef WindowsTemplate<void> Windows;

bool Windows::_failed = false;
Exception Windows::_exception;

template<class T> class PaintHandleTemplate;
typedef PaintHandleTemplate<void> PaintHandle;

class Window : public LinkedListMember<Window>
{
public:
    Window() : _added(false), _parent(0) { }
    void outerCreate()
    {
        doCreate();
        Window* window = _container.getNext();
        while (window != 0) {
            window->setWindows(_windows);
            window->setParent(this);
            window->outerCreate();
            window = _container.getNext(window);
        }
    }
    void setWindows(Windows* windows) { _windows = windows; }
    void add(Window* window) { _container.add(window); window->_added = true; }
    void remove()
    {
        if (_added) {
            LinkedListMember::remove(); 
            _added = false;
        }
        if (_parent != 0)
            _parent->removed();
    }
    virtual void removed() { }
    ~Window() { remove(); }
    void setParent(Window* window) { _parent = window; }
    virtual void doCreate() { create(); }
    virtual void create() { }
    virtual void doPaint(PaintHandle* paintHandle)
    {
        paint(paintHandle);
        Window* window = _container.getNext();
        while (window != 0) {
            window->doPaint(paintHandle);
            window = _container.getNext(window);
        }
    }
    virtual void paint(PaintHandle* paintHandle) { }
    Vector getSize() const { return _size; }
    virtual void resize() { }
    virtual void doneResize() { }

protected:
    Windows* _windows;
    LinkedList<Window> _container;
    Window* _parent;
    Vector _size;
private:
    bool _added;
};


class Menu
{
public:
    Menu(WORD resourceId)
    {
        HMODULE moduleHandle = GetModuleHandle(NULL);
        IF_NULL_THROW(moduleHandle);
        _menu = LoadMenu(moduleHandle, MAKEINTRESOURCE(resourceId));
        IF_NULL_THROW(_menu);
    }
    ~Menu() { DestroyMenu(_menu); }
    HMENU hMenu() { return _menu; }
private:
    HMENU _menu;
};


class DeviceContext : Uncopyable
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


template<class T> class PaintHandleTemplate : public DeviceContext
{
public:
    PaintHandleTemplate(const WindowsWindow* window) : _window(window)
    {
        IF_NULL_THROW(BeginPaint(_window->hWnd(), &_ps));
        _hdc = _ps.hdc;
    }
    ~PaintHandleTemplate() { EndPaint(_window->hWnd(), &_ps); }
    HDC hDC() const { return _ps.hdc; }
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
    const WindowsWindow* _window;
    PAINTSTRUCT _ps;
};


class WindowsWindow : public Window
{
    friend class WindowsTemplate<void>;
public:
    WindowsWindow() : _hdc(NULL), _hWnd(NULL), _resizing(false) { }

    virtual void doCreate()
    {
        create();
        reset();
        Vector size = initialSize();
        Vector position = initialPosition();

        WindowsWindow* parent = dynamic_cast<WindowsWindow*>(_parent);
        HWND hWndParent = NULL;
        if (parent != 0)
            hWndParent = parent->hWnd();

        _hWnd = CreateWindowEx(
            initialExtendedStyle(),   // dwExStyle
            className(),              // lpClassName
            initialCaption(),         // lpWindowName
            initialStyle(),           // dwStyle
            position.x,               // x
            position.y,               // y
            size.x,                   // nWidth
            size.y,                   // nHeight
            hWndParent,               // hWndParent
            initialMenu(),            // hMenu
            _windows->instance(),     // hInstance
            this);                    // lpParam
        IF_NULL_THROW(_hWnd);
        _hdc = GetDC(_hWnd);
        IF_NULL_THROW(_hdc);
        Windows::setContext(_hWnd, this);
        RECT rect;
        IF_ZERO_THROW(GetClientRect(_hWnd, &rect));
        _size = Vector(rect.right, rect.bottom);
    }

    ~WindowsWindow() { reset(); }

    void show(int nShowCmd) { ShowWindow(_hWnd, nShowCmd); }

    void invalidate() { IF_ZERO_THROW(InvalidateRect(_hWnd, NULL, FALSE)); }

    void outerResize(Vector size)
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

    void setText(LPCTSTR text) { IF_ZERO_THROW(SetWindowText(_hWnd, text)); }

    HWND hWnd() const { return _hWnd; }

private:
    void destroy()
    { 
        doDestroy();
        if (_hdc != NULL) {
            ReleaseDC(_hWnd, _hdc);
            _hdc = NULL;
        }
        _hWnd = NULL;
    }

    void reset()
    {
        if (_hdc != NULL) {
            ReleaseDC(_hWnd, _hdc);
            _hdc = NULL;
        }
        if (_hWnd != NULL) {
            DestroyWindow(_hWnd);
            _hWnd = NULL;
        }
    }

    void setHwnd(HWND hWnd) { _hWnd = hWnd; }

    virtual void doDestroy() { }

protected:
    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg) {
            case WM_PAINT:
                {
                    PaintHandle paintHandle(this);
                    if (!paintHandle.zeroArea())
                        doPaint(&paintHandle);
                }
                return 0;
            case WM_SIZE:
                {
                    _size = VectorFromLParam(lParam);
                    if (!_size.zeroArea()) {
                        resize();
                        invalidate();
                        _resizing = true;
                    }
                }
                break;
            case WM_EXITSIZEMOVE:
                if (_resizing)
                    doneResize();
                _resizing = false;
                break;
        }
        return DefWindowProc(_hWnd, uMsg, wParam, lParam);
    }

    static Vector VectorFromLParam(LPARAM lParam)
    {
        return Vector(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    }

    virtual LPCWSTR className() const { return _windows->className(); }
    virtual HMENU initialMenu() const { return NULL; }
    virtual Vector initialSize() const
    {
        return Vector(CW_USEDEFAULT, CW_USEDEFAULT);
    }
    virtual Vector initialPosition() const
    {
        return Vector(CW_USEDEFAULT, CW_USEDEFAULT);
    }
    virtual LPCWSTR initialCaption() const { return _windows->caption(); }
    virtual DWORD initialStyle() const { return WS_OVERLAPPEDWINDOW; }
    virtual DWORD initialExtendedStyle() const { return 0; }

    HWND _hWnd;
private:
    HDC _hdc;
    bool _resizing;
};


class Button : public WindowsWindow
{
public:
    virtual void clicked() const { }
protected:
    virtual LPCWSTR className() const { return WC_BUTTON; }
    virtual LPCWSTR initialCaption() { return _caption; }
    virtual Vector initialSize() const { return _size; }
    virtual Vector initialPosition() const { return _position; }
    virtual DWORD initialStyle() const
    { 
        return BS_PUSHBUTTON | BS_TEXT | WS_CHILD | WS_VISIBLE;
    }
    virtual HMENU initialMenu() const
    {
        return reinterpret_cast<HMENU>(_childWindowIdentifier);
    }
private:
    LPCWSTR _caption;
    Vector _position;
    Vector _size;
    int _childWindowIdentifier;
};


//template<class Base> class RootWindow : public Base
//{
//public:
//    class Params
//    {
//        friend class RootWindow;
//    public:
//        Params(typename Base::Params bp) : _bp(bp) { }
//    private:
//        typename Base::Params _bp;
//    };
//
//    void create(Params p) { Base::create(p._bp); }
//
//protected:
//    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
//    {
//        switch (uMsg) {
//            case WM_KEYDOWN:
//                if (wParam == VK_ESCAPE)
//                    destroy();
//        }
//
//        return Base::handleMessage(uMsg, wParam, lParam);
//    }
//
//    void destroy()
//    {
//        // Death of the root window ends the thread
//        PostQuitMessage(0);
//    }
//};


class WindowDeviceContext : public DeviceContext
{
public:
    WindowDeviceContext(const WindowsWindow* window) : _window(window)
    {
        _hdc = GetDC(window->hWnd());
        IF_NULL_THROW(_hdc);
    }
    ~WindowDeviceContext() { ReleaseDC(_window->hWnd(), _hdc); }
private:
    const WindowsWindow* _window;
};


template<class ImageType> class ImageWindow : public Window
{
public:
    ImageWindow() : _resizing(false) { }

    // doPaint is called only when the area is non-zero. Subclasses can get
    // zero-area WM_PAINT notifications by handling WM_PAINT in their
    // handleMessage() overrides.
    virtual void doPaint(PaintHandle* paint)
    {
        _image->paint(*paint);
    }
    virtual void destroy() { _image->destroy(); Base::destroy(); }

protected:
    //virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    //{
    //    switch (uMsg) {
    //        case WM_PAINT:
    //            {
    //                PaintHandle paint(*this);
    //                if (!paint.zeroArea())
    //                    doPaint(&paint);
    //            }
    //            return 0;
    //        case WM_SIZE:
    //            {
    //                Vector size = VectorFromLParam(lParam);
    //                if (!size.zeroArea()) {
    //                    _image->resize(size);
    //                    invalidate();
    //                    _resizing = true;
    //                }
    //            }
    //            break;
    //        case WM_EXITSIZEMOVE:
    //            if (_resizing)
    //                _image->doneResize();
    //            _resizing = false;
    //            break;
    //    }
    //    return Base::handleMessage(uMsg, wParam, lParam);
    //}

    ImageType* _image;
private:
    bool _resizing;
};


//class Image : public Bitmap<DWORD>
//{
//public:
//    Image() { }
//    Image(Vector size) : Bitmap(size)
//    {
//        if (size.x == 0 || size.y == 0)
//            return;
//
//        ZeroMemory(&_bmi, sizeof(BITMAPINFO));
//        _bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
//        _bmi.bmiHeader.biWidth = size.x;
//        _bmi.bmiHeader.biHeight = -size.y;
//        _bmi.bmiHeader.biPlanes = 1;
//        _bmi.bmiHeader.biBitCount = 32;
//        _bmi.bmiHeader.biCompression = BI_RGB;
//        _bmi.bmiHeader.biSizeImage = 0;
//        _bmi.bmiHeader.biXPelsPerMeter = 0;
//        _bmi.bmiHeader.biYPelsPerMeter = 0;
//        _bmi.bmiHeader.biClrUsed = 0;
//        _bmi.bmiHeader.biClrImportant = 0;
//
//        draw();
//    }
//
//    void resize(Vector newSize)
//    {
//        if (newSize != size()) {
//            Image image(newSize);
//            swap(image, *this);
//            doResize();
//            draw();
//        }
//    }
//
//    void paint(const PaintHandle& paint)
//    {
//        if (!valid())
//            return;
//        Vector topLeft = paint.topLeft();
//        Vector bottomRight = paint.bottomRight();
//        Vector paintSize = bottomRight - topLeft;
//        Vector s = size();
//        IF_ZERO_THROW(SetDIBitsToDevice(
//            paint,
//            topLeft.x,
//            topLeft.y,
//            paintSize.x,
//            paintSize.y,
//            topLeft.x,
//            (s - bottomRight).y,
//            0,
//            s.y,
//            data(),
//            &_bmi,
//            DIB_RGB_COLORS));
//    }
//
//    virtual void draw() { }
//
//    virtual void doneResize() { }
//
//    virtual void destroy() { }
//private:
//    BITMAPINFO _bmi;
//
//protected:
//    virtual void doResize() { }
//};
//
//
//template<class Base> class AnimatedWindow : public Base
//{
//public:
//    class Params
//    {
//        friend class AnimatedWindow;
//    public:
//        Params(typename Base::Params bp, float rate = 20.0f)
//          : _bp(bp),
//            _rate(rate)
//        { }
//    private:
//        typename Base::Params _bp;
//        float _rate;
//    };
//
//    AnimatedWindow() : _timerExpired(true), _delta(0), _lastTickCount(0) { }
//
//    void create(Params p)
//    {
//        _period = 1000.0f/p._rate;
//        Base::create(p._bp);
//    }
//
//protected:
//    virtual void doPaint(PaintHandle* paint)
//    {
//        if (_timerExpired) {
//            DWORD tickCount = GetTickCount();
//            _delta += static_cast<float>(tickCount - _lastTickCount);
//            _lastTickCount = tickCount;
//            int ms = static_cast<int>(-_delta);
//            _delta -= _period;
//            if (ms < -2.0f*_period) {
//                // Rather than trying to catch up by generating lots of
//                // events, we'll reset the clock if we're running too
//                // far behind.
//                _delta = 0;
//                ms = 0;
//            }
//            if (ms > 2.0f*max(
//                _period, static_cast<float>(USER_TIMER_MINIMUM))) {
//                // Chances are we're starting up and the tick count
//                // wrapped.
//                _delta = 0;
//                ms = static_cast<int>(_period) + 1;
//            }
//            if (ms < 0)
//                ms = 0;
//            if (ms >= USER_TIMER_MINIMUM) {
//                _timer =
//                    SetTimer(_hWnd, 1, static_cast<UINT>(ms), NULL);
//                IF_ZERO_THROW(_timer);
//            }
//            else
//                PostMessage(_hWnd, WM_TIMER, 1, 0);
//            _timerExpired = false;
//        }
//        Base::doPaint(paint);
//    }
//
//    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
//    {
//        switch (uMsg) {
//            case WM_TIMER:
//                if (wParam == 1) {
//                    invalidate();
//                    KillTimer(_hWnd, _timer);
//                    _timerExpired = true;
//                }
//                break;
//        }
//        return Base::handleMessage(uMsg, wParam, lParam);
//    }
//
//private:
//    UINT_PTR _timer;
//    bool _timerExpired;
//    float _period;
//    float _delta;
//    DWORD _lastTickCount;
//};


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
