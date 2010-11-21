#include "unity/string.h"
#include "unity/array.h"
#include "unity/file.h"
#include "unity/stack.h"
#include "unity/hashtable.h"
#include "Type.cpp"

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

template<class T> class ContextTemplate;

typedef ContextTemplate<void> Context;

class Function
{
public:
    virtual void call(Context* context) = 0;
    virtual Type returnType() const = 0;
};

class Value
{
public:
    String getString() { return _stringValue; }
private:
    int _intValue;
    String _stringValue;
    Function* _functionValue;
//    Type _type;
};

class Symbol : public ReferenceCounted
{
public:
    virtual Type type() const = 0;
};

class FunctionName : public Symbol
{
public:
    void addOverload(TypeList argumentTypes, Function* function)
    {
        if (_overloads.hasKey(argumentTypes)) {
            // TODO: Throw an error to say that this has already been defined.
        }
        _overloads.add(argumentTypes, function);
        if (_overloads.count() == 1)
            _functionType = FunctionType(function->returnType(), argumentTypes);

    }
    Function* lookUpOverload(TypeList argumentTypes)
    {
        if (!_overloads.hasKey(argumentTypes)) {
            // TODO: Throw an error to say that this overload is not available.
        }
        return _overloads.lookUp(argumentTypes);
    }
    Type type() const
    {
        if (_overloads.count() > 1) {
            // TODO: Throw an error to say that this is an overloaded function,
            // so the symbol itself does not have a definite type.
        }
        return _functionType;
    }
private:
    int _overloadCount;
    HashTable<TypeList, Function*> _overloads;
    Type _functionType;
};

class Variable : public Symbol
{
    // TODO
};

class Identifier;

template<class T> class ContextTemplate
{
public:
    CharacterSource getSource() const { return _source; }
    void setSource(const CharacterSource& source) { _source = source; }
    int get() { return _source.get(); }
    Value pop() { return _stack.pop(); }
    void addFunction(String name, TypeList argumentTypes, Function* function)
    {
        FunctionName* functionName;
        if (_symbolTable.hasKey(name)) {
            Reference<Symbol> symbol = _symbolTable.lookUp(name);
            functionName = dynamic_cast<FunctionName*>(static_cast<Symbol*>(symbol));
            if (functionName == 0) {
                static String error(" is already defined as a variable");
                _source.throwError(name + error);  // TODO: is this the right location?
            }
        }
        else {
            functionName = new FunctionName;
            _symbolTable.add(name, functionName);
        }
        functionName->addOverload(argumentTypes, function);
    }
    void addType(String name, Type type)
    {
        if (_typeTable.hasKey(name)) {
            static String error(" has already been defined.");
            _source.throwError(name + error);  // TODO: is this the right location?
        }
        _typeTable.add(name, type);
    }
    Function* resolveFunction(Reference<Identifier> identifier, TypeList typeList)
    {
        if (!_symbolTable.hasKey(identifier->name()))
            return 0;
        Reference<Symbol> symbol = _symbolTable.lookUp(identifier->name());
        FunctionName* functionName = dynamic_cast<FunctionName*>(static_cast<Symbol*>(symbol));
        if (functionName == 0) {
            static String error(" is not a function");
            _source.throwError(identifier->name() + error);  // TODO: is this the right location?
        }
        return functionName->lookUpOverload(typeList);
    }
    Reference<Symbol> resolveSymbol(String name)
    {
        if (!_symbolTable.hasKey(name))
            return Reference<Symbol>();
        return _symbolTable.lookUp(name);
    }
private:
    CharacterSource _source;
    HashTable<String, Reference<Symbol> > _symbolTable;
    HashTable<String, Type> _typeTable;
    Stack<Value> _stack;
};

typedef ContextTemplate<void> Context;

