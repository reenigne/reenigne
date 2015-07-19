#include "alfe/main.h"

#ifndef INCLUDED_USER_H
#define INCLUDED_USER_H

// TODO: Xlib port

#include <windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>
#include <vector>

class WindowsWindow;

class Font : Uncopyable
{
public:
    Font()
    {
        NONCLIENTMETRICS ncm;
        ncm.cbSize = sizeof(NONCLIENTMETRICS) - sizeof(ncm.iPaddedBorderWidth);
        IF_FALSE_THROW(SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
            ncm.cbSize, &ncm, 0));
        _font = CreateFontIndirect(&ncm.lfMessageFont);
        IF_NULL_THROW(_font);
    }
    ~Font() { DeleteObject(_font); }
    operator HFONT () const { return _font; }
private:
    HFONT _font;
};

template<class T> class WindowsTemplate : Uncopyable
{
public:
    WindowsTemplate() : _classAtom(0) { }

    void initialize(HINSTANCE hInst)
    {
        _hInst = hInst;
        WNDCLASS wc;
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = DefWindowProc; //wndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = hInst;
        wc.hIcon = NULL;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_MENU + 1);
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

    const Font* font() const { return &_font; }

    static WNDPROC subClass(HWND hWnd)
    {
        WNDPROC origWndProc = getWndProc(hWnd);
        SetLastError(0);
        IF_ZERO_CHECK_THROW_LAST_ERROR(SetWindowLongPtr(hWnd, GWLP_WNDPROC,
            reinterpret_cast<LONG>(wndProc)));
        return origWndProc;
    }

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
                        case CBN_SELCHANGE:
                            dynamic_cast<ComboBox*>(getContext(lParam))
                                ->changed();
                            break;
                    }
                    break;
                case WM_HSCROLL:
                    {
                        Slider* slider =
                            dynamic_cast<Slider*>(getContext(lParam));
                        if (slider != 0)
                            slider->updateValue(
                                SendMessage(slider->_hWnd, TBM_GETPOS, 0, 0));
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
        // this message to the original wndProc.
        WNDPROC origWndProc = getWndProc(hWnd);
        if (origWndProc != wndProc)
            return CallWindowProc(origWndProc, hWnd, uMsg, wParam, lParam);
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    static WNDPROC getWndProc(HWND hWnd)
    {
        SetLastError(0);
        LONG_PTR r = GetWindowLongPtr(hWnd, GWLP_WNDPROC);
        IF_ZERO_CHECK_THROW_LAST_ERROR(r);
        return reinterpret_cast<WNDPROC>(r);
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
    Font _font;

    static Exception _exception;
    static bool _failed;

    friend class WindowsWindow;
};

typedef WindowsTemplate<void> Windows;

bool Windows::_failed = false;
Exception Windows::_exception;

class ContainerWindow;

template<class T> class WindowTemplate;
typedef WindowTemplate<void> Window;

template<class T> class WindowTemplate
  : public LinkedListMember<WindowTemplate<T>>
{
public:
    WindowTemplate() : _parent(0), _topLeft(Vector(0, 0)) { }
    ~WindowTemplate() { remove(); }
    virtual void create() = 0;
    void setParent(ContainerWindow* window) { _parent = window; }
    ContainerWindow* parent() const { return _parent; }
    void setSize(Vector size) { _size = size; }
    void sizeSet(Vector size) { _size = size; }
    void setPosition(Vector topLeft) { _topLeft = topLeft; }
    void positionSet(Vector topLeft) { _topLeft = topLeft; }
    Vector size() const { return _size; }
    Vector topLeft() const { return _topLeft; }
    virtual void resize() { }
    virtual void doneResize() { }
    virtual void keyboardCharacter(int character) { }
    virtual bool keyboardEvent(int key, bool up) { return false; }
    //virtual void draw(Bitmap<DWORD> bitmap) { }

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

    virtual void invalidate() { invalidateRectangle(Vector(0, 0), _size); }
    virtual void invalidateRectangle(Vector topLeft, Vector size)
    {
        _parent->invalidateRectangle(_topLeft + topLeft, size);
    }

    // The draw() function should do any updates necessary for an animation and
    // (for a BitmapWindow) initiate the process of populating the bitmap. It
    // should not take a long time though, as it is called on the UI thread. If
    // longer is spent in draw() than the time between animation frames,
    // WM_PAINT starvation can result.
    virtual void draw() { }

    int top() const { return _topLeft.y; }
    int bottom() const { return _topLeft.y + _size.y; }
    int left() const { return _topLeft.x; }
    int right() const { return _topLeft.x + _size.x; }
    Vector topRight() const { return _topLeft + Vector(_size.x, 0); }
    Vector bottomRight() const { return _topLeft + _size; }
    Vector bottomLeft() const { return _topLeft + Vector(0, _size.y); }
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
    //void draw(Bitmap<DWORD> bitmap)
    //{
    //    Window* window = _container.getNext();
    //    while (window != 0) {
    //        window->draw(bitmap.subBitmap(window->topLeft(), window->size()));
    //        window = _container.getNext(window);
    //    }
    //}
    void keyboardCharacter(int character)
    {
        if (_focus != 0)
            _focus->keyboardCharacter(character);
    }
    bool keyboardEvent(int key, bool up)
    {
        if (_focus != 0)
            return _focus->keyboardEvent(key, up);
        return false;
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


class WindowsWindow : public ContainerWindow
{
    friend class WindowsTemplate<void>;
public:
    WindowsWindow()
      : _hdc(NULL), _hWnd(NULL), _resizing(false), _origWndProc(0)
    {
        sizeSet(Vector(CW_USEDEFAULT, CW_USEDEFAULT));
        positionSet(Vector(CW_USEDEFAULT, CW_USEDEFAULT));
    }

    virtual void setWindows(Windows* windows)
    {
        _windows = windows;
        _text = _windows->caption();
        _className = _windows->className();
        _style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
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
        _origWndProc = Windows::subClass(_hWnd);
        _hdc = GetDC(_hWnd);
        IF_NULL_THROW(_hdc);
        Windows::setContext(_hWnd, this);
        RECT rect;
        IF_ZERO_THROW(GetClientRect(_hWnd, &rect));
        sizeSet(Vector(rect.right - rect.left, rect.bottom - rect.top));
        positionSet(Vector(rect.left, rect.top));
        ContainerWindow::create();

        SendMessage(_hWnd, WM_SETFONT,
            reinterpret_cast<WPARAM>(_windows->font()->operator HFONT()),
            static_cast<LPARAM>(FALSE));
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
            RECT rect;
            rect.left = 0;
            rect.right = size.x;
            rect.top = 0;
            rect.bottom = size.y;
            AdjustWindowRectEx(&rect, _style, FALSE, _extendedStyle);
            IF_ZERO_THROW(SetWindowPos(
                _hWnd,                                // hWnd
                NULL,                                 // hWndInsertAfter
                0,                                    // X
                0,                                    // Y
                rect.right - rect.left,               // cx
                rect.bottom - rect.top,               // cy
                SWP_NOZORDER | SWP_NOMOVE |
                SWP_NOACTIVATE | SWP_NOREPOSITION));  // uFlags
            // sizeSet() will be called via WM_SIZE, but we want to make sure
            // our _topLeft is set correctly now so it can be used for layout
            // before the message loop next runs.
            ContainerWindow::sizeSet(size);
        }
        else
            sizeSet(size);
    }
    virtual void sizeSet(Vector size)
    {
        ContainerWindow::sizeSet(size);
    }

    void setPosition(Vector position)
    {
        if (_hWnd != NULL) {
            IF_ZERO_THROW(SetWindowPos(
                _hWnd,                                // hWnd
                NULL,                                 // hWndInsertAfter
                position.x,                           // X
                position.y,                           // Y
                0,                                    // cx
                0,                                    // cy
                SWP_NOZORDER | SWP_NOSIZE |
                SWP_NOACTIVATE | SWP_NOREPOSITION));  // uFlags
            // sizeSet() will be called via WM_SIZE, but we want to make sure
            // our _topLeft is set correctly now so it can be used for layout
            // before the message loop next runs.
            ContainerWindow::positionSet(position);
        }
        else
            positionSet(position);
    }
    virtual void positionSet(Vector position)
    {
        ContainerWindow::positionSet(position);
    }
    HWND hWnd() const { return _hWnd; }

    void postMessage(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0)
    {
        PostMessage(_hWnd, msg, wParam, lParam);
    }
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
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                if (keyboardEvent(wParam, false))
                    return 0;
                break;
            case WM_KEYUP:
            case WM_SYSKEYUP:
                if (keyboardEvent(wParam, true))
                    return 0;
                break;
            case WM_KILLFOCUS:
                releaseCapture();
                break;
        }
        if (_origWndProc == 0)
            return DefWindowProc(_hWnd, uMsg, wParam, lParam);
        return CallWindowProc(_origWndProc, _hWnd, uMsg, wParam, lParam);
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
    HDC _hdc;
    String _text;
    DWORD _style;
private:
    bool _resizing;
    Windows* _windows;
    DWORD _extendedStyle;
    LPCWSTR _className;
    HMENU _menu;
    WNDPROC _origWndProc;
};

class AnimatedWindow : public WindowsWindow
{
public:
    AnimatedWindow()
        : _period(50), _timerExpired(true), _delta(0), _lastTickCount(0) { }
    void setDrawWindow(Window* window)
    {
        _drawWindow = window;
    }
    void setRate(float rate) { _period = 1000.0f/rate; }
    void stop() { KillTimer(_hWnd, _timer); _stopped = true; }
    void start()
    {
        stop();
        _stopped = false;
        _timerExpired = true;
        _drawWindow->draw();
    }
    // Invalidation window needs to call this on paint or invalidate to restart
    // timer.
    void restart()
    {
        if (_timerExpired && !_stopped) {
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
                _timer = SetTimer(_hWnd, 1, static_cast<UINT>(ms), NULL);
                IF_ZERO_THROW(_timer);
            }
            else
                postMessage(WM_TIMER, 1, 0);
            _timerExpired = false;
        }
    }

protected:
    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg) {
            case WM_TIMER:
                if (wParam == 1)
                    start();
                break;
        }
        return WindowsWindow::handleMessage(uMsg, wParam, lParam);
    }

