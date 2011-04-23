#ifndef INCLUDED_MAIN_H
#define INCLUDED_MAIN_H

#include "unity/string.h"
#include "unity/array.h"

#ifdef _WIN32
#include <windows.h>
#include "shellapi.h"
#endif

void displayError(const Exception& e)
{
#if defined(_WIN32) && defined(_WINDOWS)
    NullTerminatedWideString s(e.message());
    MessageBox(NULL, s, L"Error", MB_OK | MB_ICONERROR);
#else
    e.write(Handle::consoleOutput());
#endif
}

class ProgramBase
{
public:
#ifdef _WIN32
#ifdef _WINDOWS
    void initialize(HINSTANCE hInst)
    {
        BEGIN_CHECKED {
            _windows.initialize(hInst);
            initializeWindowsCommandLine();
        }
        END_CHECKED(Exception& e) {
            e.write(Handle::consoleOutput());
        }
    }
#else
    void initialize()
    {
        BEGIN_CHECKED {
            initializeWindowsCommandLine();
        }
        END_CHECKED(Exception& e) {
            e.write(Handle::consoleOutput());
        }
    }
#endif
#else
    void initialize(int argc, char* argv[])
    {
        BEGIN_CHECKED {
            _arguments.allocate(argc);
            for (int i = 0; i < argc; ++i) {
                _arguments[i] = String(argv[i]);
            run();
        }
        END_CHECKED(Exception& e) {
            e.write(Handle::consoleOutput());
        }
    }
#endif
protected:
#ifdef _WIN32
#ifdef _WINDOWS
    Windows _windows;
#endif
#endif
    Array<String> _arguments;
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
                if (_szArglist == NULL) {
                    static String parsingCommandLine("Parsing command line");
                    throw Exception::systemError(parsingCommandLine);
                }
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
            nBytes += String::countBytes(szArglist[i]);
        Reference<OwningBufferImplementation> bufferImplementation = new OwningBufferImplementation;
        bufferImplementation->allocate(nBytes);
        Buffer buffer(bufferImplementation);
        UInt8* p = bufferImplementation->data();
        int s = 0;
        for (int i = 0; i < nArgs; ++i) {
            UInt8* p2 = String::addToBuffer(szArglist[i], p);
            int n = p2 - p;
            p = p2;
            _arguments[i] = String(buffer, s, n);
            s += n;
        }
        run();
    }
#endif
};

#ifdef _WIN32
#ifdef _WINDOWS
INT APIENTRY WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT nCmdShow)
#else
int main()
#endif
#else
int main(int argc, char* argv[])
#endif
{
    Program program;
#ifdef _WIN32
#ifdef _WINDOWS
    program.initialize(hInst);
#else
    program.initialize();
#endif
#else
    program.initialize(argc, argv);
#endif
    return 0;
}

#endif // INCLUDED_MAIN_H