class PrintFunction : public Function
{
public:
    PrintFunction() : _consoleOutput(Handle::consoleOutput())
    { }
    void call(Context* context)
    {
        context->pop().getString().write(_consoleOutput);
    }
    Type returnType() const { return VoidType(); }
private:
    Handle _consoleOutput;
};

class Space
{
public:
    static void parse(Context* context)
    {
        do {
            CharacterSource s = context->getSource();
            int c = context->get();
            if (c == ' ' || c == 10)
                continue;
            if (parseComment(context))
                continue;
            context->setSource(s);
            return;
        } while (true);
    }
private:
    static bool parseComment(Context* context)
    {
        static String endOfFile("End of file in comment");
        static String printableCharacter("printable character");
        CharacterSource s = context->getSource();
        int c = context->get();
        if (c != '/') {
            context->setSource(s);
            return false;
        }
        c = context->get();
        if (c == '/') {
            do {
                s = context->getSource();
                c = context->get();
                if (c == 10 || c == -1)
                    break;
                if (c < 0x20)
                    s.throwUnexpected(printableCharacter, String::hexadecimal(c, 2));
            } while (true);
            return true;
        }
        if (c == '*') {
            do {
                if (parseComment(context))
                    continue;
                s = context->getSource();
                c = context->get();
                while (c == '*') {
                    s = context->getSource();
                    c = context->get();
                    if (c == '/')
                        return true;
                }
                if (c == -1)
                    s.throwError(endOfFile);
                if (c < 0x20 && c != 10)
                    s.throwUnexpected(printableCharacter, String::hexadecimal(c, 2));
            } while (true);
        }
        return false;
    }
};

template<class T> class ExpressionTemplate : public ReferenceCounted
{
public:
    static Reference<ExpressionTemplate> parse(Context* context)
    {
        Reference<Expression> e = parseComponent(context);
        do {
            int c = context->get();
            if (c == '+') {
                Space::parse(context);
                CharacterSource s = context->getSource();
                Reference<Expression> e2 = parseComponent(context);
                if (!e2.valid()) {
                    static String literal("literal or opening parenthesis");
                    s.throwUnexpected(literal, String::codePoint(c));
                }
                e = new AddExpression(e, e2);
            }
            else
                return e;
        } while (true);
    }
    static Reference<ExpressionTemplate> parseComponent(Context* context)
    {
        Reference<Expression> e = DoubleQuotedString::parse(context);
        if (e.valid())
            return e;
        e = Integer::parse(context);
        if (e.valid())
            return e;
        CharacterSource s = context->getSource();
        int c = context->get();
        if (c == '(') {
            Space::parse(context);
            e = parse(context);
            CharacterSource s = context->getSource();
            c = context->get();
            if (c != ')') {
                static String closingParenthesis("closing parenthesis");
                s.throwUnexpected(closingParenthesis, String::codePoint(c));
            }
            Space::parse(context);
            return e;
        }
        context->setSource(s);
        return 0;
    }
    virtual void compile(Context* context) = 0;
    virtual Type type() const = 0;
};

typedef ExpressionTemplate<void> Expression;

class Identifier : public Expression
{
public:
    static Reference<Identifier> parse(Context* context)
    {
        CharacterSource s = context->getSource();
        CharacterSource s2 = s;
        int start = s.position();
        int c = s.get();
        if (c < 'a' || c > 'z')
            return 0;
        s2 = s;
        do {
            c = s.get();
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
                s2 = s;
                continue;
            }
            break;
        } while (true);
        int end = s.position();
        context->setSource(s2);
        Space::parse(context);
        return new Identifier(s, s.subString(start, end));
    }
    Type type() const { return _symbol->type(); }
    String name() const { return _name; }
    void compile(Context* context)
    {
        _symbol = context->resolveSymbol(_name);
        if (!_symbol.valid()) {
            static String error("Undefined symbol ");
            _source.throwError(error + _name);
        }
    }
private:
    Identifier(CharacterSource source, String name)
      : _source(source), _name(name) 
    { }
    CharacterSource _source;
    String _name;
    Reference<Symbol> _symbol;
};