private:
    UINT_PTR _timer;
    bool _timerExpired;
    float _period;
    float _delta;
    DWORD _lastTickCount;
    bool _stopped;
    Window* _drawWindow;
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
    void create()
    {
        WindowsWindow::create();
        SIZE s;
        NullTerminatedWideString w(_text);
        IF_ZERO_THROW(GetTextExtentPoint32(_hdc, w, _text.length(), &s));
        Vector size(s.cx + 20, s.cy + 20);
        setSize(size);
    }
    bool checked()
    {
        return SendMessage(_hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
    }
    void uncheck()
    {
        SendMessage(_hWnd, BM_SETCHECK, static_cast<WPARAM>(FALSE), 0);
    }
    void check()
    {
        SendMessage(_hWnd, BM_SETCHECK, static_cast<WPARAM>(TRUE), 0);
    }
};

class ToggleButton : public Button
{
public:
    void create()
    {
        setStyle(BS_AUTOCHECKBOX | BS_PUSHLIKE | BS_PUSHBUTTON | BS_TEXT |
            WS_CHILD | WS_VISIBLE);
        Button::create();
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
    void create()
    {
        WindowsWindow::create();
        size();
    }
    void size()
    {
        if (_hWnd != NULL) {
            SIZE s;
            NullTerminatedWideString w(_text);
            IF_ZERO_THROW(GetTextExtentPoint32(_hdc, w, _text.length(), &s));
            Vector size(s.cx, s.cy);
            setSize(size);
        }
    }
};

class Slider : public WindowsWindow
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
            static_cast<LPARAM>(MAKELONG(0, 16384)));
        setValue(_pos);
    }
    void setValue(double pos)
    {
        _pos = pos;
        if (_hWnd != NULL) {
            int iPos =
                static_cast<int>(((pos - _min) * 16384)/(_max - _min) + 0.5);
            SendMessage(_hWnd, TBM_SETPOS, static_cast<WPARAM>(TRUE),
                static_cast<LPARAM>(iPos));
        }
        valueSet(pos);
    }
    void setRange(double min, double max) { _min = min; _max = max; }
    void updateValue(int pos)
    {
        _pos = pos*(_max - _min) / 16384 + _min;
        valueSet(_pos);
    }
