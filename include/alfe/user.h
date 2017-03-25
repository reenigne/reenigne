#include "alfe/main.h"

#ifndef INCLUDED_USER_H
#define INCLUDED_USER_H

#include "alfe/bitmap.h"

// TODO: Xlib port

#include <windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>
#include <vector>
#include <functional>

template<class T> class WindowsWindowT;
typedef WindowsWindowT<void> WindowsWindow;

template<class T> class PaintHandleT;
typedef PaintHandleT<void> PaintHandle;

class GDIObject : public ConstHandle
{
public:
    GDIObject() { }
    GDIObject(HGDIOBJ object) : _object(object)
    {
        IF_NULL_THROW(object);
        ConstHandle::operator=(create<Body>(object));
    }
    operator HGDIOBJ() const { return _object; }
    HGDIOBJ object() const { return _object; }
protected:
    class Body : public ConstHandle::Body
    {
    public:
        Body(HGDIOBJ object) : _object(object) { }
        ~Body() { DeleteObject(_object); }
        HGDIOBJ _object;
    };
    HGDIOBJ _object;
};

class Font : public GDIObject
{
public:
    Font() : GDIObject(object()) { }
    operator HFONT() const { return static_cast<HFONT>(_object); }
private:
    HGDIOBJ object()
    {
        NONCLIENTMETRICS ncm;
        ncm.cbSize = sizeof(NONCLIENTMETRICS) - sizeof(ncm.iPaddedBorderWidth);
        IF_FALSE_THROW(SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
            ncm.cbSize, &ncm, 0));
        return CreateFontIndirect(&ncm.lfMessageFont);
    }
};

template<class T> class WindowsT : Uncopyable
{
public:
    WindowsT() : _failed(false) { }
    void check()
    {
        if (!_failed)
            return;
        _failed = false;
        throw _exception;
    }
    void stashException(Exception exception)
    {
        _failed = true;
        _exception = exception;
    }
    bool failed() const { return _failed; }

    const Font* font() const { return &_font; }

private:
    Font _font;
    Exception _exception;
    bool _failed;
};

typedef WindowsT<void> Windows;

class ContainerWindow;

template<class T> class WindowT;
typedef WindowT<void> Window;

template<class T> class WindowT
  : public LinkedListMember<WindowT<T>>
{
public:
    WindowT() : _parent(0), _topLeft(Vector(0, 0)) { }
    ~WindowT() { remove(); }
    virtual void create() { layout(); }
    virtual void layout() { }
    void setParent(ContainerWindow* window) { _parent = window; }
    ContainerWindow* parent() const { return _parent; }

    // We call these functions to notify derived classes about events
    virtual void innerSizeSet(Vector size) { }
    virtual void positionSet(Vector topLeft) { }
    // Derived classes override these to implement windows with frames
    virtual Vector outerSize() const { return _innerSize; }
    virtual Vector outerTopLeft() const { return Vector(0, 0); }
    virtual void setInnerSize(Vector size)
    {
        if (size == _innerSize)
            return;
        _innerSize = size;
        innerSizeSet(size);
    }
    Vector innerSize() const { return _innerSize; }
    Vector origin() const { return _topLeft; }
    // These all return outer edges/corners in the parent coordinate system,
    // useful for laying out.
    int left() const { return _topLeft.x + outerTopLeft().x; }
    int top() const { return _topLeft.y + outerTopLeft().y; }
    int right() const { return _topLeft.x + outerTopLeft().x + outerSize().x; }
    int bottom() const
    {
        return _topLeft.y + outerTopLeft().y + outerSize().y;
    }
    Vector topLeft() const { return _topLeft + outerTopLeft(); }
    Vector topRight() const { return topLeft() + Vector(outerSize().x, 0); }
    Vector bottomRight() const { return topLeft() + outerSize(); }
    Vector bottomLeft() const { return topLeft() + Vector(0, outerSize().y); }
    virtual void setTopLeft(Vector tl)
    {
        _topLeft = tl - outerTopLeft();
        positionSet(_topLeft);
    }
    void setTopRight(Vector topRight)
    {
        setTopLeft(topRight - Vector(outerSize().x, 0));
    }
    void setBottomLeft(Vector bottomLeft)
    {
        setTopLeft(bottomLeft - Vector(0, outerSize().y));
    }
    void setBottomRight(Vector bottomRight)
    {
        setTopLeft(bottomRight - outerSize());
    }

    virtual void doneResize() { }
    virtual void keyboardCharacter(int character) { }
    virtual bool keyboardEvent(int key, bool up) { return false; }
    //virtual void draw(Bitmap<DWORD> bitmap) { }

    // mouseInput() should return true if the mouse should be captured
    virtual bool mouseInput(Vector position, int buttons, int wheel)
    {
        return false;
    }
    virtual void releaseCapture() { }
    void remove()
    {
        if (_parent != 0) {
            _parent->childRemoved(this);
            LinkedListMember::remove();
        }
    }

    // The draw() function should do any updates necessary for an animation and
    // (for a BitmapWindow) initiate the process of populating the bitmap. It
    // should not take a long time though, as it is called on the UI thread. If
    // longer is spent in draw() than the time between animation frames,
    // WM_PAINT starvation can result.
    virtual void draw() { }

private:
    ContainerWindow* _parent;
    Vector _innerSize;

    // _topLeft is the position within the parent window of the origin
    // (top-left of inner/client rectangle) of this window.
    Vector _topLeft;
};

