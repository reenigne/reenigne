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
        CharacterSource o = *source;
        do {
            int c = o.get();
            if (c == ' ' || c == 10) {
                *source = o;
                continue;
            }
            if (parseComment(source))
                continue;
            return;
        } while (true);
    }
private:
    static bool parseComment(CharacterSource* source)
    {
        static String endOfFile("End of file in comment");
        static String printableCharacter("printable character");
        CharacterSource o = *source;
        int c = o.get();
        if (c != '/')
            return false;
        c = o.get();
        if (c == '/') {
            do {
                c = o.get();
                if (c == 10 || c == -1)
                    break;
                if (c < 0x20)
                    o.throwUnexpected(printableCharacter, String::hexadecimal(c, 2));
            } while (true);
            *source = o;
            return true;
        }
        if (c == '*') {
            do {
                if (parseComment(&o))
                    continue;
                c = o.get();
                if (c == -1)
                    o.throwError(endOfFile);
                if (c == '*') {
                    c = o.get();
                    if (c == -1)
                        o.throwError(endOfFile);
                    if (c == '/') {
                        *source = o;
                        return true;
                    }
                }
                if (c < 0x20 && c != 10)
                    o.throwUnexpected(printableCharacter, String::hexadecimal(c, 2));
            } while (true);
        }
        return false;
    }
};

template<class T> class ExpressionTemplate : public ReferenceCounted
{
public:
    static Reference<ExpressionTemplate> parse(CharacterSource* source)
    {
        Reference<Expression> e = Literal::parse(source);
        CharacterSource o;
        do {
            o = *source;
            int c = o.get();
            if (c == '+') {
                Space::parse(&o);
                Reference<Expression> e2 = Literal::parse(&o);
                if (!e2.valid()) {
                    static String literal("literal");
                    source->throwUnexpected(literal, String::codePoint(c));
                }
                e = new AddExpression(e, e2);
                *source = o;
            }
            else
                return e;
        } while (true);
    }
    virtual String output() const = 0;
};

typedef ExpressionTemplate<void> Expression;

template<class T> class LiteralTemplate : public Expression
{
public:
    static Reference<LiteralTemplate> parse(CharacterSource* source)
    {
        Reference<Literal> e = DoubleQuotedString::parse(source);
        if (e.valid())
            return e;
        e = Integer::parse(source);
        if (e.valid())
            return e;
        return 0;
    }
};

typedef LiteralTemplate<void> Literal;

class Integer : public Literal
{
public:
    static Reference<Integer> parse(CharacterSource* source)
    {
        CharacterSource o = *source;
        int n = 0;
        int c = o.get();
        if (c < '0' || c > '9')
            return 0;
        *source = o;
        do {
            n = n*10 + c - '0';
            c = o.get();
            if (c < '0' || c > '9') {
                Space::parse(source);
                return new Integer(n);
            }
            *source = o;
        } while (true);
    }
    String output() const
    {
        return String::decimal(_n);
    }
    int value() const { return _n; }
private:
    Integer(int n) : _n(n) { }
    int _n;
};

class DoubleQuotedString : public Literal
{
public:
    static Reference<DoubleQuotedString> parse(CharacterSource* source)
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
        CharacterSource o = *source;
        if (o.get() != '"')
            return 0;
        *source = o;
        int start = source->position();
        int end;
        String insert;
        int n;
        int nn;
        String string;
        do {
            o = *source;
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
                    string += source->subString(start, end);
                    Space::parse(source);
                    return new DoubleQuotedString(string);
                case '\\':
                    string += source->subString(start, end);
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
                            for (int i = 0; i < 4; ++i) {
                                o = *source;
                                nn = parseHexadecimalCharacter(source);
                                if (nn == -1)
                                    o.throwUnexpected(hexadecimalDigit, String::codePoint(source->get()));
                                n = (n << 4) | nn;
                            }
                            nn = parseHexadecimalCharacter(source);
                            if (nn != -1) {
                                n = (n << 4) | nn;
                                nn = parseHexadecimalCharacter(source);
                                if (nn != -1)
                                    n = (n << 4) | nn;
                            }
                            o = *source;
                            insert = String::codePoint(n);
                            break;
                        default:
                            o.throwUnexpected(escapedCharacter, String::codePoint(c));
                    }
                    string += insert;
                    start = source->position();
                    break;
            }
        } while (true);
    }
    String output() const { return _string; }
private:
    DoubleQuotedString(String string) : _string(string) { }

    static int parseHexadecimalCharacter(CharacterSource* source)
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

class AddExpression : public Expression
{
public:
    AddExpression(Reference<Expression> left, Reference<Expression> right) : _left(left), _right(right) { }
    String output() const
    {
        Reference<Integer> l = _left;
        Reference<Integer> r = _right;
        if (l.valid() && r.valid())
            return String::decimal(l->value() + r->value());
        return _left->output() + _right->output(); 
    }
private:
    Reference<Expression> _left;
    Reference<Expression> _right;
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
        Reference<Expression> program = Expression::parse(&source);
        if (!program.valid()) {
            static String error("Expected expression");
            source.throwError(error);
        }
        if (!source.empty()) {
            static String error("Expected end of file");
            source.throwError(error);
        }
        program->output().write(Handle::consoleOutput());
	}
	END_CHECKED(Exception& e) {
		e.write(Handle::consoleOutput());
	}
}