private:
    double _min;
    double _max;
    double _pos;
};

class ComboBox : public WindowsWindow
{
public:
    virtual void changed(int value) { }
    void setWindows(Windows* windows)
    {
        WindowsWindow::setWindows(windows);
        setClassName(WC_COMBOBOX);
        setStyle(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL);
    }
    void create()
    {
        WindowsWindow::create();
        _width = 0;
    }
    void set(int value)
    {
        SendMessage(_hWnd, CB_SETCURSEL, value, 0);
    }
    void changed()
    {
        changed(SendMessage(_hWnd, CB_GETCURSEL, 0, 0));
    }
    void add(String s)
    {
        NullTerminatedWideString ns(s);
        int r = SendMessage(_hWnd, CB_ADDSTRING, 0,
            reinterpret_cast<LPARAM>(ns.operator WCHAR *()));
        SIZE size;
        NullTerminatedWideString w(s);
        IF_ZERO_THROW(GetTextExtentPoint32(_hdc, w, s.length(), &size));
        _width = max(_width, static_cast<int>(size.cx));
    }
    void autoSize() { setSize(Vector(_width + 20, 200)); }
private:
    int _width;
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


class PaintHandle : public DeviceContext
{
public:
    PaintHandle(const WindowsWindow* window) : _window(window)
    {
        IF_NULL_THROW(BeginPaint(_window->hWnd(), &_ps));
        _hdc = _ps.hdc;
    }
    ~PaintHandle() { EndPaint(_window->hWnd(), &_ps); }
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


class BitmapWindow : public WindowsWindow
{
public:
    BitmapWindow() { }