class ContainerWindow : public Window
{
public:
    ContainerWindow() : _focus(0), _capture(0) { }
    void create()
    {
        for (auto& window : _container) {
            window.setParent(this);
            window.create();
        }
        Window::create();
    }
    void remove()
    {
        for (auto& window : _container)
            window.remove();
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
    //    for (auto& window : _container)
    //        window.draw(bitmap.subBitmap(window.topLeft(), window.outerSize()));
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
    bool mouseInput(Vector position, int buttons, int wheel)
    {
        // If the mouse is captured, send the input to the capturing window
        if (_capture != 0) {
            _capture->mouseInput(position - _capture->topLeft(), buttons,
                wheel);
            if (buttons == 0)
                releaseCapture();
            return false;
        }
        // Otherwise, send the input the window under the mouse
        Window* window = windowForPosition(position);
        if (window != 0) {
            bool capture = window->mouseInput(position - window->topLeft(),
                buttons, wheel);
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
    void repositionChildren()
    {
        Window* window = _container.getNext();
        while (window != 0) {
            window->setTopLeft(window->topLeft());
            window = _container.getNext(window);
        }
    }
    virtual void enableWindow(bool enabled)
    {
        for (auto& window : _container)
            dynamic_cast<ContainerWindow*>(&window)->enableWindow(enabled);
    }
protected:
    LinkedList<Window> _container;
    Window* _focus;
    Window* _capture;
    int _buttons;
private:
    Window* windowForPosition(Vector p)
    {
        for (auto& window : _container) {
            if ((p - window.topLeft()).inside(window.outerSize()))
                return &window;
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
    void setDC(HDC hdc) { _hdc = hdc; }
    void NoFailSelectObject(HGDIOBJ hObject) { ::SelectObject(_hdc, hObject); }
    HGDIOBJ SelectObject(HGDIOBJ hObject)
    {
        HGDIOBJ r = ::SelectObject(_hdc, hObject);
        IF_NULL_THROW(r);
        return r;
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

class SelectedObject : public ConstHandle
{
public:
    SelectedObject() { }
    SelectedObject(DeviceContext* dc, const GDIObject& object)
        : ConstHandle(create<Body>(dc, object)) { }
private:
    class Body : public ConstHandle::Body
    {
    public:
        Body(DeviceContext* dc, const GDIObject& object) : _dc(dc)
        {
            _previous = dc->SelectObject(object);
        }
        ~Body()
        {
            _dc->NoFailSelectObject(_previous);
        }
    private:
        DeviceContext* _dc;
        HGDIOBJ _previous;
    };
};

class Pen : public GDIObject
{
public:
    Pen(int fnPenStyle, int nWidth, COLORREF crColor)
        : GDIObject(CreatePen(fnPenStyle, nWidth, crColor)) { }
    operator HPEN() { return static_cast<HPEN>(_object); }
};

template<class T> class WindowsWindowT : public ContainerWindow
{
    friend class WindowsT<void>;
public:
    WindowsWindowT()
      : _hWnd(NULL), _resizing(false), _origWndProc(0), _className(0),
        _style(WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN), _extendedStyle(0),
        _menu(NULL), _windowsParent(0), _outerSize(0, 0), _outerTopLeft(0, 0)
    {
        _dc.setDC(NULL);
        ContainerWindow::setInnerSize(Vector(CW_USEDEFAULT, CW_USEDEFAULT));
        ContainerWindow::setTopLeft(Vector(CW_USEDEFAULT, CW_USEDEFAULT));
    }
    void setWindows(Windows* windows) { _windows = windows; }
    Vector clientToScreen(Vector client)
    {
        POINT p;
        p.x = client.x;
        p.y = client.y;
        IF_ZERO_THROW(ClientToScreen(_hWnd, &p));
        return Vector(p.x, p.y);
    }
    Vector mousePosition()
    {
        POINT point;
        IF_ZERO_THROW(GetCursorPos(&point));
        return Vector(point.x, point.y) - clientToScreen(Vector(0, 0));
    }
    virtual HWND hWndParent()
    {
        if (_windowsParent != 0)
            return _windowsParent->hWnd();
        return NULL;
    }
    virtual void create()
    {
        reset();
        Vector s = adjustRect(innerSize());
        Vector position = topLeft();
        Vector o(0, 0);

        Window* p = parent();
        while (p != 0) {
            auto w = dynamic_cast<WindowsWindow*>(p);
            if (w != 0) {
                _windowsParent = w;
                _windows = w->_windows;
                break;
            }
            else
                o += p->origin();
            p = p->parent();
        }

        position += o;
        if (topLeft().x == CW_USEDEFAULT)
            position.x = CW_USEDEFAULT;
        if (topLeft().y == CW_USEDEFAULT)
            position.y = CW_USEDEFAULT;
        if (innerSize().x == CW_USEDEFAULT)
            s.x = CW_USEDEFAULT;
        if (innerSize().y == CW_USEDEFAULT)
            s.y = CW_USEDEFAULT;

        NullTerminatedWideString caption(_text);

        HMENU menu = _menu;
        if ((_style & WS_CHILD) != 0)
            menu = reinterpret_cast<HMENU>(this);

        LPCWSTR className = _className;
        // We don't really need to register our own window class - we can use
        // one of the built-in classes. Some demoscene sizecoded demos use
        // "Edit" for example (but that sets the cursor to the I-beam). And
        // WC_STATIC doesn't work with SetLayeredWindowAttributes.
        if (className == 0)
            className = L"MDIClient";

        HINSTANCE hInstance = GetModuleHandle(NULL);
        IF_ZERO_THROW(hInstance);
        _hWnd = CreateWindowEx(
            _extendedStyle,
            className,
            caption,
            _style,
            position.x,
            position.y,
            s.x,
            s.y,
            hWndParent(),
            menu,
            hInstance,
            this);
        IF_NULL_THROW(_hWnd);
        SetLastError(0);
        IF_ZERO_CHECK_THROW_LAST_ERROR(SetWindowLongPtr(_hWnd, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(this)));
        if (_className == 0)
            _origWndProc = DefWindowProc;
        else {
            SetLastError(0);
            LONG_PTR r = GetWindowLongPtr(_hWnd, GWLP_WNDPROC);
            IF_ZERO_CHECK_THROW_LAST_ERROR(r);
            _origWndProc = reinterpret_cast<WNDPROC>(r);
        }
        SetLastError(0);
        IF_ZERO_CHECK_THROW_LAST_ERROR(SetWindowLongPtr(_hWnd, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(wndProc)));
        HDC hdc = GetDC(_hWnd);
        IF_NULL_THROW(hdc);
        _dc.setDC(hdc);
        RECT clientRect;
        IF_ZERO_THROW(GetClientRect(_hWnd, &clientRect));
        RECT windowRect;
        IF_ZERO_THROW(GetWindowRect(_hWnd, &windowRect));
        Vector origin = clientToScreen(Vector(0, 0));
        Vector stl(windowRect.left, windowRect.top);
        _outerTopLeft = stl - origin;
        _outerSize = Vector(windowRect.right, windowRect.bottom) - stl;
        Vector pOrigin(0, 0);
        if (_windowsParent != 0)
            pOrigin = _windowsParent->clientToScreen(Vector(0, 0));

        ContainerWindow::setInnerSize(Vector(
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top));
        ContainerWindow::setTopLeft(origin - (pOrigin + o));

        SendMessage(_hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(font()),
            static_cast<LPARAM>(FALSE));
        SelectObject(_dc, font());

        ContainerWindow::create();
    }

    ~WindowsWindowT() { reset(); }

    void show(int nShowCmd) { ShowWindow(_hWnd, nShowCmd); }

    void setText(String text)
    {
        _text = text;
        if (_hWnd != NULL) {
            NullTerminatedWideString w(text);
            IF_ZERO_THROW(SetWindowText(_hWnd, w));
        }
    }
    String getText()
    {
        int l = GetWindowTextLength(_hWnd) + 1;
        Array<WCHAR> buf(l);
        IF_ZERO_CHECK_THROW_LAST_ERROR(GetWindowText(_hWnd, &buf[0], l));
        return String(&buf[0]);
    }

    void setInnerSize(Vector size)
    {
        _outerSize = adjustRect(size);
        if (!_resizing) {
            if (_hWnd != NULL) {
                IF_ZERO_THROW(SetWindowPos(
                    _hWnd,                                // hWnd
                    NULL,                                 // hWndInsertAfter
                    0,                                    // X
                    0,                                    // Y
                    _outerSize.x,                         // cx
                    _outerSize.y,                         // cy
                    SWP_NOZORDER | SWP_NOMOVE |
                    SWP_NOACTIVATE | SWP_NOREPOSITION |
                    SWP_NOREDRAW | SWP_ASYNCWINDOWPOS));  // uFlags
                // ContainerWindow::setInnerSize() will be called via WM_SIZE,
                // but we want to make sure our size is set correctly now so it
                // can be used for layout before the message loop next runs.
            }
        }
        ContainerWindow::setInnerSize(size);
    }

    void setTopLeft(Vector position)
    {
        if (_hWnd != NULL) {
            Vector tl = position + _outerTopLeft;
            Window* p = parent();
            while (p != 0 && p != static_cast<Window*>(_windowsParent)) {
                tl += p->origin();
                p = p->parent();
            }
            IF_ZERO_THROW(SetWindowPos(
                _hWnd,                                // hWnd
                NULL,                                 // hWndInsertAfter
                tl.x,                                 // X
                tl.y,                                 // Y
                0,                                    // cx
                0,                                    // cy
                SWP_NOZORDER | SWP_NOSIZE |
                SWP_NOACTIVATE | SWP_NOREPOSITION |
                SWP_NOREDRAW | SWP_ASYNCWINDOWPOS));  // uFlags
            // ContainerWindow::setTopLeft() will be called via WM_MOVE, but we
            // want to make sure our position is set correctly now so it can be
            // used for layout before the message loop next runs.
        }
        ContainerWindow::setTopLeft(position);
    }
    HWND hWnd() const { return _hWnd; }
    HDC getDC() { return _dc; }
    HFONT font() { return *_windows->font(); }

    void postMessage(UINT msg, WPARAM wParam = 0, LPARAM lParam = 0)
    {
        PostMessage(_hWnd, msg, wParam, lParam);
    }

    void updateWindow()
    {
        if (_hWnd != NULL)
            IF_ZERO_THROW(UpdateWindow(_hWnd));
    }
    void redrawWindow(Vector topLeft, Vector size, UINT flags)
    {
        if (_hWnd == NULL)
            return;
        RECT rect;
        rect.left = topLeft.x;
        rect.top = topLeft.y;
        rect.right = topLeft.x + size.x;
        rect.bottom = topLeft.y + size.y;
        IF_ZERO_THROW(RedrawWindow(_hWnd, &rect, NULL, flags));
    }
    void redrawWindow(UINT flags)
    {
        redrawWindow(Vector(0, 0), outerSize(), flags);
    }
    void enableWindow(bool enabled)
    {
        EnableWindow(_hWnd, enabled ? TRUE : FALSE);
        ContainerWindow::enableWindow(enabled);
    }
    void invalidate() { redrawWindow(RDW_INVALIDATE | RDW_FRAME); }
    Vector outerSize() const { return _outerSize; }
    Vector outerTopLeft() const { return _outerTopLeft; }
private:
    Vector adjustRect(Vector size)
    {
        RECT rect;
        rect.left = 0;
        rect.right = size.x;
        rect.top = 0;
        rect.bottom = size.y;
        AdjustWindowRectEx(&rect, _style, FALSE, _extendedStyle);
        return Vector(rect.right - rect.left, rect.bottom - rect.top);
    }

    void destroy()
    {
        if (getDC() != NULL) {
            ReleaseDC(_hWnd, _dc);
            _dc.setDC(NULL);
        }
        _hWnd = NULL;
    }

    void reset()
    {
        if (getDC() != NULL) {
            ReleaseDC(_hWnd, _dc);
            _dc.setDC(NULL);
        }
        if (_hWnd != NULL) {
            DestroyWindow(_hWnd);
            _hWnd = NULL;
        }
    }

    void setHwnd(HWND hWnd) { _hWnd = hWnd; }
protected:
    virtual bool command(WORD code) { return false; }
    virtual bool hScroll(WORD code, WORD position) { return false; }
    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        int w = static_cast<int>(wParam);
        switch (uMsg) {
            case WM_SIZING:
                {
                    _resizing = true;
                    RECT* r = reinterpret_cast<RECT*>(lParam);
                    Vector requestedSize(r->right - r->left,
                        r->bottom - r->top);
                    ContainerWindow::setInnerSize(
                        requestedSize + innerSize() - outerSize());
                    Vector adjustedSize = outerSize();
                    if (wParam == WMSZ_TOPLEFT || wParam == WMSZ_LEFT ||
                        wParam == WMSZ_BOTTOMLEFT)
                        r->left = r->right - adjustedSize.x;
                    if (wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOP ||
                        wParam == WMSZ_TOPRIGHT)
                        r->top = r->bottom - adjustedSize.y;
                    if (wParam == WMSZ_TOPRIGHT || wParam == WMSZ_RIGHT ||
                        wParam == WMSZ_BOTTOMRIGHT)
                        r->right = r->left + adjustedSize.x;
                    if (wParam == WMSZ_BOTTOMLEFT || wParam == WMSZ_BOTTOM ||
                        wParam == WMSZ_BOTTOMRIGHT)
                        r->bottom = r->top + adjustedSize.y;
                }
                return TRUE;
            case WM_MOVE:
                ContainerWindow::setTopLeft(
                    vectorFromLParam(lParam) + _outerTopLeft);
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
            case WM_MOUSEWHEEL:
                {
                    int buttons;
                    int wheel;
                    if (uMsg == WM_MOUSEWHEEL) {
                        buttons = GET_KEYSTATE_WPARAM(wParam);
                        wheel = GET_WHEEL_DELTA_WPARAM(wParam);
                    }
                    else {
                        buttons = w;
                        wheel = 0;
                    }
                    if (mouseInput(vectorFromLParam(lParam), buttons, wheel))
                        SetCapture(_hWnd);
                }
                break;
            case WM_CHAR:
                keyboardCharacter(w);
                break;
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                if (keyboardEvent(w, false))
                    return 0;
                break;
            case WM_KEYUP:
            case WM_SYSKEYUP:
                if (keyboardEvent(w, true))
                    return 0;
                break;
            case WM_KILLFOCUS:
                releaseCapture();
                break;
            case WM_GETDLGCODE:
                return DLGC_WANTMESSAGE;
            case WM_COMMAND:
                if (lParam != 0) {
                    WindowsWindow* window = getContext(lParam);
                    if (window != 0 && window->command(HIWORD(wParam)))
                        return 0;
                }
                else {
                    if (LOWORD(wParam) == IDCANCEL)
                        keyboardCharacter(VK_ESCAPE);
                }
                break;
            case WM_HSCROLL:
                if (lParam != 0) {
                    if (getContext(lParam)->hScroll(LOWORD(wParam),
                        HIWORD(wParam)))
                        return 0;
                }
                break;
            case WM_NCDESTROY:
                destroy();
                remove();
                break;
        }
        return originalHandleMessage(uMsg, wParam, lParam);
    }
    void erase(bool invalidateChildren = false)
    {
        PaintHandle paintHandle(this);
        if (paintHandle.zeroArea())
            return;
        if (!paintHandle.erase())
            return;
        HBRUSH hBrush = GetSysColorBrush(COLOR_BTNFACE);
        SelectedObject bo(&paintHandle, hBrush);
        Pen pen(PS_SOLID, 1, GetSysColor(COLOR_BTNFACE));
        SelectedObject po(&paintHandle, pen);
        Vector tl = paintHandle.topLeft();
        Vector br = paintHandle.bottomRight();
        Rectangle(paintHandle, tl.x, tl.y, br.x, br.y);
        if (invalidateChildren) {
            redrawWindow(tl, br - tl,
                RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
        }
    }
    LRESULT originalHandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
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
    void setExtendedStyle(DWORD extendedStyle)
    {
        _extendedStyle = extendedStyle;
    }

    HWND _hWnd;
    DeviceContext _dc;
    String _text;
    DWORD _style;
    Vector _outerSize;
    Vector _outerTopLeft;
private:
    bool _resizing;
    Windows* _windows;
    WindowsWindow* _windowsParent;
    DWORD _extendedStyle;
    LPCWSTR _className;
    HMENU _menu;
    WNDPROC _origWndProc;

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

    LRESULT windowProcedure(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        // Avoid reentering and causing a cascade of alerts if an assert fails.
        if (!alerting && !_windows->failed()) {
            BEGIN_CHECKED {
                return handleMessage(uMsg, wParam, lParam);
            } END_CHECKED(Exception& e) {
                PostQuitMessage(0);
                _windows->stashException(e);
            }
        }
        return originalHandleMessage(uMsg, wParam, lParam);
    }

    static LRESULT CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam,
        LPARAM lParam)
    {
        return getContext(hWnd)->handleMessage(uMsg, wParam, lParam);
    }
};

template<class T> class PaintHandleT : public DeviceContext
{
public:
    PaintHandleT(const WindowsWindow* window) : _window(window)
    {
        IF_NULL_THROW(BeginPaint(_window->hWnd(), &_ps));
        _hdc = _ps.hdc;
    }
    ~PaintHandleT() { EndPaint(_window->hWnd(), &_ps); }
    Vector topLeft() const
    {
        return Vector(_ps.rcPaint.left, _ps.rcPaint.top);
    }
    Vector bottomRight() const
    {
        return Vector(_ps.rcPaint.right, _ps.rcPaint.bottom);
    }
    bool zeroArea() const { return (topLeft() - bottomRight()).zeroArea(); }
    bool erase() const { return _ps.fErase != 0; }
private:
    const WindowsWindow* _window;
    PAINTSTRUCT _ps;
};

class CompoundWindow : public WindowsWindow
{
public:
    LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_PAINT) {
            erase();
            return 0;
        }
        if (uMsg == WM_ERASEBKGND)
            return 0;
        if (uMsg == WM_SIZE) {
            Vector s = vectorFromLParam(lParam);
            if (s != innerSize()) {
                ContainerWindow::setInnerSize(s);
                redrawWindow(RDW_INVALIDATE | RDW_ALLCHILDREN);
            }
            return 0;
        }
        return WindowsWindow::handleMessage(uMsg, wParam, lParam);
    }
};

class RootWindow : public CompoundWindow
{
public:
    void childRemoved(Window* child)
    {
        if (_container.getNext(child) == 0 && _container.getNext() == child) {
            // Once there are no more child windows left, the thread must
            // end.
            PostQuitMessage(0);
        }
        CompoundWindow::childRemoved(child);
    }
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

class Button : public WindowsWindow
{
public:
    Button() : _checked(false)
    {
        setClassName(WC_BUTTON);
        setStyle(BS_PUSHBUTTON | BS_TEXT | WS_CHILD | WS_VISIBLE | WS_TABSTOP);
    }
    virtual void clicked(bool value) { _clicked(value); }
    void setClicked(std::function<void(bool)> clicked) { _clicked = clicked; }
    void create()
    {
        WindowsWindow::create();
        SIZE s;
        NullTerminatedWideString w(_text);
        IF_ZERO_THROW(GetTextExtentPoint32(_dc, w, _text.length(), &s));
        Vector size(s.cx + 20, s.cy + 10);
        setInnerSize(size);
        setCheckState(_checked);
    }
    bool checked() { return _checked; }
    void uncheck()
    {
        if (_hWnd != NULL)
            SendMessage(_hWnd, BM_SETCHECK, static_cast<WPARAM>(FALSE), 0);
        _checked = false;
    }
    void check()
    {
        if (_hWnd != NULL)
            SendMessage(_hWnd, BM_SETCHECK, static_cast<WPARAM>(TRUE), 0);
        _checked = true;
    }
    void setCheckState(bool checked)
    {
        if (checked)
            check();
        else
            uncheck();
    }
    bool command(WORD code)
    {
        if (code == BN_CLICKED) {
            clicked(
                (Button_GetState(_hWnd) & (BST_CHECKED | BST_PUSHED)) != 0);
            return true;
        }
        return false;
    }
private:
    bool _checked;
    std::function<void(bool)> _clicked;
};

class ToggleButton : public Button
{
public:
    void create()
    {
        setStyle(BS_AUTOCHECKBOX | BS_PUSHLIKE | BS_PUSHBUTTON | BS_TEXT |
            WS_CHILD | WS_VISIBLE | WS_TABSTOP);
        Button::create();
    }
};

class CheckBox : public Button
{
public:
    void create()
    {
        setStyle(BS_AUTOCHECKBOX | BS_TEXT | WS_CHILD | WS_VISIBLE |
            WS_TABSTOP);
        Button::create();
    }
};

class GroupBox : public Button
{
public:
    GroupBox()
    {
        setStyle(BS_GROUPBOX | BS_TEXT | WS_CHILD | WS_VISIBLE);
    }
    // The code in Button::create() isn't appropriate for GroupBox
    void create() { WindowsWindow::create(); }
    LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg == WM_PAINT)
            erase(true);
        if (uMsg == WM_ERASEBKGND)
            return 0;
        return WindowsWindow::handleMessage(uMsg, wParam, lParam);
    }
};

class TextWindow : public WindowsWindow
{
public:
    TextWindow()
    {
        setClassName(WC_STATIC);
        setStyle(WS_CHILD | WS_VISIBLE | /*SS_CENTER |*/ SS_CENTERIMAGE);
    }
    void layout()
    {
        if (_hWnd != NULL) {
            SIZE s;
            NullTerminatedWideString w(_text);
            IF_ZERO_THROW(GetTextExtentPoint32(_dc, w, _text.length(), &s));
            setInnerSize(Vector(s.cx, s.cy));
        }
    }
};

class EditWindow : public WindowsWindow
{
public:
    EditWindow()
    {
        setClassName(WC_EDIT);
        setStyle(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL);
    }
    virtual void changed() { }
    void layout()
    {
        TEXTMETRIC metric;
        GetTextMetrics(getDC(), &metric);
        setInnerSize(Vector(44, metric.tmHeight));
    }
    bool command(WORD code)
    {
        if (code == EN_CHANGE) {
            changed();
            return true;
        }
        return false;
    }
};

class Slider : public WindowsWindow
{
public:
    Slider() : _min(0), _pos(0), _max(1)
    {
        setClassName(TRACKBAR_CLASS);
        setStyle(WS_CHILD | WS_VISIBLE | WS_TABSTOP);
    }
    virtual void valueSet(double value) { }
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
    double getValue() const { return _pos; }
    bool hScroll(WORD code, WORD position)
    {
        updateValue(static_cast<int>(SendMessage(_hWnd, TBM_GETPOS, 0, 0)));
        return true;
    }
private:
    double _min;
    double _max;
    double _pos;
};

class ProgressBar : public WindowsWindow
{
public:
    ProgressBar() : _pos(0)
    {
        setClassName(PROGRESS_CLASS);
        setStyle(WS_CHILD | WS_VISIBLE | PBS_SMOOTH);
    }
    virtual void valueSet(double value) { }
    void create()
    {
        WindowsWindow::create();
        SendMessage(_hWnd, PBM_SETRANGE, static_cast<WPARAM>(TRUE),
            static_cast<LPARAM>(MAKELONG(0, 16384)));
        setValue(_pos);
    }
    void setValue(float pos)
    {
        _pos = pos;
        if (_hWnd != NULL) {
            int iPos = static_cast<int>(pos*16384 + 0.5);
            SendMessage(_hWnd, PBM_SETPOS, static_cast<WPARAM>(iPos),
                static_cast<LPARAM>(0));
        }
    }
private:
    float _pos;
};

template<class T> class NumericSliderWindow
{
public:
    void setText(String text) { _caption.setText(text); }
    void setHost(T* host)
    {
        _host = host;
        host->add(&_slider);
        host->add(&_caption);
        host->add(&_text);
        _slider.setHost(this);
    }
    void setPositionAndSize(Vector position, Vector size)
    {
        _slider.setInnerSize(size);
        _slider.setTopLeft(position);
        _caption.setTopLeft(_slider.bottomLeft() + Vector(0, 15));
        _text.setTopLeft(_caption.topRight());
    }
    Vector bottomLeft() { return _caption.bottomLeft(); }
    int right() const { return _slider.right(); }
    void setRange(double low, double high)
    {
        _slider.setRange(positionFromValue(low), positionFromValue(high));
    }
    void setValue(double value) { _slider.setValue(positionFromValue(value)); }
    double getValue() { return valueFromPosition(_slider.getValue()); }

protected:
    virtual void create() { }
    virtual void valueSet(double value) { }
    virtual double positionFromValue(double value) { return value; }
    virtual double valueFromPosition(double position) { return position; }

    T* _host;
private:
    void valueSet1(double value)
    {
        double v = valueFromPosition(value);
        _text.setText(format("%f", v));
        _text.layout();
        valueSet(v);
    }

    class NumericSlider : public Slider
    {
    public:
        void setHost(NumericSliderWindow* host) { _host = host; }
        void valueSet(double value) { _host->valueSet1(value); }
        void create() { _host->create(); Slider::create(); }
    private:
        NumericSliderWindow* _host;
    };
    NumericSlider _slider;
    TextWindow _caption;
    TextWindow _text;
    friend class NumericSlider;
};

class DropDownList : public WindowsWindow
{
public:
    DropDownList() : _value(0)
    {
        setClassName(WC_COMBOBOX);
        setStyle(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL |
            WS_TABSTOP);
    }
    virtual void changed(int value) { _changed(value); }
    void setChanged(std::function<void(int)> changed) { _changed = changed; }
    void create()
    {
        WindowsWindow::create();
        for (auto s : _entries) {
            NullTerminatedWideString ns(s);
            int r = static_cast<int>(SendMessage(_hWnd, CB_ADDSTRING, 0,
                reinterpret_cast<LPARAM>(ns.operator WCHAR *())));
            IF_FALSE_THROW(r != CB_ERR && r != CB_ERRSPACE);
        }
        set(_value);
    }
    void set(int value)
    {
        if (_hWnd != NULL)
            IF_FALSE_THROW(ComboBox_SetCurSel(_hWnd, value) != CB_ERR);
        _value = value;
    }
    void add(String s) { _entries.add(s); }
    void layout()
    {
        int width = 0;
        for (auto s : _entries) {
            NullTerminatedWideString ns(s);
            SIZE size;
            IF_ZERO_THROW(GetTextExtentPoint32(_dc, ns, s.length(), &size));
            width = max(width, static_cast<int>(size.cx));
        }
        COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };
        IF_ZERO_THROW(GetComboBoxInfo(_hWnd, &info));
        setInnerSize(Vector(10 + width +
            info.rcButton.right - info.rcButton.left, 200));
        RECT rect;
        IF_ZERO_THROW(GetClientRect(info.hwndCombo, &rect));
        _outerSize = Vector(rect.right - rect.left, rect.bottom - rect.top);
    }
    bool command(WORD code)
    {
        if (code == CBN_SELCHANGE) {
            int v = ComboBox_GetCurSel(_hWnd);
            IF_FALSE_THROW(v != CB_ERR);
            changed(v);
            return true;
        }
        return false;
    }
private:
    List<String> _entries;
    int _value;
    std::function<void(int)> _changed;
};

class CaptionedDropDownList : public ContainerWindow
{
public:
    CaptionedDropDownList()
    {
        ContainerWindow::add(&_caption);
        ContainerWindow::add(&_list);
    }
    void setText(String text) { _caption.setText(text); }
    void add(String s) { _list.add(s); }
    void set(int value) { _list.set(value); }
    void layout()
    {
        int captionHeight = _caption.outerSize().y;
        int listHeight = _list.outerSize().y;
        int captionY = 0;
        int listY = 0;
        if (captionHeight > listHeight)
            listY = (captionHeight - listHeight)/2;
        else
            captionY = (listHeight - captionHeight)/2;
        _caption.setTopLeft(Vector(0, captionY));
        _list.setTopLeft(Vector(_caption.right() + 10, listY));
        setInnerSize(
            Vector(_list.right(), max(_caption.bottom(), _list.bottom())));
    }
    void setChanged(std::function<void(int)> changed)
    {
        _list.setChanged(changed);
    }
    void setTopLeft(Vector topLeft)
    {
        ContainerWindow::setTopLeft(topLeft);
        repositionChildren();
    }
private:
    TextWindow _caption;
    DropDownList _list;
};

class WindowDeviceContext : public DeviceContext
{
public:
    WindowDeviceContext(HWND hWnd) : _hWnd(hWnd)
    {
        _hdc = GetDC(_hWnd);
        IF_NULL_THROW(_hdc);
    }
    ~WindowDeviceContext() { ReleaseDC(_hWnd, _hdc); }
private:
    HWND _hWnd;
};

class OwnedDeviceContext : public DeviceContext
{
public:
    OwnedDeviceContext(HDC hdc)
    {
        _hdc = hdc;
        IF_NULL_THROW(_hdc);
    }
    ~OwnedDeviceContext() { DeleteDC(_hdc); }
};


class BitmapWindow : public WindowsWindow
{
public:
    BitmapWindow()
    {
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

    // Override this if you just want a simple bitmap that is updated from the
    // UI thread.
    virtual void draw2() { }

    // Override this to have more control, e.g. for a rendering from a
    // different thread.
    void draw()
    {
        _bitmap.ensure(innerSize());
        draw2();
        invalidate();
    }

    void innerSizeSet(Vector size)
    {
        if (!size.zeroArea())
            draw();
    }

    virtual LRESULT handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg) {
            case WM_PAINT:
                {
                    PaintHandle paintHandle(this);
                    if (paintHandle.zeroArea())
                        return 0;
                    if (!_bitmap.valid())
                        return 0;
                    Vector ptl = paintHandle.topLeft();
                    Vector pbr = paintHandle.bottomRight();
                    Vector br = _bitmap.size();
                    pbr = Vector(min(pbr.x, br.x), min(pbr.y, br.y));
                    Vector ps = pbr - ptl;
                    if (ps.x <= 0 || ps.y <= 0)
                        return 0;
                    Vector s = _bitmap.size();
                    _bmi.bmiHeader.biWidth =
                        _bitmap.stride() / sizeof(DWORD);
                    _bmi.bmiHeader.biHeight = -s.y;
                    paint();
                    IF_ZERO_THROW(SetDIBitsToDevice(
                        paintHandle,
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

    // Override this to restart animation only when painting actually happens.
    virtual void paint() { };

    Bitmap<DWORD> setNextBitmap(Bitmap<DWORD> nextBitmap)
    {
        Lock lock(&_mutex);
        _nextBitmap = nextBitmap;
        postMessage(WM_USER);
        invalidate();
        return _lastBitmap;
    }

    int stride() const { return _bitmap.stride(); }
    const Byte* data() const { return _bitmap.data(); }
    Byte* data() { return _bitmap.data(); }
    Bitmap<DWORD> bitmap() { return _bitmap; }
protected:
    Bitmap<DWORD> _bitmap;

private:
    Mutex _mutex;

    Bitmap<DWORD> _nextBitmap;
    Bitmap<DWORD> _lastBitmap;

    BITMAPINFO _bmi;
};

#endif // INCLUDED_USER_H
