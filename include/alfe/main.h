#ifndef INCLUDED_MAIN_H
#define INCLUDED_MAIN_H

#include <new>
#include <exception>
#include <limits>
#include <utility>
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
static const double tau = 2*M_PI;


bool alerting = false;

#include "alfe/integer_types.h"
#include "alfe/uncopyable.h"
#include "alfe/hash.h"
#include "alfe/handle.h"
#include "alfe/swap.h"
#include "alfe/array.h"
#include "alfe/minimum_maximum.h"
#include "alfe/string.h"
#include "alfe/exception.h"
#include "alfe/file.h"
#include "alfe/find_handle.h"
#include "alfe/circular_buffer.h"
#include "alfe/stream.h"
#include "alfe/file_stream.h"
#include "alfe/character_source.h"
#include "alfe/thread.h"
#if defined(_WIN32) && defined(_WINDOWS)
#include "alfe/vectors.h"
#include "alfe/colour_space.h"
#include "alfe/bitmap.h"
#include "alfe/linked_list.h"
#include "alfe/thread.h"
#include "alfe/user.h"
#endif

Stream console;

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
            console = Stream(STDOUT_FILENO, Console());
            BEGIN_CHECKED {
                _arguments.allocate(argc);
                for (int i = 0; i < argc; ++i)
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

#endif
    virtual void run() = 0;
    Array<String> _arguments;
    int _returnValue;
    HINSTANCE _hInst;
private:
#ifdef _WIN32
#ifdef _WINDOWS

    void initializeWindows()
    {
        BEGIN_CHECKED {
            initializeWindowsCommandLine();
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
            console = Stream(GetStdHandle(STD_OUTPUT_HANDLE), Console());
            // We can't validate console here because we might be in a GUI
            // program where there is no console.
            //if (!console.valid())
            //    throw Exception::systemError("Getting console handle");
            BEGIN_CHECKED {
#ifdef _WINDOWS
                initializeWindows();
#else
                initializeWindowsCommandLine();
#endif
            }
            END_CHECKED(Exception& e) {
                if (console.valid())
                    console.write(e);
            }
        }
        END_CHECKED(...) {
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
        int start = 0;
        Byte* p = buffer.data();
        for (int i = 0; i < nArgs; ++i) {
            p = String::write(p, szArglist[i]);
            int end = static_cast<int>(p - buffer.data());
            _arguments[i] = buffer.subString(start, end - start);
            start = end;
        }
        run();
    }
#endif
};

#ifdef _WIN32
template<class WindowClass> class WindowProgram : public ProgramBase
{
public:
    WindowProgram() : _quitting(false) { }
    void createWindow()
    {
        _window.setParent(0);
        _window.setWindows(&_windows);
        _window.create();
        _window.show(_nCmdShow);
    }
    void pumpMessages()
    {
        if (_quitting)
            return;
        MSG msg;
        do {
            BOOL fMessage = PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE);
            if (fMessage == 0)
                break;
            if (msg.message == WM_QUIT)
                break;
            if (!IsDialogMessage(_window.hWnd(), &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            _windows.check();
        } while (true);
        if (msg.message == WM_QUIT) {
            _quitting = true;
            _returnValue = static_cast<int>(msg.wParam);
        }
    }
    void run()
    {
        createWindow();
        do {
            bool more = idle();
            if (!more) {
                HANDLE handle = _interruptMessageLoop;
                DWORD r = MsgWaitForMultipleObjects(1, &handle, FALSE,
                    INFINITE, QS_ALLINPUT);
                IF_FALSE_THROW(r != WAIT_FAILED);
            }
            pumpMessages();
        } while (!_quitting);
    }
protected:
    // idle() returns true if there is more idle processing to do, false if
    // there isn't and we should wait for a message before calling again.
    virtual bool idle() { return false; }

    bool _quitting;
    Event _interruptMessageLoop;
    WindowClass _window;
};
#endif

// Put Program in a template because it's not declared yet.
#ifdef _WIN32
#ifdef _WINDOWS
template<class Program> INT APIENTRY WinMainTemplate(HINSTANCE hInst,
    INT nCmdShow)
#else
template<class Program> int mainTemplate()
#endif
#else
template<class Program> int mainTemplate(int argc, char* argv[])
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

class Program;

#ifdef _WIN32
#ifdef _WINDOWS
INT APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT nCmdShow)
{
    return WinMainTemplate<Program>(hInst, nCmdShow);
}

// Define main() as well in case we want to make a Windows program linked as
// a Console subsystem program (for debugging purposes).
int __cdecl main()
{
    return WinMainTemplate<Program>(GetModuleHandle(NULL), SW_SHOWNORMAL);
}

#else
int main()
{
    return mainTemplate<Program>();
}
#endif
#else
int main(int argc, char* argv[])
{
    return mainTemplate<Program>(argc, argv);
}
#endif


#endif // INCLUDED_MAIN_H