    void setWindows(Windows* windows)
    {
        WindowsWindow::setWindows(windows);
        setClassName(WC_STATIC);
        setStyle(WS_CHILD | WS_VISIBLE | SS_OWNERDRAW);
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
        draw();
        WindowsWindow::create();
    }

    void resize()
    {
        draw();
    }

    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg) {
            case WM_PAINT:
                {
                    PaintHandle paintHandle(this);
                    if (!paintHandle.zeroArea()) {
                        if (!_bitmap.valid())
                            return 0;
                        Vector ptl = paintHandle.topLeft();
                        Vector pbr = paintHandle.bottomRight();
                        Vector br = size();
                        pbr = Vector(min(pbr.x, br.x), min(pbr.y, br.y));
                        Vector ps = pbr - ptl;
                        if (ps.x <= 0 || ps.y <= 0)
                            return 0;
                        Vector s = size();
                        _bmi.bmiHeader.biWidth = s.x;
                        _bmi.bmiHeader.biHeight = -s.y;
                        paint();
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
                    }
                    return 0;
                }
            case WM_USER:
                {
                    Lock lock(&_mutex);
                    _lastBitmap = _bitmap;
                    _bitmap = _nextBitmap;
                    return 0;
                }
        }
        return WindowsWindow::handleMessage(uMsg, wParam, lParam);
    }

    virtual void paint() { };

    Bitmap<DWORD> setNextBitmap(Bitmap<DWORD> nextBitmap)
    {
        Lock lock(&_mutex);
        _nextBitmap = nextBitmap;
        postMessage(WM_USER);
        return _lastBitmap;
    }

    int stride() const { return _bitmap.stride(); }
    const Byte* data() const { return _bitmap.data(); }
    Byte* data() { return _bitmap.data(); }

private:
    Mutex _mutex;

    Bitmap<DWORD> _bitmap;
    Bitmap<DWORD> _nextBitmap;
    Bitmap<DWORD> _lastBitmap;

    BITMAPINFO _bmi;
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