template<class T> class StatementTemplate : public ReferenceCounted
{
public:
    static Reference<StatementTemplate> parse(Context* context)
    {
        Reference<Statement> s = FunctionCallStatement::parse(context);
        if (s.valid())
            return s;
        return 0;
    }
    virtual void compile(Context* context) = 0;
    virtual void run(Context* context) = 0;
};

typedef StatementTemplate<void> Statement;

class StatementSequence : public ReferenceCounted
{
public:
    static Reference<StatementSequence> parse(Context* context)
    {
        Stack<Reference<Statement> > statements;
        do {
            Reference<Statement> statement = Statement::parse(context);
            if (!statement.valid())
                break;
            statements.push(statement);
        } while (true);
        Reference<StatementSequence> statementSequence = new StatementSequence;
        statements.toArray(&statementSequence->_statements);
        return statementSequence;
    }
    void compile(Context* context)
    {
        for (int i = 0; i < _statements.count(); ++i)
            _statements[i]->compile(context);
    }
    void run(Context* context)
    {
        for (int i = 0; i < _statements.count(); ++i)
            _statements[i]->run(context);
    }
private:
    StatementSequence() { }
    Array<Reference<Statement> > _statements;
};

class FunctionCallStatement : public Statement
{
public:
    static Reference<FunctionCallStatement> parse(Context* context)
    {
        CharacterSource s = context->getSource();
        Reference<Identifier> functionName = Identifier::parse(context);
        if (!functionName.valid())
            return 0;
        int c = context->get();
        if (c != '(') {
            context->setSource(s);
            return 0;
        }
        Space::parse(context);
        int n = 0;
        c = context->get();
        Stack<Reference<Expression> > stack;
        while (c != ')') {
            Reference<Expression> e = Expression::parse(context);
            if (!e.valid()) {
                static String expression("Expected expression");
                context->getSource().throwError(expression);
            }
            stack.push(e);
            s = context->getSource();
            c = context->get();
            if (c != ',' && c != ')') {
                static String commaOrCloseParentheses(", or )");
                s.throwUnexpected(commaOrCloseParentheses, String::codePoint(c));
            }
            Space::parse(context);
        } while (true);
        context->getSource().assert(';');
        Reference<FunctionCallStatement> functionCall = new FunctionCallStatement(functionName, n);
        stack.toArray(&functionCall->_arguments);
        return functionCall;
    }
    void run(Context* context)
    {
        _function->call(context);
    }
    void compile(Context* context)
    {
        TypeList argumentTypes;
        for (int i = 0; i < _arguments.count(); ++i) {
            _arguments[i]->compile(context);
            argumentTypes.push(_arguments[i]->type());
        }
        argumentTypes.finalize();
        _function = context->resolveFunction(_functionName, argumentTypes);
    }
private:
    FunctionCallStatement(Reference<Identifier> functionName, int n) : _functionName(functionName)
    {
        _arguments.allocate(n);
    }

    Reference<Identifier> _functionName;
    Function* _function;
    Array<Reference<Expression> > _arguments;
};

class Integer : public Expression
{
public:
    static Reference<Integer> parse(Context* context)
    {
        CharacterSource s = context->getSource();
        int n = 0;
        int c = context->get();
        if (c < '0' || c > '9') {
            context->setSource(s);
            return 0;
        }
        do {
            n = n*10 + c - '0';
            s = context->getSource();
            c = s.get();
            if (c < '0' || c > '9') {
                context->setSource(s);
                Space::parse(context);
                return new Integer(n);
            }
        } while (true);
    }
    void compile(Context* context) { }
    Type type() const { return IntType(); }
private:
    Integer(int n) : _n(n) { }
    int _n;
};

