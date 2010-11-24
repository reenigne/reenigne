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
    Reference<Variable> addVariable(String name, Type type, DiagnosticLocation location)
    {
        if (_symbolTable.hasKey(name)) {
            static String error(" is already defined");
            location.throwError(name + error);
        }
        Reference<Variable> variable = new Variable(type);
        _symbolTable.add(name, variable);
        return variable;
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
    Type resolveType(String name, DiagnosticLocation location)
    {
        if (!_typeTable.hasKey(name)) {
            static String error("Undefined type ");
            location.throwError(error + name);
        }
        return _typeTable.lookUp(name);
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

class TypeIdentifier;

template<class T> class TypeSpecifierTemplate : public ReferenceCounted
{
public:
    static Reference<TypeSpecifierTemplate> parse(CharacterSource* source, Context* context)
    {
        Reference<TypeSpecifierTemplate> typeSpecifier = TypeIdentifier::parse(source, context);
        if (!typeSpecifier.valid())
            return 0;
        do {
            CharacterSource s = *source;
            int c = s.get();
            if (c == '*') {
                *source = s;
                Space::parse(source, context);
                typeSpecifier = new PointerTypeSpecifier(typeSpecifier);
                continue;
            }
            if (c == '(') {
                *source = s;
                Space::parse(source, context);
                Reference<TypeListSpecifier> typeListSpecifier = TypeListSpecifier::parse(source, context);
                typeSpecifier = new FunctionTypeSpecifier(typeSpecifier, typeListSpecifier);
                continue;
            }
            break;
        } while (true);
        return typeSpecifier;
    }
    virtual void compile(Context* context) = 0;
    Type type() const { return _type; }
protected:
    Type _type;
};

typedef TypeSpecifierTemplate<void> TypeSpecifier;

class TypeIdentifier : public TypeSpecifier
{
public:
    static Reference<TypeIdentifier> parse(CharacterSource* source, Context* context)
    {
        CharacterSource s = *source;
        DiagnosticLocation location = s.location();
        int start = s.offset();
        int c = s.get();
        if (c < 'A' || c > 'Z')
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
        return new TypeIdentifier(s.subString(start, end), location);
    }
    String name() const { return _name; }
    void compile(Context* context)
    {
        _type = context->resolveType(_name, _location);
    }
private:
    TypeIdentifier(String name, DiagnosticLocation location)
      : _name(name), _location(location)
    { }
    String _name;
    DiagnosticLocation _location;
};

class PointerTypeSpecifier : public TypeSpecifier
{
public:
    PointerTypeSpecifier(Reference<TypeSpecifier> referentTypeSpecifier)
      : _referentTypeSpecifier(referentTypeSpecifier)
    { }
    void compile(Context* context)
    {
        _referentTypeSpecifier->compile(context);
        _type = PointerType(_referentTypeSpecifier->type());
    }
private:
    Reference<TypeSpecifier> _referentTypeSpecifier;
};

class TypeListSpecifier : public ReferenceCounted
{
public:
    static Reference<TypeListSpecifier> parse(CharacterSource* source, Context* context)
    {
        Stack<Reference<TypeSpecifier> > _stack;
        do {
            Reference<TypeSpecifier> typeSpecifier = TypeSpecifier::parse(source, context);
            if (!typeSpecifier.valid()) {
                static String error("Type specifier expected");
                source->location().throwError(error);
            }
            _stack.push(typeSpecifier);
            CharacterSource s = *source;
            int c = s.get();
            if (c == ',') {
                *source = s;
                Space::parse(source, context);
                continue;
            }
            break;
        } while (true);
        return new TypeListSpecifier(&_stack);
    }
    void compile(Context* context)
    {
        for (int i = 0; i < _array.count(); ++i) {
            _array[i]->compile(context);
            _typeList.push(_array[i]->type());
        }
        _typeList.finalize();
    }
    TypeList typeList() const { return _typeList; }
private:
    TypeListSpecifier(Stack<Reference<TypeSpecifier> >* stack)
    {
        stack->toArray(&_array);
    }
    Array<Reference<TypeSpecifier> > _array;
    TypeList _typeList;
};

class FunctionTypeSpecifier : public TypeSpecifier
{
public:
    FunctionTypeSpecifier(Reference<TypeSpecifier> returnTypeSpecifier, Reference<TypeListSpecifier> argumentTypeListSpecifier)
      : _returnTypeSpecifier(returnTypeSpecifier), _argumentTypeListSpecifier(argumentTypeListSpecifier)
    { }
    void compile(Context* context)
    {
        _returnTypeSpecifier->compile(context);
        _argumentTypeListSpecifier->compile(context);
        _type = FunctionType(_returnTypeSpecifier->type(), _argumentTypeListSpecifier->typeList());
    }
private:
    Reference<TypeSpecifier> _returnTypeSpecifier;
    Reference<TypeListSpecifier> _argumentTypeListSpecifier;
};


template<class T> class ExpressionTemplate : public ReferenceCounted
{
public:
    static Reference<ExpressionTemplate> parse(CharacterSource* source, Context* context)
    {
        Reference<Expression> e = parseComponent(source, context);
        if (!e.valid())
            return 0;
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
        e = EmbeddedLiteral::parse(source, context);
        if (e.valid())
            return e;
        e = Integer::parse(source, context);
        if (e.valid())
            return e;
        e = Identifier::parse(source, context);
        if (e.valid())
            return e;
        CharacterSource s = *source;
        int c = s.get();
        if (c == '(') {
            *source = s;
            Space::parse(source, context);
            e = parse(source, context);
            source->assert(')');
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
        s = AssignmentStatement::parse(source, context);
        if (s.valid())
            return s;
        s = FunctionDeclarationStatement::parse(source, context);
        if (s.valid()
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
        Reference<TypeSpecifier> typeSpecifier = TypeSpecifier::parse(&s, context);
        if (!typeSpecifier.valid())
            return 0;
        Reference<Identifier> identifier = Identifier::parse(&s, context);
        if (!identifier.valid())
            return 0;
        Reference<Expression> initializer;
        *source = s;
        int c = s.get();
        if (c == '=') {
            s = *source;
            Space::parse(source, context);
            initializer = Expression::parse(source, context);
            if (!initializer.valid()) {
                static String expression("Expected expression");
                source->location().throwError(expression);
            }
        }
        source->assert(';');
        Space::parse(source, context);
        return new VariableDeclarationStatement(typeSpecifier, identifier, initializer, location);
    }
    void compile(Context* context)
    {
        _typeSpecifier->compile(context);
        _variable = context->addVariable(_identifier->name(), _typeSpecifier->type(), _location);
    }
    void run(Context* context)
    {
        if (_initializer.valid()) {
            _initializer->push(context);
            _variable->setValue(context->pop());
        }
    }
private:
    VariableDeclarationStatement(Reference<TypeSpecifier> typeSpecifier, Reference<Identifier> identifier, Reference<Expression> initializer, DiagnosticLocation location)
      : _typeSpecifier(typeSpecifier), _identifier(identifier), _initializer(initializer), _location(location)
    { }
    Reference<TypeSpecifier> _typeSpecifier;
    Reference<Identifier> _identifier;
    Reference<Expression> _initializer;
    DiagnosticLocation _location;
    Reference<Variable> _variable;
};

class AssignmentStatement : public Statement
{
public:
    static Reference<AssignmentStatement> parse(CharacterSource* source, Context* context)
    {
        DiagnosticLocation location = source->location();
        CharacterSource s = *source;
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
        return new AssignmentStatement(identifier, e, location);
    }
    void compile(Context* context)
    {
        String name = _identifier->name();
        _variable = context->resolveSymbol(name, _location);
        if (!_variable.valid()) {
            static String error(" is not a variable");
            _location.throwError(name + error);
        }
    }
    void run(Context* context)
    {
        _value->push(context);
        _variable->setValue(context->pop());
    }
private:
    AssignmentStatement(Reference<Identifier> identifier, Reference<Expression> value, DiagnosticLocation location)
      : _identifier(identifier), _value(value), _location(location)
    { }
    Reference<Identifier> _identifier;
    Reference<Expression> _value;
    DiagnosticLocation _location;
    Reference<Variable> _variable;
};

class FunctionDeclarationStatement : public Statement
{
public:
    static Reference<FunctionDeclarationStatement> parse(CharacterSource* source, Context* context)
    {
        Reference<TypeSpecifier> returnTypeSpecifier = TypeSpecifier::parse(source, context);
        if (!returnTypeSpecifier.valid())
            return 0;
        Reference<Identifier> name = Identifier::parse(source, context);
        if (!name.valid())
            return 0;
        CharacterSource s = *source;
        int c = s.get();
        if (c != '(')
            return 0;
        *source = s;
        Space::parse(source, context);
        Stack<Reference<Argument> > stack;
        do {
            Reference<Argument> argument = Argument::parse(source, context);
            stack.push(argument);
            s = *source;
            int c = s.get();
            if (c == ',') {
                *source = s;
                Space::parse(source, context);
                continue;
            }
            source->assert(')');
            break;
        } while (true);

        
        return new FunctionDeclarationStatement(returnTypeSpecifier, name, &stack);
    }
    void compile(Context* context)
    { }
    void run(Context* context)
    { }
private:
    FunctionDeclarationStatement()
    { }
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

class AddExpression : public Expression
{
public:
    AddExpression(Reference<Expression> left, Reference<Expression> right)
      : _left(left), _right(right)
    { }
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

class DoubleQuotedString : public Expression
{
public:
    static Reference<Expression> parse(CharacterSource* source, Context* context)
    {
        static String empty("");
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
        String insert(empty);
        int n;
        int nn;
        String string(empty);
        Reference<Expression> expression;
        Reference<Expression> part;
        do {
            s = *source;
            end = s.offset();
            int c = s.get();
            if (c < 0x20 && c != 10) {
                if (c == -1)
                    source->location().throwError(endOfFile);
                source->throwUnexpected(printableCharacter, String::hexadecimal(c, 2));
            }
            *source = s;
            switch (c) {
                case '"':
                    string += s.subString(start, end);
                    Space::parse(source, context);
                    return combine(expression, new DoubleQuotedString(string));
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
                case '$':
                    part = Identifier::parse(source, context);
                    if (!part.valid()) {
                        s = *source;
                        c = s.get();
                        if (c == '(') {
                            *source = s;
                            Space::parse(source, context);
                            part = Expression::parse(source, context);
                            source->assert(')');
                            if (!part.valid())
                                break;
                        }
                    }
                    string += s.subString(start, end);
                    start = source->offset();
                    expression = combine(expression, new DoubleQuotedString(string));
                    string = empty;
                    expression = combine(expression, part);
                    break;
            }
        } while (true);
    }
    void compile(Context* context) { }
    Type type() const { return StringType(); }
    void push(Context* context) { context->push(Value(_string)); }
private:
    static Reference<Expression> combine(Reference<Expression> left, Reference<Expression> right)
    {
        if (left.valid())
            return new AddExpression(left, right);
        return right;
    }
    DoubleQuotedString(String string) : _string(string) { }

    static int parseHexadecimalCharacter(CharacterSource* source,
        Context* context)
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

class EmbeddedLiteral : public Expression
{
public:
    static Reference<EmbeddedLiteral> parse(CharacterSource* source,
        Context* context)
    {
        static String empty;
        static String endOfFile("End of file in string");
        CharacterSource s = *source;
        if (s.get() != '#')
            return 0;
        if (s.get() != '#')
            return 0;
        if (s.get() != '#')
            return 0;
        int start = s.offset();
        do {
            *source = s;
            int c = s.get();
            if (c == -1)
                source->location().throwError(endOfFile);
            if (c == 10)
                break;
        } while (true);
        int end = source->offset();
        String terminator = source->subString(start, end);
        start = s.offset();
        CharacterSource terminatorSource(terminator, empty);
        int cc = terminatorSource.get();
        String string;
        do {
            *source = s;
            int c = s.get();
            if (c == -1)
                source->location().throwError(endOfFile);
            if (cc == -1) {
                if (c != '#')
                    continue;
                CharacterSource s2 = s;
                if (s2.get() != '#')
                    continue;
                if (s2.get() != '#')
                    continue;
                string += s.subString(start, source->offset());
                *source = s2;
                Space::parse(source, context);
                return new EmbeddedLiteral(string);
            }
            else
                if (c == cc) {
                    CharacterSource s2 = s;
                    CharacterSource st = terminatorSource;
                    do {
                        int ct = st.get();
                        if (ct == -1) {
                            if (s2.get() != '#')
                                break;
                            if (s2.get() != '#')
                                break;
                            if (s2.get() != '#')
                                break;
                            string += s.subString(start, source->offset());
                            *source = s2;
                            Space::parse(source, context);
                            return new EmbeddedLiteral(string);
                        }
                        int cs = s2.get();
                        if (ct != cs)
                            break;
                    } while (true);
                }
            if (c == 10) {
                string += s.subString(start, source->offset()) + String::codePoint(10);
                start = s.offset();
            }
        } while (true);
    }
    void compile(Context* context) { }
    Type type() const { return StringType(); }
    void push(Context* context) { context->push(Value(_string)); }
private:
    EmbeddedLiteral(String string) : _string(string) { }

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
        Context context;

        context.addType(String("String"), StringType(), DiagnosticLocation());
        context.addType(String("Void"), VoidType(), DiagnosticLocation());
        context.addType(String("Int"), IntType(), DiagnosticLocation());

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
