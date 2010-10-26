#include "unity/string.h"
#include "unity/array.h"
#include "unity/file.h"

#ifdef _WIN32
#include "shellapi.h"
#endif

#include <stdio.h>

class CommandLine
{
public:
#ifdef _WIN32
    CommandLine()
    {
        WindowsCommandLine windowsCommandLine;
        int nArgs = windowsCommandLine.nArgs();
        const LPWSTR* szArglist = windowsCommandLine.arguments();
        _arguments.allocate(nArgs);
        int nBytes = 0;
        for (int i = 0; i < nArgs; ++i)
            nBytes += String::countBytes(szArglist[i]);
        static String commandLine("Command line");
        Reference<OwningBufferImplementation> bufferImplementation = new OwningBufferImplementation(commandLine);
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
        _nArguments = nArgs;
    }
#else
    CommandLine(int argc, char** argv)
    {
        for (int i = 0; i < argc; ++i) {
            _arguments[i] = String(argv[i]);
    }
#endif
    String argument(int i) const { return _arguments[i]; }
    int arguments() const { return _nArguments; }
private:
#ifdef _WIN32
    class WindowsCommandLine
    {
    public:
        WindowsCommandLine()
        {
            _szArglist = CommandLineToArgvW(GetCommandLineW(), &_nArgs);
            if (_szArglist == NULL) {
                static String parsingCommandLine("Parsing command line");
                Exception::throwSystemError(parsingCommandLine);
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
#endif
    Array<String> _arguments;
    int _nArguments;
};


#ifdef _WIN32
int main()
#else
int main(int argc, char* argv[])
#endif
{
	BEGIN_CHECKED {
#ifdef _WIN32
        CommandLine commandLine;
#else
        CommandLine commandLine(argc, argv);
#endif
        int n = commandLine.arguments();
        for (int i = 0; i < n; ++i) {
            printf("%i: ", i);
            commandLine.argument(i).write(Handle::consoleOutput());
            printf("\n");
        }
	}
	END_CHECKED(Exception& e) {
		e.write(Handle::consoleOutput());
	}
}