class DoubleQuotedString : public Expression
{
public:
    static Reference<DoubleQuotedString> parse(Context* context)
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
        CharacterSource s = context->getSource();
        if (context->get() != '"') {
            context->setSource(s);
            return 0;
        }
        int start = context->getSource().position();
        int end;
        String insert;
        int n;
        int nn;
        String string;
        do {
            s = context->getSource();
            end = s.position();
            int c = context->get();
            if (c < 0x20) {
                if (c == 10)
                    s.throwError(endOfLine);
                if (c == -1)
                    s.throwError(endOfFile);
                s.throwUnexpected(printableCharacter, String::hexadecimal(c, 2));
            }
            switch (c) {
                case '"':
                    string += context->getSource().subString(start, end);
                    Space::parse(context);
                    return new DoubleQuotedString(string);
                case '\\':
                    s = context->getSource();
                    string += s.subString(start, end);
                    c = context->get();
                    if (c < 0x20) {
                        if (c == 10)
                            s.throwError(endOfLine);
                        if (c == -1)
                            s.throwError(endOfFile);
                        s.throwUnexpected(escapedCharacter, String::hexadecimal(c, 2));
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
                            context->getSource().assert('+');
                            n = 0;
                            for (int i = 0; i < 4; ++i) {
                                s = context->getSource();
                                nn = parseHexadecimalCharacter(context);
                                if (nn == -1)
                                    s.throwUnexpected(hexadecimalDigit, String::codePoint(context->get()));
                                n = (n << 4) | nn;
                            }
                            s = context->getSource();
                            nn = parseHexadecimalCharacter(context);
                            if (nn != -1) {
                                n = (n << 4) | nn;
                                s = context->getSource();
                                nn = parseHexadecimalCharacter(context);
                                if (nn != -1)
                                    n = (n << 4) | nn;
                                else
                                    context->setSource(s);
                            }
                            else
                                context->setSource(s);
                            insert = String::codePoint(n);
                            break;
                        default:
                            s.throwUnexpected(escapedCharacter, String::codePoint(c));
                    }
                    string += insert;
                    start = context->getSource().position();
                    break;
            }
        } while (true);
    }
    void compile(Context* context) { }
    Type type() const { return StringType(); }
private:
    DoubleQuotedString(String string) : _string(string) { }

    static int parseHexadecimalCharacter(Context* context)
    {
        CharacterSource s = context->getSource();
        int c = context->get();
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'A' && c <= 'F')
            return c + 10 - 'A';
        if (c >= 'a' && c <= 'f')
            return c + 10 - 'a';
        context->setSource(s);
        return -1;
    }
    String _string;
};

class AddExpression : public Expression
{
public:
    AddExpression(Reference<Expression> left, Reference<Expression> right) : _left(left), _right(right) { }
    Type type() const
    {
        if (_left->type() == StringType())
            if (_right->type() == IntType() || _right->type() == StringType())
                return StringType();
        if (_right->type() == StringType())
            if (_left->type() == IntType())
                return StringType();
        if (_left->type() == IntType() && _right->type() == IntType())
            return IntType();
        // TODO: error
    }
    void compile(Context* context)
    {
        _left->compile(context);
        _right->compile(context);
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
        Context context;

        context.addType(String("String"), StringType());
        context.addType(String("Void"), VoidType());

        PrintFunction print;
        TypeList printArgumentTypes;
        printArgumentTypes.push(StringType());
        printArgumentTypes.finalize();
        Type printFunctionType = FunctionType(VoidType(), printArgumentTypes);
        context.addFunction(String("print"), printArgumentTypes, &print);

        CharacterSource source(contents, file.path());
        context.setSource(source);
        Space::parse(&context);
        Reference<StatementSequence> program = StatementSequence::parse(&context);
        if (!context.getSource().empty()) {
            static String error("Expected end of file");
            source.throwError(error);
        }

        program->compile(&context);

        program->run(&context);
    }
    END_CHECKED(Exception& e) {
        e.write(Handle::consoleOutput());
    }
}
