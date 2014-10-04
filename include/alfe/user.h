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
        NullTerminatedWideString c(caption());
        wc.lpszClassName = c;
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

    String caption() const { return "ALFE Window"; }

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
                    // delete window;
                    window = 0;
                    setContext(hWnd, 0);
                    break;
                case WM_COMMAND:
                    switch (HIWORD(wParam)) {
                        case BN_CLICKED:
                            dynamic_cast<Button*>(getContext(lParam))
                                ->clicked();
                            break;
                    }
                    break;
                case WM_HSCROLL:
                    switch (HIWORD(wParam)) {
                        case SB_THUMBPOSITION:
                        case SB_THUMBTRACK:
                            dynamic_cast<SliderWindow*>(getContext(lParam))
                                ->updateValue(LOWORD(wParam));
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

    static void setContext(HWND hWnd, WindowsWindow* p)
    {
        SetLastError(0);
        IF_ZERO_CHECK_THROW_LAST_ERROR(SetWindowLongPtr(hWnd, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(p)));
    }

    static WindowsWindow* getContext(LPARAM lParam)
    {
        return getContext(reinterpret_cast<HWND>(lParam));
    }

    static WindowsWindow* getContext(HWND hWnd)
    {
        SetLastError(0);
        LONG_PTR r = GetWindowLongPtr(hWnd, GWLP_USERDATA);
        IF_ZERO_CHECK_THROW_LAST_ERROR(r);
        return reinterpret_cast<WindowsWindow *>(r);
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

class ContainerWindow;

template<class T> class WindowTemplate;
typedef WindowTemplate<void> Window;

class Edge;

class EdgeExpression
{
public:
    EdgeExpression operator+(EdgeExpression a) { }
    EdgeExpression operator-(EdgeExpression a) { }
    EdgeExpression operator*(double a) { }
    EdgeExpression operator/(double a) { }
private:
    class Clause
    {
    public:
        Clause(Edge* edge, double coefficient)          : _edge(edge), _coefficient(coefficient) { }        Edge* _edge;
        double _coefficient;
    };
    List<Clause> _clauses;
    double _offset;

    void setToEdge(Edge* edge)
    {
        _clauses = List<Clause>();
        _clauses.add(Clause(edge, 1));
        _offset = 0;
    }

    friend class Edge;
};

class Edge : public EdgeExpression
{
public:
    Edge()
    {
        setToEdge(this);
    }
    void operator=(const EdgeExpression& expression)
    {
        EdgeExpression::operator=(expression);
    }
private:
    List<Edge> _dependents;
};

template<class T> class WindowTemplate
  : public LinkedListMember<WindowTemplate<T>>
{
public:
    WindowTemplate() : _parent(0), _topLeft(Vector(0, 0)) { }
    ~WindowTemplate() { remove(); }
    virtual void create() = 0;
    void setParent(ContainerWindow* window) { _parent = window; }
    ContainerWindow* parent() const { return _parent; }
    virtual void paint(PaintHandle* paintHandle) { }
    void setSize(Vector size) { _size = size; }
    void sizeSet(Vector size) { _size = size; }
    void setPosition(Vector topLeft) { _topLeft = topLeft; }
    Vector size() const { return _size; }
    Vector topLeft() const { return _topLeft; }
    virtual void resize() { }
    virtual void doneResize() { }
    virtual void keyboardCharacter(int character) { }
    virtual void draw(Bitmap<DWORD> bitmap) { }

    // mouseInput() should return true if the mouse should be captured
    virtual bool mouseInput(Vector position, int buttons) { return false; }
    virtual void releaseCapture() { }
    void remove()
    {
        if (_parent != 0) {
            _parent->childRemoved(this);
            LinkedListMember::remove();
        }
    }

    Edge top;
    Edge left;
    Edge right;
    Edge bottom;

    virtual void invalidate() { invalidateRectangle(Vector(0, 0), _size); }
    virtual void invalidateRectangle(Vector topLeft, Vector size)
    {
        _parent->invalidateRectangle(_topLeft + topLeft, size);
    }
private:
    ContainerWindow* _parent;
    Vector _size;
    Vector _topLeft;
};

class ContainerWindow : public Window
{
public:
    ContainerWindow() : _focus(0), _capture(0) { }
    void create()
    {
        Window* window = _container.getNext();
        while (window != 0) {
            window->setParent(this);
            window->create();
            window = _container.getNext(window);
        }
    }
    void remove()
    {
        Window* window = _container.getNext();
        while (window != 0) {
            window->remove();
            window = _container.getNext(window);
        }
        Window::remove();
    }
    void add(Window* window)
    {
        _container.add(window);
        window->setParent(this);
        if (_focus == 0)
            _focus = window;
    }
    virtual void childRemoved(Window* child)
    {
        if (_focus == child) {
            _focus = _container.getNext(child);
            if (_focus == 0) {
                // No more windows after child, try starting from the beginning
                // again.
                _focus = _container.getNext();
                if (_focus == child) {
                    // It's an only child, so there's nowhere left to put the
                    // focus.
                    _focus = 0;
                }
            }
        }
    }
    void paint(PaintHandle* paintHandle)
    {
        Window* window = _container.getNext();
        while (window != 0) {
            window->paint(paintHandle);
            window = _container.getNext(window);
        }
    }
    void draw(Bitmap<DWORD> bitmap)
    {
        Window* window = _container.getNext();
        while (window != 0) {
            window->draw(bitmap.subBitmap(window->topLeft(), window->size()));
            window = _container.getNext(window);
        }
    }
    void keyboardCharacter(int character)
    {
        if (_focus != 0)
            _focus->keyboardCharacter(character);
    }
    bool mouseInput(Vector position, int buttons)
    {
        // If the mouse is captured, send the input to the capturing window
        if (_capture != 0) {
            _capture->mouseInput(position - _capture->topLeft(), buttons);
            if (buttons == 0)
                releaseCapture();
            return false;
        }
        // Otherwise, send the input the window under the mouse
        Window* window = windowForPosition(position);
        if (window != 0) {
            bool capture =
                window->mouseInput(position - window->topLeft(), buttons);
            if ((buttons & ~_buttons) != 0) {
                // A button is newly pressed - put focus on this child window
                _focus = window;
            }
            _buttons = buttons;
            if (capture)
                _capture = window;
            return capture;
        }
        return false;
    }
    void releaseCapture() { _capture = 0; }

protected:
    LinkedList<Window> _container;
    Window* _focus;
    Window* _capture;
    int _buttons;
private:
    Window* windowForPosition(Vector p)
    {
        Window* window = _container.getNext();
        while (window != 0) {
            if ((p - window->topLeft()).inside(window->size()))
                return window;
            window = _container.getNext(window);
        }
        return 0;
    }
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


class WindowsWindow : public ContainerWindow
{
    friend class WindowsTemplate<void>;
public:
    WindowsWindow() : _hdc(NULL), _hWnd(NULL), _resizing(false)
    {
        sizeSet(Vector(CW_USEDEFAULT, CW_USEDEFAULT));
        setPosition(Vector(CW_USEDEFAULT, CW_USEDEFAULT));
    }

    void setWindows(Windows* windows)
    {
        _windows = windows;
        _text = _windows->caption();
        _className = _windows->className();
        _style = WS_OVERLAPPEDWINDOW;
        _extendedStyle = 0;
        _menu = NULL;
        Window* window = _container.getNext();
        while (window != 0) {
            WindowsWindow* w = dynamic_cast<WindowsWindow*>(window);
            if (w != 0)
                w->setWindows(windows);
            window = _container.getNext(window);
        }
    }

    virtual void create()
    {
        reset();
        Vector s = size();
        Vector position = topLeft();

        WindowsWindow* p = dynamic_cast<WindowsWindow*>(parent());
        HWND hWndParent = NULL;
        if (p != 0)
            hWndParent = p->hWnd();

        NullTerminatedWideString caption(_text);

        _hWnd = CreateWindowEx(
            _extendedStyle,
            _className,
            caption,
            _style,
            position.x,
            position.y,
            s.x,
            s.y,
            hWndParent,
            _menu,
            _windows->instance(),
            this);
        IF_NULL_THROW(_hWnd);
        _hdc = GetDC(_hWnd);
        IF_NULL_THROW(_hdc);
        Windows::setContext(_hWnd, this);
        RECT rect;
        IF_ZERO_THROW(GetClientRect(_hWnd, &rect));
        sizeSet(Vector(rect.right, rect.bottom));
        ContainerWindow::create();
    }

    ~WindowsWindow() { reset(); }

    void show(int nShowCmd) { ShowWindow(_hWnd, nShowCmd); }

    void invalidateRectangle(Vector topLeft, Vector size)
    {
        RECT rect;
        rect.left = topLeft.x;
        rect.top = topLeft.y;
        rect.right = topLeft.x + size.x;
        rect.bottom = topLeft.y + size.y;
        IF_ZERO_THROW(InvalidateRect(_hWnd, &rect, FALSE));
    }

    void setText(String text)
    {
        _text = text;
        if (_hWnd != NULL) {
            NullTerminatedWideString w(text);
            IF_ZERO_THROW(SetWindowText(_hWnd, w));
        }
    }

    void setSize(Vector size)
    {
        if (_hWnd != NULL) {
            IF_ZERO_THROW(SetWindowPos(
                _hWnd,                                // hWnd
                NULL,                                 // hWndInsertAfter
                0,                                    // X
                0,                                    // Y
                size.x,                               // cx
                size.y,                               // cy
                SWP_NOZORDER | SWP_NOMOVE |
                SWP_NOACTIVATE | SWP_NOREPOSITION));  // uFlags
            // sizeSet() will be called via WM_SIZE.
        }
        else
            sizeSet(size);
    }
    virtual void sizeSet(Vector size)
    { 
        ContainerWindow::sizeSet(size);
    }

    HWND hWnd() const { return _hWnd; }

private:
    void destroy()
    { 
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

protected:
    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg) {
            case WM_PAINT:
                {
                    PaintHandle paintHandle(this);
                    if (!paintHandle.zeroArea())
                        paint(&paintHandle);
                }
                return 0;
            case WM_SIZE:
                {
                    sizeSet(vectorFromLParam(lParam));
                    if (!size().zeroArea()) {
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
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEMOVE:
                if (mouseInput(vectorFromLParam(lParam), wParam))
                    SetCapture(_hWnd);
                break;
            case WM_CHAR:
                keyboardCharacter(wParam);
                break;
            case WM_KILLFOCUS:
                releaseCapture();
                break;
        }
        return DefWindowProc(_hWnd, uMsg, wParam, lParam);
    }
    void releaseCapture()
    {
        ReleaseCapture();
        ContainerWindow::releaseCapture();
    }
                                              
    static Vector vectorFromLParam(LPARAM lParam)
    {
        return Vector(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    }

    void setClassName(LPCWSTR className) { _className = className; }

    void setStyle(DWORD style) { _style = style; }

    HWND _hWnd;
private:
    HDC _hdc;
    bool _resizing;
    String _text;
    Windows* _windows;
    DWORD _style;
    DWORD _extendedStyle;
    LPCWSTR _className;
    HMENU _menu;
};


class RootWindow : public WindowsWindow
{
public:
    void childRemoved(Window* child)
    {
        if (_container.getNext(child) == 0 && _container.getNext() == child) {
            // Once there are no more child windows left, the thread must
            // end.
            PostQuitMessage(0);
        }
        WindowsWindow::childRemoved(child);
    }
};

class Button : public WindowsWindow
{
public:
    virtual void clicked() { }
    void setWindows(Windows* windows)
    {
        WindowsWindow::setWindows(windows);
        setClassName(WC_BUTTON);
        setStyle(BS_PUSHBUTTON | BS_TEXT | WS_CHILD | WS_VISIBLE);
    }
};

class TextWindow : public WindowsWindow
{
public:
    void setWindows(Windows* windows)
    {
        WindowsWindow::setWindows(windows);
        setClassName(WC_STATIC);
        setStyle(WS_CHILD | WS_VISIBLE);
    }
};

class SliderWindow : public WindowsWindow
{
public:
    virtual void valueSet(double value) { }
    void setWindows(Windows* windows)
    {
        WindowsWindow::setWindows(windows);
        setClassName(TRACKBAR_CLASS);
        setStyle(WS_CHILD | WS_VISIBLE);
        _min = 0;
        _pos = 0;
        _max = 1;
    }
    void create()
    {
        WindowsWindow::create();
        SendMessage(_hWnd, TBM_SETRANGE, static_cast<WPARAM>(TRUE),
            static_cast<LPARAM>(MAKELONG(0, 65535)));
        setValue(_pos);
    }
    void setValue(double pos)
    {
        _pos = pos;
        if (_hWnd != NULL) {
            int iPos = static_cast<int>((pos * 65535)/(_max - _min) + 0.5);
            SendMessage(_hWnd, TBM_SETPOS, static_cast<WPARAM>(TRUE),
                static_cast<LPARAM>(iPos));
        }
    }
    void setRange(double min, double max) { _min = min; _max = max; }
    void updateValue(int pos)
    {
        _pos = pos*(_max - _min) / 65535;
        valueSet(_pos);
    }
private:
    double _min;
    double _max;
    double _pos;
};

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


class BitmapWindow : public ContainerWindow
{
public:
    BitmapWindow() : _resizing(false) { }

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
        resize();
    }

    void resize()
    {
        Vector s = size();
        _bmi.bmiHeader.biWidth = s.x;
        _bmi.bmiHeader.biHeight = -s.y;
        _bitmap = Bitmap<DWORD>(s);
        draw();
    }

    // doPaint is called only when the area is non-zero. Subclasses can get
    // zero-area WM_PAINT notifications by handling WM_PAINT in their
    // handleMessage() overrides.
    virtual void paint(PaintHandle* paint)
    {
        if (!_bitmap.valid())
            return;
        Vector topLeft = paint->topLeft();
        Vector bottomRight = paint->bottomRight();
        Vector paintSize = bottomRight - topLeft;
        Vector s = size();
        IF_ZERO_THROW(SetDIBitsToDevice(
            paint->hDC(),
            topLeft.x,
            topLeft.y,
            paintSize.x,
            paintSize.y,
            topLeft.x,
            (s - bottomRight).y,
            0,
            s.y,
            _bitmap.data(),
            &_bmi,
            DIB_RGB_COLORS));
        ContainerWindow::paint(paint);
    }

    virtual void draw() = 0;

    //Vector size() const { return _bitmap.size(); }
    int stride() const { return _bitmap.stride(); }
    const Byte* data() const { return _bitmap.data(); }
    Byte* data() { return _bitmap.data(); }

protected:
    Bitmap<DWORD> _bitmap;

private:
    bool _resizing;
    BITMAPINFO _bmi;
};

class AnimationThread : public Thread
{
public:
    AnimationThread()
      : _timerExpired(true), _delta(0), _lastTickCount(0), _period(0.05f)
    { }
    void setWindow(Window* window) { _window = window; }
    void setRate(float rate) { _period = 1000.0f/rate; }
    void threadProc()
    {
        bool interrupted;
        do {
            _start.wait();
            _start.reset();
            _window->invalidate();

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
            if (ms > 2.0f*_period) {
                // Chances are we're starting up and the tick count
                // wrapped.
                _delta = 0;
                ms = static_cast<int>(_period);
            }
            if (ms < 0)
                ms = 0;
            interrupted = _interrupt.wait(ms);
        } while (!interrupted);
    }
    void onPaint() { _start.signal(); }
    void close()
    {
        _interrupt.signal();
        _start.signal();
    }
    ~AnimationThread() { close(); }
private:
    UINT_PTR _timer;
    bool _timerExpired;
    float _period;
    float _delta;
    DWORD _lastTickCount;
    Window* _window;
    Event _start;
    Event _interrupt;
};

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
