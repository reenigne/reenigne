#ifndef INCLUDED_USER_H
#define INCLUDED_USER_H

// TODO: Xlib port

#include "unity/string.h"
#include <windows.h>

class Windows : Uncopyable
{
    friend class Window;
public:
    Windows(HINSTANCE hInst) : _hInst(hInst)
    {
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

    ~Windows() { UnregisterClass(className(), _hInst); }

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

#endif // INCLUDED_USER_H
