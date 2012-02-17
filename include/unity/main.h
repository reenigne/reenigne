#ifndef INCLUDED_MAIN_H
#define INCLUDED_MAIN_H

#include "unity/string.h"
#include "unity/array.h"
#include "unity/user.h"

#ifdef _WIN32
#include <windows.h>
#include "shellapi.h"
#endif

Handle* debug;

class ProgramBase : public Uncopyable
{
public:
#ifdef _WIN32
#ifdef _WINDOWS
    int initialize(HINSTANCE hInst, INT nCmdShow)
    {
        BEGIN_CHECKED {
            BEGIN_CHECKED {
                _windows.initialize(hInst);
                _nCmdShow = nCmdShow;
                return initializeWindowsCommandLine();
            }
            END_CHECKED(Exception& e) {
                NullTerminatedWideString s(e.message());
                MessageBox(NULL, s, L"Error", MB_OK | MB_ICONERROR);
            }
        }
        END_CHECKED(Exception&) {
            // Can't even display an error
        }
        return 0;
    }
#else
    void initialize()
    {
        BEGIN_CHECKED {
            HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
            if (h == INVALID_HANDLE_VALUE || h == NULL)
                throw Exception::systemError("Getting console handle");
            _console.set(h, "console");
            debug = &_console;
            BEGIN_CHECKED {
                initializeWindowsCommandLine();
            }
            END_CHECKED(Exception& e) {
                _console.write(e);
            }
        }
        END_CHECKED(Exception&) {
            // Can't even display an error
        }
    }
#endif
#else
    void initialize(int argc, char* argv[])
    {
        BEGIN_CHECKED {
            _console.set(STDOUT_FILENO, "console");
            debug = &_console;
            BEGIN_CHECKED {
                _arguments.allocate(argc);
                for (int i = 0; i < argc; ++i) {
                    _arguments[i] = String(argv[i]);
                run();
            }
            END_CHECKED(Exception& e) {
                _console.write(e);
            }
        }
        END_CHECKED(Exception&) {
            // Can't even display an error
        }
    }
#endif
protected:
#if defined(_WIN32) && defined(_WINDOWS)
    Windows _windows;
    INT _nCmdShow;
    virtual int run() = 0;
#else
    virtual void run() = 0;
    Handle _console;
#endif
    Array<String> _arguments;
private:
#ifdef _WIN32
#ifdef _WINDOWS
    int initializeWindowsCommandLine()
#else
    void initializeWindowsCommandLine()
#endif
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
#ifdef _WINDOWS
        return run();
#else
        run();
#endif
    }
#endif
};

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
    program.initialize();
    return 0;
#endif
#else
    program.initialize(argc, argv);
    return 0;
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
