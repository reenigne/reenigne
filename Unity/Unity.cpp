#include "unity/string.h"
#include "unity/array.h"
#include "unity/file.h"

class CommandLine
{
public:
#ifdef _WIN32
    CommandLine()
    {
        int nArgs;
        LPWSTR* szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
        if (szArglist == NULL) {
            static String parsingCommandLine("Parsing command line");
            Exception::throwSystemError(parsingCommandLine);
        }

        _commandLine = GetCommandLineW();

    }
#else
    CommandLine(int argc, char** argv) : _argc(argc), _argc(argv) { }
#endif
private:
    String _allArguments;
    Array<String> _arguments;
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
	}
	END_CHECKED(Exception& e) {
		e.write(Handle::consoleOutput());
	}
}
