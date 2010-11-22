#include "unity/string.h"
#include "unity/character_source.h"
#include "unity/array.h"
#include "unity/file.h"
#include "unity/stack.h"
#include "unity/hashtable.h"

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

class Space
{
public:
    static void parse(CharacterSource* source, Context* context)
    {
        do {
            CharacterSource s = *source;
            int c = s.get();
            if (c == ' ' || c == 10) {
                *source = s;
                continue;
            }
            if (parseComment(source, context))
                continue;
            return;
        } while (true);
    }
private:
    static bool parseComment(CharacterSource* source, Context* context)
    {
        static String endOfFile("End of file in comment");
        static String printableCharacter("printable character");
        CharacterSource s = *source;
        int c = s.get();
        if (c != '/')
            return false;
        c = s.get();
        if (c == '/') {
            do {
                *source = s;
                c = s.get();
                if (c == 10 || c == -1)
                    break;
                if (c < 0x20)
                    source->throwUnexpected(printableCharacter, String::hexadecimal(c, 2));
            } while (true);
            *source = s;
            return true;
        }
        if (c == '*') {
            do {
                if (parseComment(&s, context))
                    continue;
                *source = s;
                c = s.get();
                while (c == '*') {
                    c = s.get();
                    if (c == '/') {
                        *source = s;
                        return true;
                    }
                }
                if (c == -1)
                    source->location().throwError(endOfFile);
                if (c < 0x20 && c != 10)
                    source->throwUnexpected(printableCharacter, String::hexadecimal(c, 2));
            } while (true);
        }
        return false;
    }
};

template<class T> class ContextTemplate;

typedef ContextTemplate<void> Context;

#include "Type.cpp"

class Function
{
public:
    virtual void call(Context* context) = 0;
    virtual Type returnType() const = 0;
};

class Value
{
public:
    Value() { }
    Value(int intValue) : _intValue(intValue) { }
    Value(String stringValue) : _stringValue(stringValue) { }
    int getInt() const { return _intValue; }
    String getString() const { return _stringValue; }
private:
    int _intValue;
    String _stringValue;
};

class Symbol : public ReferenceCounted
{
public:
    virtual Type type(String name, DiagnosticLocation location) const = 0;
    virtual Value value() const = 0;
};

class FunctionName : public Symbol
{
public:
    void addOverload(TypeList argumentTypes, Function* function, DiagnosticLocation location)
    {                                                             
        if (_overloads.hasKey(argumentTypes)) {
            static String error("This overload has already been defined.");
            location.throwError(error);
        }
        _overloads.add(argumentTypes, function);
        if (_overloads.count() == 1)
            _functionType = FunctionType(function->returnType(), argumentTypes);
    }
    bool hasOverload(TypeList argumentTypes)
    {
        return _overloads.hasKey(argumentTypes);
    }
    Function* lookUpOverload(TypeList argumentTypes)
    {
        return _overloads.lookUp(argumentTypes);
    }
    Type type(String name, DiagnosticLocation location) const
    {
        if (_overloads.count() > 1) {
            static String error(" is an overloaded function - I don't know which overload you mean.");
            location.throwError(name + error);
        }
        return _functionType;
    }
    Value value() const { return Value(); }
private:
    int _overloadCount;
    HashTable<TypeList, Function*> _overloads;
    Type _functionType;
};

class Variable : public Symbol
{
public:
    Variable(Type type) : _type(type) { }
    Type type(String name, DiagnosticLocation location) const { return _type; }
    Value value() const { return _value; }
    void setValue(Value value) { _value = value; }
private:
    Type _type;
    Value _value;
};

class Identifier;

