#include "unity/string.h"
#include "unity/array.h"
#include "unity/file.h"

#ifdef _WIN32
#include "shellapi.h"
#endif

#include <stdio.h>
#include <stdlib.h>

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

class Space
{
public:
    static void parse(CharacterSource* source)
    {

    }
};

class DoubleQuotedString
{
public:
    DoubleQuotedString(CharacterSource* source)
    {
        static String endOfFile("End of file in string");
        static String endOfLine("End of line in string");
        static String printableCharacter("printable character");
        static String escapedCharacter("escaped character");
        static String hexadecimalDigit("hexadecimal digit");
        static String newLine = String::codePoint(10);
        static String tab = String::codePoint(9);
        static String backslash = String::codePoint('\\');
        static String doubleQuote = String::codePoint('"');
        static String dollar = String::codePoint('$');
        static String singleQuote = String::codePoint('\'');
        static String backQuote = String::codePoint('`');
        source->assert('"');
        int start = source->position();
        int end;
        static String insert;
        int n;
        int nn;
        do {
            CharacterSource o = *source;
            end = source->position();
            int c = source->get();
            if (c < 0x20) {
                if (c == 10)
                    o.throwError(endOfLine);
                if (c == -1)
                    o.throwError(endOfFile);
                o.throwUnexpected(printableCharacter, String::hexadecimal(c, 2));
            }
            switch (c) {
                case '"':
                    _string += source->subString(start, end);
                    return;
                case '\\':
                    _string += source->subString(start, end);
                    o = *source;
                    c = source->get();
                    if (c < 0x20) {
                        if (c == 10)
                            o.throwError(endOfLine);
                        if (c == -1)
                            o.throwError(endOfFile);
                        o.throwUnexpected(escapedCharacter, String::hexadecimal(c, 2));
                    }
                    switch (c) {
                        case 'n':
                            insert = newLine;
                            break;
                        case 't':
                            insert = tab;
                            break;
                        case '$':
                            insert = dollar;
                            break;
                        case '"':
                            insert = doubleQuote;
                            break;
                        case '\'':
                            insert = singleQuote;
                            break;
                        case '`':
                            insert = backQuote;
                            break;
                        case 'U':
                            source->assert('+');
                            n = 0;
                            o = *source;
                            nn = parseHexadecimalCharacter(source);
                            if (nn == -1)
                                o.throwUnexpected(hexadecimalDigit, String::codePoint(source->get()));
                            n = (n << 4) | nn;
                            nn = parseHexadecimalCharacter(source);
                            if (nn == -1)
                                o.throwUnexpected(hexadecimalDigit, String::codePoint(source->get()));
                            n = (n << 4) | nn;
                            nn = parseHexadecimalCharacter(source);
                            if (nn == -1)
                                o.throwUnexpected(hexadecimalDigit, String::codePoint(source->get()));
                            n = (n << 4) | nn;
                            nn = parseHexadecimalCharacter(source);
                            if (nn == -1)
                                o.throwUnexpected(hexadecimalDigit, String::codePoint(source->get()));
                            n = (n << 4) | nn;
                            nn = parseHexadecimalCharacter(source);
                            if (nn != -1) {
                                n = (n << 4) | nn;
                                nn = parseHexadecimalCharacter(source);
                                if (nn != -1)
                                    n = (n << 4) | nn;
                            }
                            insert = String::codePoint(n);
                            break;
                        default:
                            o.throwUnexpected(escapedCharacter, String::codePoint(c));
                    }
                    _string += insert;
                    start = source->position();
                    break;
            }
        } while (true);
    }
    void output()
    {
        _string.write(Handle::consoleOutput());
    }
private:
    int parseHexadecimalCharacter(CharacterSource* source)
    {
        CharacterSource o = *source;
        int c = o.get();
        if (c >= '0' && c <= '9') {
            *source = o;
            return c - '0';
        }
        if (c >= 'A' && c <= 'F') {
            *source = o;
            return c + 10 - 'A';
        }
        if (c >= 'a' && c <= 'f') {
            *source = o;
            return c + 10 - 'a';
        }
        return -1;
    }
    String _string;
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
        if (commandLine.arguments() < 2) {
            static String syntax1("Syntax: ");
            static String syntax2(" <input file name>\n");
            (syntax1 + commandLine.argument(0) + syntax2).write(Handle::consoleOutput());
            exit(1);
        }
		File file(commandLine.argument(1));
		String contents = file.contents();
        CharacterSource source = contents.start();
        DoubleQuotedString program(&source);
        if (!source.empty()) {
            static String error("Expected end of file");
            source.throwError(error);
        }
        program.output();
	}
	END_CHECKED(Exception& e) {
		e.write(Handle::consoleOutput());
	}
}
