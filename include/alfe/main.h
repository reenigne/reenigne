#ifndef INCLUDED_MAIN_H
#define INCLUDED_MAIN_H

#include <new>
#include <exception>
#include <limits>
#include <string.h>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifdef _WINDOWS
#define UTF16_MESSAGES
#endif
#include "shellapi.h"
#else
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#endif

#define _USE_MATH_DEFINES
#include <math.h>

bool alerting = false;

#include "alfe/integer_types.h"
#include "alfe/uncopyable.h"
#include "alfe/reference.h"
#include "alfe/swap.h"
#include "alfe/array.h"
#include "alfe/minimum_maximum.h"
#include "alfe/string.h"
#include "alfe/exception.h"
#include "alfe/file.h"
#include "alfe/find_handle.h"
#include "alfe/circular_buffer.h"
#include "alfe/handle.h"
#include "alfe/file_handle.h"
#include "alfe/character_source.h"
#if defined(_WIN32) && defined(_WINDOWS)
#include "alfe/vectors.h"
#include "alfe/colour_space.h"
#include "alfe/reference_counted_array.h"
#include "alfe/bitmap.h"
#include "alfe/user.h"

class IdleProcessor
{
public:
    virtual void idle() = 0;
};

#endif

Handle console;

class ProgramBase : public Uncopyable
{
public:
    ProgramBase() : _returnValue(0) { }
#ifdef _WIN32
#ifdef _WINDOWS
    int initialize(HINSTANCE hInst, INT nCmdShow)
    {
        _hInst = hInst;
        _nCmdShow = nCmdShow;
        initializeMain();
        return _returnValue;
    }
#else
    int initialize()
    {
        initializeMain();
        return _returnValue;
    }
#endif
#else
    int initialize(int argc, char* argv[])
    {
        BEGIN_CHECKED {
            console = Handle(STDOUT_FILENO, Console());
            BEGIN_CHECKED {
                _arguments.allocate(argc);
                for (int i = 0; i < argc; ++i) {
                    _arguments[i] = String(argv[i]);
                run();
            }
            END_CHECKED(Exception& e) {
                console.write(e);
            }
        }
        END_CHECKED(Exception&) {
            // Can't even display an error
        }
        return _returnValue;
    }
#endif
protected:
#if defined(_WIN32) && defined(_WINDOWS)
    Windows _windows;
    INT _nCmdShow;

    void pumpMessages()
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
                _returnValue = static_cast<int>(msg.wParam);
        } while (bRet != 0);
    }

    void pumpMessages(IdleProcessor* idle)
    {
        BOOL fMessage;
        MSG msg;
        do {
            do {
                fMessage = PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE);
                if (fMessage == 0)
                    break;
                if (msg.message == WM_QUIT)
                    break;
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            } while (true);
            if (msg.message == WM_QUIT)
                break;
            idle->idle();
        } while (true);
        _returnValue = static_cast<int>(msg.wParam);
    }

#endif
    virtual void run() = 0;
    Array<String> _arguments;
    int _returnValue;
private:
#ifdef _WIN32
#ifdef _WINDOWS
    HINSTANCE _hInst;

    void initializeWindows()
    {
        BEGIN_CHECKED {
            _windows.initialize(_hInst);
            initializeWindowsCommandLine();
            _windows.check();
        }
        END_CHECKED(Exception& e) {
            NullTerminatedWideString s(e.message());
            MessageBox(NULL, s, L"Error", MB_OK | MB_ICONERROR);
        }
    }

#endif
    void initializeMain()
    {
        // Disable exception swallowing on 64-bit Windows.
        typedef BOOL (WINAPI* getType)(LPDWORD lpFlags);
        typedef BOOL (WINAPI* setType)(DWORD dwFlags);
#ifndef PROCESS_CALLBACK_FILTER_ENABLED
#define PROCESS_CALLBACK_FILTER_ENABLED 1
#endif
        HMODULE kernel32 = LoadLibraryA("kernel32.dll");
        if (kernel32 != NULL) {
            getType getProcessUserModeExceptionPolicy =
                reinterpret_cast<getType>(GetProcAddress(kernel32,
                    "GetProcessUserModeExceptionPolicy"));
            setType setProcessUserModeExceptionPolicy =
                reinterpret_cast<setType>(GetProcAddress(kernel32,
                    "SetProcessUserModeExceptionPolicy"));
            if (getProcessUserModeExceptionPolicy != 0 &&
                setProcessUserModeExceptionPolicy != 0) {
                DWORD dwFlags;
                if (getProcessUserModeExceptionPolicy(&dwFlags))
                    setProcessUserModeExceptionPolicy(dwFlags &
                        ~PROCESS_CALLBACK_FILTER_ENABLED);
            }
        }

        BEGIN_CHECKED {
            console = Handle(GetStdHandle(STD_OUTPUT_HANDLE), Console());
            if (!console.valid())
                throw Exception::systemError("Getting console handle");
            BEGIN_CHECKED {
#ifdef _WINDOWS
                initializeWindows();
#else
                initializeWindowsCommandLine();
#endif
            }
            END_CHECKED(Exception& e) {
                console.write(e);
            }
        }
        END_CHECKED(Exception&) {
            // Can't even display an error
        }
    }

    void initializeWindowsCommandLine()
    {
        class WindowsCommandLine
        {
        public:
            WindowsCommandLine()
            {
                _szArglist = CommandLineToArgvW(GetCommandLineW(), &_nArgs);
                if (_szArglist == NULL)
                    throw Exception::systemError("Parsing command line");
            }
            ~WindowsCommandLine()
            {
                LocalFree(static_cast<HLOCAL>(_szArglist));
            }
            const LPWSTR* arguments() const { return _szArglist; }
            int nArgs() const { return _nArgs; }
        private:
            LPWSTR* _szArglist;
            int _nArgs;
        };
        WindowsCommandLine windowsCommandLine;
        int nArgs = windowsCommandLine.nArgs();
        const LPWSTR* szArglist = windowsCommandLine.arguments();
        _arguments.allocate(nArgs);
        int nBytes = 0;
        for (int i = 0; i < nArgs; ++i)
            nBytes += String::bytes(szArglist[i]);
        String buffer(nBytes);
        int s = 0;
        for (int i = 0; i < nArgs; ++i) {
            buffer += szArglist[i];
            int n = buffer.length() - s;
            _arguments[i] = buffer.subString(s, n);
            s += n;
        }
        run();
    }
#endif
};

// Put Program in a template because it's not declared yet.
#ifdef _WIN32
#ifdef _WINDOWS
template<class T> INT APIENTRY WinMainTemplate(HINSTANCE hInst, INT nCmdShow)
#else
template<class T> int mainTemplate()
#endif
#else
template<class T> int mainTemplate(int argc, char* argv[])
#endif
{
    Program program;
#ifdef _WIN32
#ifdef _WINDOWS
    return program.initialize(hInst, nCmdShow);
#else
    return program.initialize();
#endif
#else
    return program.initialize(argc, argv);
#endif
}

#ifdef _WIN32
#ifdef _WINDOWS
INT APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT nCmdShow)
{
    return WinMainTemplate<void>(hInst, nCmdShow);
}

// Define main() as well in case we want to make a Windows program linked as
// a Console subsystem program (for debugging purposes).
int __cdecl main()
{
    return WinMainTemplate<void>(GetModuleHandle(NULL), SW_SHOWNORMAL);
}

#else
int main()
{
    return mainTemplate<void>();
}
#endif
#else
int main(int argc, char* argv[])
{
    return mainTemplate<void>(argc, argc);
}
#endif


#endif // INCLUDED_MAIN_H