template<class T> class ContextTemplate
{
public:
    void push(Value value) { _stack.push(value); }
    Value pop() { return _stack.pop(); }
    void addFunction(String name, TypeList argumentTypes, Function* function, DiagnosticLocation location)
    {
        FunctionName* functionName;
        if (_symbolTable.hasKey(name)) {
            Reference<Symbol> symbol = _symbolTable.lookUp(name);
            functionName = dynamic_cast<FunctionName*>(static_cast<Symbol*>(symbol));
            if (functionName == 0) {
                static String error(" is already defined as a variable");
                location.throwError(name + error);
            }
        }
        else {
            functionName = new FunctionName;
            _symbolTable.add(name, functionName);
        }
        functionName->addOverload(argumentTypes, function, location);
    }
    void addType(String name, Type type, DiagnosticLocation location)
    {
        if (_typeTable.hasKey(name)) {
            static String error(" has already been defined.");
            location.throwError(name + error);
        }
        _typeTable.add(name, type);
    }
    void addVariable(String name, Type type, DiagnosticLocation location)
    {
        if (_symbolTable.hasKey(name)) {
            static String error(" is already defined");
            location.throwError(name + error);
        }
        _symbolTable.add(name, new Variable(type));
    }
    Function* resolveFunction(Reference<Identifier> identifier, TypeList typeList, DiagnosticLocation location)
    {
        if (!_symbolTable.hasKey(identifier->name()))
            return 0;
        Reference<Symbol> symbol = _symbolTable.lookUp(identifier->name());
        FunctionName* functionName = dynamic_cast<FunctionName*>(static_cast<Symbol*>(symbol));
        if (functionName == 0) {
            static String error(" is not a function");
            location.throwError(identifier->name() + error);
        }
        if (!functionName->hasOverload(typeList)) {
            static String error(" has no overload with argument types ");
            location.throwError(identifier->name() + error + typeList.toString());
        }
        return functionName->lookUpOverload(typeList);
    }
    Reference<Symbol> resolveSymbol(String name, DiagnosticLocation location)
    {
        if (!_symbolTable.hasKey(name)) {
            static String error("Undefined symbol ");
            location.throwError(error + name);
        }
        return _symbolTable.lookUp(name);
    }
private:
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

template<class T> class ExpressionTemplate : public ReferenceCounted
{
public:
    static Reference<ExpressionTemplate> parse(CharacterSource* source, Context* context)
    {
        Reference<Expression> e = parseComponent(source, context);
        do {
            CharacterSource s = *source;
            int c = s.get();
            if (c == '+') {
                *source = s;
                Space::parse(source, context);
                Reference<Expression> e2 = parseComponent(source, context);
                if (!e2.valid()) {
                    static String literal("literal or opening parenthesis");
                    source->throwUnexpected(literal, String::codePoint(c));
                }
                e = new AddExpression(e, e2);
            }
            else
                return e;
        } while (true);
    }
    static Reference<ExpressionTemplate> parseComponent(CharacterSource* source, Context* context)
    {
        Reference<Expression> e = DoubleQuotedString::parse(source, context);
        if (e.valid())
            return e;
        e = Integer::parse(source, context);
        if (e.valid())
            return e;
        CharacterSource s = *source;
        int c = s.get();
        if (c == '(') {
            *source = s;
            Space::parse(source, context);
            e = parse(source, context);
            c = s.get();
            if (c != ')') {
                static String closingParenthesis("closing parenthesis");
                source->throwUnexpected(closingParenthesis, String::codePoint(c));
            }
            *source = s;
            Space::parse(source, context);
            return e;
        }
        return 0;
    }
    virtual void compile(Context* context) = 0;
    virtual Type type() const = 0;
    virtual void push(Context* context) = 0;
};

typedef ExpressionTemplate<void> Expression;

class Identifier : public Expression
{
public:
    static Reference<Identifier> parse(CharacterSource* source, Context* context)
    {
        CharacterSource s = *source;
        DiagnosticLocation location = s.location();
        int start = s.offset();
        int c = s.get();
        if (c < 'a' || c > 'z')
            return 0;
        do {
            *source = s;
            c = s.get();
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
                continue;
            break;
        } while (true);
        int end = source->offset();
        Space::parse(source, context);
        return new Identifier(s.subString(start, end), location);
    }
    Type type() const { return _symbol->type(_name, _location); }
    String name() const { return _name; }
    void push(Context* context)
    {
        context->push(_symbol->value());
    }
    void compile(Context* context)
    {
        _symbol = context->resolveSymbol(_name, _location);
    }
private:
    Identifier(String name, DiagnosticLocation location)
      : _name(name), _location(location)
    { }
    String _name;
    Reference<Symbol> _symbol;
    DiagnosticLocation _location;
};

template<class T> class StatementTemplate : public ReferenceCounted
{
public:
    static Reference<StatementTemplate> parse(CharacterSource* source, Context* context)
    {
        Reference<Statement> s = FunctionCallStatement::parse(source, context);
        if (s.valid())
            return s;
        s = VariableDeclarationStatement::parse(source, context);
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
    static Reference<StatementSequence> parse(CharacterSource* source, Context* context)
    {
        Stack<Reference<Statement> > statements;
        do {
            Reference<Statement> statement = Statement::parse(source, context);
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
    static Reference<FunctionCallStatement> parse(CharacterSource* source, Context* context)
    {
        CharacterSource s = *source;
        DiagnosticLocation location = s.location();
        Reference<Identifier> functionName = Identifier::parse(&s, context);
        if (!functionName.valid())
            return 0;
        int c = s.get();
        if (c != '(')
            return 0;
        Space::parse(&s, context);
        int n = 0;
        *source = s;
        c = s.get();
        Stack<Reference<Expression> > stack;
        while (c != ')') {
            Reference<Expression> e = Expression::parse(source, context);
            if (!e.valid()) {
                static String expression("Expected expression");
                source->location().throwError(expression);
            }
            stack.push(e);
            s = *source;
            c = s.get();
            if (c != ',' && c != ')') {
                static String commaOrCloseParentheses(", or )");
                source->throwUnexpected(commaOrCloseParentheses, String::codePoint(c));
            }
            *source = s;
            Space::parse(source, context);
        } 
        source->assert(';');
        Space::parse(source, context);
        Reference<FunctionCallStatement> functionCall = new FunctionCallStatement(functionName, n, location);
        stack.toArray(&functionCall->_arguments);
        return functionCall;
    }
    void run(Context* context)
    {
        for (int i = 0; i < _arguments.count(); ++i)
            _arguments[i]->push(context);
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
        _function = context->resolveFunction(_functionName, argumentTypes, _location);
    }
private:
    FunctionCallStatement(Reference<Identifier> functionName, int n, DiagnosticLocation location)
      : _functionName(functionName), _location(location)
    {
        _arguments.allocate(n);
    }

    Reference<Identifier> _functionName;
    Function* _function;
    Array<Reference<Expression> > _arguments;
    DiagnosticLocation _location;
};

class VariableDeclarationStatement : public Statement
{
public:
    static Reference<VariableDeclarationStatement> parse(CharacterSource* source, Context* context)
    {
        DiagnosticLocation location = source->location();
        CharacterSource s = *source;
        Type type = Type::parse(&s, context);
        if (!type.valid())
            return 0;
        Reference<Identifier> identifier = Identifier::parse(&s, context);
        if (!identifier.valid())
            return 0;
        int c = s.get();
        if (c != '=')
            return 0;
        Space::parse(&s, context);
        *source = s;
        Reference<Expression> e = Expression::parse(source, context);
        if (!e.valid()) {
            static String expression("Expected expression");
            source->location().throwError(expression);
        }
        source->assert(';');
        Space::parse(source, context);
        return new VariableDeclarationStatement(type, identifier, e, location);
    }
    void compile(Context* context)
    {
        _variable = context->addVariable(_identifier->name(), _type, _location);
    }
    void run(Context* context)
    {
        _initializer->push(context);
        _variable->setValue(context->pop());
    }
private:
    VariableDeclarationStatement(Type type, Reference<Identifier> identifier, Reference<Expression> initializer, DiagnosticLocation location)
      : _type(type), _identifier(identifier), _initializer(initializer), _location(location)
    { }
    Type _type;
    Reference<Identifier> _identifier;
    Reference<Expression> _initializer;
    DiagnosticLocation _location;
    Reference<Variable> _variable;
};

class Integer : public Expression
{
public:
    static Reference<Integer> parse(CharacterSource* source, Context* context)
    {
        CharacterSource s = *source;
        int n = 0;
        int c = s.get();
        if (c < '0' || c > '9')
            return 0;
        do {
            n = n*10 + c - '0';
            *source = s;
            c = s.get();
            if (c < '0' || c > '9') {
                Space::parse(source, context);
                return new Integer(n);
            }
        } while (true);
    }
    void compile(Context* context) { }
    Type type() const { return IntType(); }
    void push(Context* context)
    {
        context->push(Value(_n));
    }
private:
    Integer(int n) : _n(n) { }
    int _n;
};

class DoubleQuotedString : public Expression
{
public:
    static Reference<DoubleQuotedString> parse(CharacterSource* source, Context* context)
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
        CharacterSource s = *source;
        if (s.get() != '"')
            return 0;
        *source = s;
        int start = s.offset();
        int end;
        String insert;
        int n;
        int nn;
        String string;
        do {
            s = *source;
            end = s.offset();
            int c = s.get();
            if (c < 0x20) {
                if (c == 10)
                    source->location().throwError(endOfLine);
                if (c == -1)
                    source->location().throwError(endOfFile);
                source->throwUnexpected(printableCharacter, String::hexadecimal(c, 2));
            }
            *source = s;
            switch (c) {
                case '"':
                    string += s.subString(start, end);
                    Space::parse(source, context);
                    return new DoubleQuotedString(string);
                case '\\':
                    string += s.subString(start, end);
                    c = s.get();
                    if (c < 0x20) {
                        if (c == 10)
                            source->location().throwError(endOfLine);
                        if (c == -1)
                            source->location().throwError(endOfFile);
                        source->throwUnexpected(escapedCharacter, String::hexadecimal(c, 2));
                    }
                    *source = s;
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
                                nn = parseHexadecimalCharacter(source, context);
                                if (nn == -1) {
                                    s = *source;
                                    source->throwUnexpected(hexadecimalDigit, String::codePoint(s.get()));
                                }
                                n = (n << 4) | nn;
                            }
                            nn = parseHexadecimalCharacter(source, context);
                            if (nn != -1) {
                                n = (n << 4) | nn;
                                nn = parseHexadecimalCharacter(source, context);
                                if (nn != -1)
                                    n = (n << 4) | nn;
                            }
                            insert = String::codePoint(n);
                            break;
                        default:
                            source->throwUnexpected(escapedCharacter, String::codePoint(c));
                    }
                    string += insert;
                    start = source->offset();
                    break;
            }
        } while (true);
    }
    void compile(Context* context) { }
    Type type() const { return StringType(); }
    void push(Context* context)
    {
        context->push(Value(_string));
    }
private:
    DoubleQuotedString(String string) : _string(string) { }

    static int parseHexadecimalCharacter(CharacterSource* source, Context* context)
    {
        CharacterSource s = *source;
        int c = s.get();
        if (c >= '0' && c <= '9') {
            *source = s;
            return c - '0';
        }
        if (c >= 'A' && c <= 'F') {
            *source = s;
            return c + 10 - 'A';
        }
        if (c >= 'a' && c <= 'f') {
            *source = s;
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
        static String error1("Don't know how to add a ");
        static String error2(" to a ");
        throw Exception(error1 + _left->type().toString() + error2 + _right->type().toString());
    }
    void compile(Context* context)
    {
        _left->compile(context);
        _right->compile(context);
    }
    void push(Context* context)
    {
        _right->push(context);
        _left->push(context);
        Value l = context->pop();
        Value r = context->pop();
        if (_left->type() == StringType())
            if (_right->type() == StringType())
                context->push(Value(l.getString() + r.getString()));
            else
                context->push(Value(l.getString() + String::decimal(r.getInt())));
        else
            if (_right->type() == StringType())
                context->push(Value(String::decimal(l.getInt()) + r.getString()));
            else
                context->push(Value(l.getInt() + r.getInt()));
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

        context.addType(String("String"), StringType(), DiagnosticLocation());
        context.addType(String("Void"), VoidType(), DiagnosticLocation());

        PrintFunction print;
        TypeList printArgumentTypes;
        printArgumentTypes.push(StringType());
        printArgumentTypes.finalize();
        Type printFunctionType = FunctionType(VoidType(), printArgumentTypes);
        context.addFunction(String("print"), printArgumentTypes, &print, DiagnosticLocation());

        CharacterSource source(contents, file.path());
        Space::parse(&source, &context);
        Reference<StatementSequence> program = StatementSequence::parse(&source, &context);
        CharacterSource s = source;
        if (s.get() != -1) {
            static String error("Expected end of file");
            source.location().throwError(error);
        }

        program->compile(&context);

        program->run(&context);
    }
    END_CHECKED(Exception& e) {
        e.write(Handle::consoleOutput());
    }
}
