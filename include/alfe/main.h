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

#include "alfe/integer_types.h"
#include "alfe/uncopyable.h"
#include "alfe/reference_counted.h"
#include "alfe/swap.h"
#include "alfe/array.h"
#include "alfe/minimum_maximum.h"
#include "alfe/string.h"
#include "alfe/exception.h"
#include "alfe/file.h"
#include "alfe/find_handle.h"
#include "alfe/handle.h"
#include "alfe/file_handle.h"
#include "alfe/character_source.h"
#if defined(_WIN32) && defined(_WINDOWS)
#include "alfe/vectors.h"
#include "alfe/colour_space.h"
#include "alfe/reference_counted_array.h"
#include "alfe/bitmap.h"
#include "alfe/user.h"
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
        BEGIN_CHECKED {
            BEGIN_CHECKED {
                _windows.initialize(hInst);
                _nCmdShow = nCmdShow;
                initializeWindowsCommandLine();
            }
            END_CHECKED(Exception& e) {
                NullTerminatedWideString s(e.message());
                MessageBox(NULL, s, L"Error", MB_OK | MB_ICONERROR);
            }
        }
        END_CHECKED(Exception&) {
            // Can't even display an error
        }
        return _returnValue;
    }
#else
    int initialize()
    {
        BEGIN_CHECKED {
            console = Handle(GetStdHandle(STD_OUTPUT_HANDLE), Console());
            if (!console.valid())
                throw Exception::systemError("Getting console handle");
            BEGIN_CHECKED {
                initializeWindowsCommandLine();
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
#endif
    virtual void run() = 0;
    Array<String> _arguments;
    int _returnValue;
private:
#ifdef _WIN32
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
