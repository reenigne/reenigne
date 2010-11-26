#include "unity/string.h"
#include "unity/character_source.h"
#include "unity/array.h"
#include "unity/file.h"
#include "unity/stack.h"
#include "unity/hash_table.h"

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

#include "Type.cpp"

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

class FunctionDeclarationStatement;

template<class T> class FunctionNameTemplate : public Symbol
{
public:
    void addOverload(TypeList argumentTypes, FunctionDeclarationStatement* function, DiagnosticLocation location)
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
    FunctionDeclarationStatement* lookUpOverload(TypeList argumentTypes)
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
    HashTable<TypeList, FunctionDeclarationStatement*> _overloads;
    Type _functionType;
};

typedef FunctionNameTemplate<void> FunctionName;

class TypeDefinitionStatement;

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

template<class T> class ScopeTemplate;

typedef ScopeTemplate<void> Scope;

template<class T> class ScopeTemplate : public ReferenceCounted
{
public:
    ScopeTemplate(Scope* outer, bool functionScope = false)
      : _outer(outer)
    {
        if (functionScope)
            _functionScope = this;
        else
            _functionScope = outer->_functionScope;
    }
    Scope* outer() const { return _outer; }
    Scope* functionScope() const { return _functionScope; }
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
    Reference<Symbol> resolveSymbol(String name, DiagnosticLocation location)
    {
        if (!_symbolTable.hasKey(name)) {
            if (_outer != 0)
                return _outer->resolveSymbol(name, location);
            static String error("Undefined symbol ");
            location.throwError(error + name);
        }
        return _symbolTable.lookUp(name);
    }
    void addFunction(String name, TypeList argumentTypes, FunctionDeclarationStatement* function, DiagnosticLocation location)
    {
        _functionScope->doAddFunction(name, argumentTypes, function, location);
    }
    void addType(String name, TypeDefinitionStatement* type, DiagnosticLocation location)
    {
        _functionScope->doAddType(name, type, location);
    }
    FunctionDeclarationStatement* resolveFunction(Reference<Identifier> identifier, TypeList typeList, DiagnosticLocation location)
    {
        return _functionScope->doResolveFunction(identifier, typeList, location);
    }
    TypeDefinitionStatement* resolveType(String name, DiagnosticLocation location)
    {
        return _functionScope->doResolveType(name, location);
    }
private:
    void doAddFunction(String name, TypeList argumentTypes, FunctionDeclarationStatement* function, DiagnosticLocation location)
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
    void doAddType(String name, TypeDefinitionStatement* type, DiagnosticLocation location)
    {
        if (_typeTable.hasKey(name)) {
            static String error(" has already been defined.");
            location.throwError(name + error);
        }
        _typeTable.add(name, type);
    }
    FunctionDeclarationStatement* doResolveFunction(Reference<Identifier> identifier, TypeList typeList, DiagnosticLocation location)
    {
        String name = identifier->name();
        if (!_symbolTable.hasKey(name)) {
            if (_outer == 0) {
                static String error("Undefined function ");
                location.throwError(error + name);
            }
            return _outer->resolveFunction(identifier, typeList, location);
        }
        Reference<Symbol> symbol = _symbolTable.lookUp(name);
        FunctionName* functionName = dynamic_cast<FunctionName*>(static_cast<Symbol*>(symbol));
        if (functionName == 0) {
            static String error(" is not a function");
            location.throwError(name + error);
        }
        if (!functionName->hasOverload(typeList)) {
            static String error(" has no overload with argument types ");
            location.throwError(name + error + typeList.toString());
        }
        return functionName->lookUpOverload(typeList);
    }
    TypeDefinitionStatement* doResolveType(String name, DiagnosticLocation location)
    {
        if (!_typeTable.hasKey(name)) {
            if (_outer == 0) {
                static String error("Undefined type ");
                location.throwError(error + name);
            }
            return _outer->resolveType(name, location);
        }
        return _typeTable.lookUp(name);
    }
    HashTable<String, Reference<Symbol> > _symbolTable;
    HashTable<String, TypeDefinitionStatement*> _typeTable;
    Scope* _outer;
    Scope* _functionScope;
};

class Space
{
public:
    static void parse(CharacterSource* source)
    {
        do {
            CharacterSource s = *source;
            int c = s.get();
            if (c == ' ' || c == 10) {
                *source = s;
                continue;
            }
            if (parseComment(source))
                continue;
            return;
        } while (true);
    }
    static bool parseCharacter(CharacterSource* source, int character)
    {
        if (!source->parse(character))
            return false;
        parse(source);
        return true;
    }
    static void assertCharacter(CharacterSource* source, int character)
    {
        source->assert(character);
        parse(source);
    }
private:
    static bool parseComment(CharacterSource* source)
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
                if (parseComment(&s))
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

template<class T> class TypeIdentifierTemplate;
typedef TypeIdentifierTemplate<void> TypeIdentifier;

template<class T> class TypeSpecifierTemplate : public ReferenceCounted
{
public:
    static Reference<TypeSpecifierTemplate> parse(CharacterSource* source, Scope* scope)
    {
        Reference<TypeSpecifierTemplate> typeSpecifier = TypeIdentifier::parse(source, scope);
        if (!typeSpecifier.valid())
            return 0;
        do {
            if (Space::parseCharacter(source, '*')) {
                typeSpecifier = new PointerTypeSpecifier(typeSpecifier);
                continue;
            }
            if (Space::parseCharacter(source, '(')) {
                Reference<TypeListSpecifier> typeListSpecifier = TypeListSpecifier::parse(source, scope);
                typeSpecifier = new FunctionTypeSpecifier(typeSpecifier, typeListSpecifier);
                continue;
            }
            break;
        } while (true);
        return typeSpecifier;
    }
    virtual Type type() = 0;
};

typedef TypeSpecifierTemplate<void> TypeSpecifier;

template<class T> class TypeIdentifierTemplate : public TypeSpecifier
{
public:
    static Reference<TypeIdentifier> parse(CharacterSource* source, Scope* scope)
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
        Space::parse(source);
        return new TypeIdentifier(scope, s.subString(start, end), location);
    }
    String name() const { return _name; }
    Type type()
    {
        if (!_type.valid()) {
            if (_resolving) {
                static String error("A type cannot be defined in terms of itself");
                _location.throwError(error);
            }
            _resolving = true;
            _type = _scope->resolveType(_name, _location)->type();
            _resolving = false;
        }
        return _type;
    }
private:
    TypeIdentifierTemplate(Scope* scope, String name, DiagnosticLocation location)
      : _scope(scope), _name(name), _location(location), _resolving(false)
    { }
    Scope* _scope;
    String _name;
    DiagnosticLocation _location;
    bool _resolving;
    Type _type;
};

class PointerTypeSpecifier : public TypeSpecifier
{
public:
    PointerTypeSpecifier(Reference<TypeSpecifier> referentTypeSpecifier)
      : _referentTypeSpecifier(referentTypeSpecifier)
    { }
    Type type()
    {
        if (!_type.valid())
            _type = PointerType(_referentTypeSpecifier->type());
        return _type;
    }
private:
    Reference<TypeSpecifier> _referentTypeSpecifier;
    Type _type;
};

class TypeListSpecifier : public ReferenceCounted
{
public:
    static Reference<TypeListSpecifier> parse(CharacterSource* source, Scope* scope)
    {
        Stack<Reference<TypeSpecifier> > stack;
        do {
            Reference<TypeSpecifier> typeSpecifier = TypeSpecifier::parse(source, scope);
            if (!typeSpecifier.valid()) {
                static String error("Type specifier expected");
                source->location().throwError(error);
            }
            stack.push(typeSpecifier);
            if (Space::parseCharacter(source, ','))
                continue;
            break;
        } while (true);
        return new TypeListSpecifier(&stack);
    }
    TypeList typeList()
    {
        if (!_typeList.valid()) {
            for (int i = 0; i < _array.count(); ++i)
                _typeList.push(_array[i]->type());
            _typeList.finalize();
        }
        return _typeList;
    }
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
    Type type()
    {
        if (!_type.valid())
            _type = FunctionType(_returnTypeSpecifier->type(), _argumentTypeListSpecifier->typeList());
        return _type;
    }
private:
    Reference<TypeSpecifier> _returnTypeSpecifier;
    Reference<TypeListSpecifier> _argumentTypeListSpecifier;
    Type _type;
};


template<class T> class ExpressionTemplate : public ReferenceCounted
{
public:
    static Reference<ExpressionTemplate> parse(CharacterSource* source, Scope* scope)
    {
        Reference<Expression> e = parseComponent(source, scope);
        if (!e.valid())
            return 0;
        do {
            if (Space::parseCharacter(source, '+')) {
                Reference<Expression> e2 = parseComponent(source, scope);
                if (!e2.valid()) {
                    static String literal("Expected literal or opening parenthesis");
                    source->location().throwError(literal);
                }
                e = new AddExpression(e, e2);
            }
            else
                return e;
        } while (true);
    }
    static Reference<ExpressionTemplate> parseComponent(CharacterSource* source, Scope* scope)
    {
        Reference<Expression> e = DoubleQuotedString::parse(source, scope);
        if (e.valid())
            return e;
        e = EmbeddedLiteral::parse(source);
        if (e.valid())
            return e;
        e = Integer::parse(source);
        if (e.valid())
            return e;
        e = Identifier::parse(source, scope);
        if (e.valid())
            return e;
        if (Space::parseCharacter(source, '(')) {
            e = parse(source, scope);
            Space::assertCharacter(source, ')');
            return e;
        }
        return 0;
    }
    virtual void compile() = 0;
    virtual Type type() const = 0;
    virtual void push(Stack<Value>* scope) = 0;
};

typedef ExpressionTemplate<void> Expression;

class Identifier : public Expression
{
public:
    static Reference<Identifier> parse(CharacterSource* source, Scope* scope)
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
        Space::parse(source);
        return new Identifier(scope, s.subString(start, end), location);
    }
    Type type() const { return _symbol->type(_name, _location); }
    String name() const { return _name; }
    void push(Stack<Value>* stack)
    {
        stack->push(_symbol->value());
    }
    void compile()
    {
        _symbol = _scope->resolveSymbol(_name, _location);
    }
private:
    Identifier(Scope* scope, String name, DiagnosticLocation location)
      : _scope(scope), _name(name), _location(location)
    { }
    Scope* _scope;
    String _name;
    Reference<Symbol> _symbol;
    DiagnosticLocation _location;
};

class Integer : public Expression
{
public:
    static Reference<Integer> parse(CharacterSource* source)
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
                Space::parse(source);
                return new Integer(n);
            }
        } while (true);
    }
    void compile() { }
    Type type() const { return IntType(); }
    void push(Stack<Value>* stack)
    {
        stack->push(Value(_n));
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
    void compile()
    {
        _left->compile();
        _right->compile();
    }
    void push(Stack<Value>* stack)
    {
        _right->push(stack);
        _left->push(stack);
        Value l = stack->pop();
        Value r = stack->pop();
        if (_left->type() == StringType())
            if (_right->type() == StringType())
                stack->push(Value(l.getString() + r.getString()));
            else
                stack->push(Value(l.getString() + String::decimal(r.getInt())));
        else
            if (_right->type() == StringType())
                stack->push(Value(String::decimal(l.getInt()) + r.getString()));
            else
                stack->push(Value(l.getInt() + r.getInt()));
    }
private:
    Reference<Expression> _left;
    Reference<Expression> _right;
};

class DoubleQuotedString : public Expression
{
public:
    static Reference<Expression> parse(CharacterSource* source, Scope* scope)
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
        if (!source->parse('"'))
            return 0;
        int start = source->offset();
        int end;
        String insert(empty);
        int n;
        int nn;
        String string(empty);
        Reference<Expression> expression;
        Reference<Expression> part;
        do {
            CharacterSource s = *source;
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
                    Space::parse(source);
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
                                nn = parseHexadecimalCharacter(source, scope);
                                if (nn == -1) {
                                    s = *source;
                                    source->throwUnexpected(hexadecimalDigit, String::codePoint(s.get()));
                                }
                                n = (n << 4) | nn;
                            }
                            nn = parseHexadecimalCharacter(source, scope);
                            if (nn != -1) {
                                n = (n << 4) | nn;
                                nn = parseHexadecimalCharacter(source, scope);
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
                    part = Identifier::parse(source, scope);
                    if (!part.valid()) {
                        if (Space::parseCharacter(source, '(')) {
                            part = Expression::parse(source, scope);
                            source->assert(')');
                        }
                    }
                    string += s.subString(start, end);
                    start = source->offset();
                    if (part.valid()) {
                        expression = combine(expression, new DoubleQuotedString(string));
                        string = empty;
                        expression = combine(expression, part);
                    }
                    break;
            }
        } while (true);
    }
    void compile() { }
    Type type() const { return StringType(); }
    void push(Stack<Value>* stack) { stack->push(Value(_string)); }
private:
    static Reference<Expression> combine(Reference<Expression> left, Reference<Expression> right)
    {
        if (left.valid())
            return new AddExpression(left, right);
        return right;
    }
    DoubleQuotedString(String string) : _string(string) { }

    static int parseHexadecimalCharacter(CharacterSource* source,
        Scope* scope)
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
    static Reference<EmbeddedLiteral> parse(CharacterSource* source)
    {
        static String empty;
        static String endOfFile("End of file in string");
        if (!source->parse('#'))
            return 0;
        if (!source->parse('#'))
            return 0;
        if (!source->parse('#'))
            return 0;
        int start = source->offset();
        CharacterSource s = *source;
        do {
            int c = s.get();
            if (c == -1)
                source->location().throwError(endOfFile);
            if (c == 10)
                break;
            *source = s;
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
                Space::parse(source);
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
                            Space::parse(source);
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
    void compile() { }
    Type type() const { return StringType(); }
    void push(Stack<Value>* stack) { stack->push(Value(_string)); }
private:
    EmbeddedLiteral(String string) : _string(string) { }

    String _string;
};

template<class T> class StatementTemplate : public ReferenceCounted
{
public:
    static Reference<StatementTemplate> parse(CharacterSource* source, Scope* scope)
    {
        Reference<Statement> s = FunctionCallStatement::parse(source, scope);
        if (s.valid())
            return s;
        s = FunctionDeclarationStatement::parse(source, scope);
        if (s.valid())
            return s;
        s = VariableDeclarationStatement::parse(source, scope);
        if (s.valid())
            return s;
        s = AssignmentStatement::parse(source, scope);
        if (s.valid())
            return s;
        s = CompoundStatement::parse(source, scope);
        if (s.valid())
            return s;
        s = TypeAliasStatement::parse(source, scope);
        if (s.valid())
            return s;
        return 0;
    }
    virtual void resolveTypes() = 0;
    virtual void compile() = 0;
    virtual void run(Stack<Value>* stack) = 0;
};

typedef StatementTemplate<void> Statement;

class StatementSequence : public ReferenceCounted
{
public:
    static Reference<StatementSequence> parse(CharacterSource* source, Scope* scope)
    {
        Stack<Reference<Statement> > statements;
        do {
            Reference<Statement> statement = Statement::parse(source, scope);
            if (!statement.valid())
                break;
            statements.push(statement);
        } while (true);
        Reference<StatementSequence> statementSequence = new StatementSequence;
        statements.toArray(&statementSequence->_statements);
        return statementSequence;
    }
    void resolveTypes()
    {
        for (int i = 0; i < _statements.count(); ++i)
            _statements[i]->resolveTypes();
    }
    void compile()
    {
        for (int i = 0; i < _statements.count(); ++i)
            _statements[i]->compile();
    }
    void run(Stack<Value>* stack)
    {
        for (int i = 0; i < _statements.count(); ++i)
            _statements[i]->run(stack);
    }
private:
    StatementSequence() { }
    Array<Reference<Statement> > _statements;
};

class FunctionDeclarationStatement : public Statement
{
public:
    static Reference<FunctionDeclarationStatement> parse(CharacterSource* source, Scope* scope)
    {
        DiagnosticLocation location = source->location();
        CharacterSource s = *source;
        Reference<TypeSpecifier> returnTypeSpecifier = TypeSpecifier::parse(&s, scope);
        if (!returnTypeSpecifier.valid())
            return 0;
        Reference<Identifier> name = Identifier::parse(&s, scope);
        if (!name.valid())
            return 0;
        if (!Space::parseCharacter(&s, '('))
            return 0;
        Reference<Scope> inner = new Scope(scope, true);
        *source = s;
        Stack<Reference<Argument> > stack;
        do {
            Reference<Argument> argument = Argument::parse(source, inner);
            stack.push(argument);
            if (Space::parseCharacter(source, ')'))
                break;
            Space::assertCharacter(source, ',');
        } while (true);

        Reference<Identifier> identifier = Identifier::parse(source, inner);
        static String from("from");
        if (identifier.valid() && identifier->name() == from) {
            Reference<Expression> dll = DoubleQuotedString::parse(source, inner);
            if (dll.valid())
                return new FunctionDeclarationStatement(inner, returnTypeSpecifier, name, &stack, dll, location);
            static String error("Expected string");
            source->location().throwError(error);
        }
        Reference<Statement> statement = Statement::parse(source, inner);
        if (!statement.valid()) {
            static String error("Expected statement");
            source->location().throwError(error);
        }
        return new FunctionDeclarationStatement(inner, returnTypeSpecifier, name, &stack, statement, location);
    }
    void resolveTypes()
    {
        _returnType = _returnTypeSpecifier->type();
        TypeList typeList;
        for (int i = 0; i < _arguments.count(); ++i)
            typeList.push(_arguments[i]->type());
        typeList.finalize();
        _scope->outer()->addFunction(_name->name(), typeList, this, _location);
    }
    void compile()
    {
        if (_statement.valid())
            _statement->compile();
    }
    void run(Stack<Value>* stack) { }
    virtual Type returnType() const { return _returnTypeSpecifier->type(); }
    virtual void call(Stack<Value>* stack)
    {
        if (_statement.valid()) {
            for (int i = 0; i < _arguments.count(); ++i)
                _arguments[i]->setValue(stack->pop(), _scope);
            _statement->run(stack);
        }
        else {

        }
    }
protected:
    FunctionDeclarationStatement() { }
private:
    class Argument : public ReferenceCounted
    {
    public:
        static Reference<Argument> parse(CharacterSource* source, Scope* scope)
        {
            Reference<TypeSpecifier> typeSpecifier = TypeSpecifier::parse(source, scope);
            if (!typeSpecifier.valid()) {
                static String error("Expected type specifier");
                source->location().throwError(error);
            }
            Reference<Identifier> name = Identifier::parse(source, scope);
            if (!name.valid()) {
                static String error("Expected identifier");
                source->location().throwError(error);
            }
            return new Argument(typeSpecifier, name);
        }
        Type type() { return _typeSpecifier->type(); }
        void setValue(Value value, Scope* scope)
        {
            Reference<Variable> variable = scope->resolveSymbol(_name->name(), DiagnosticLocation());
            variable->setValue(value);
        }
    private:
        Argument(Reference<TypeSpecifier> typeSpecifier, Reference<Identifier> name)
          : _typeSpecifier(typeSpecifier), _name(name)
        { }
        Reference<TypeSpecifier> _typeSpecifier;
        Reference<Identifier> _name;
    };
    FunctionDeclarationStatement(Reference<Scope> scope, Reference<TypeSpecifier> returnTypeSpecifier, Reference<Identifier> name, Stack<Reference<Argument> >* stack, Reference<Expression> dll, DiagnosticLocation location)
      : _scope(scope), _returnTypeSpecifier(returnTypeSpecifier), _name(name), _dll(dll), _location(location)
    {
        stack->toArray(&_arguments);
    }
    FunctionDeclarationStatement(Reference<Scope> scope, Reference<TypeSpecifier> returnTypeSpecifier, Reference<Identifier> name, Stack<Reference<Argument> >* stack, Reference<Statement> statement, DiagnosticLocation location)
      : _scope(scope), _returnTypeSpecifier(returnTypeSpecifier), _name(name), _statement(statement), _location(location)
    {
        stack->toArray(&_arguments);
    }

    Reference<Scope> _scope;
    Reference<TypeSpecifier> _returnTypeSpecifier;
    Reference<Identifier> _name;
    Array<Reference<Argument> > _arguments;
    Reference<Expression> _dll;
    Reference<Statement> _statement;
    Type _returnType;
    DiagnosticLocation _location;
};

class FunctionCallStatement : public Statement
{
public:
    static Reference<FunctionCallStatement> parse(CharacterSource* source, Scope* scope)
    {
        CharacterSource s = *source;
        DiagnosticLocation location = s.location();
        Reference<Identifier> functionName = Identifier::parse(&s, scope);
        if (!functionName.valid())
            return 0;
        if (!Space::parseCharacter(&s, '('))
            return 0;
        int n = 0;
        Stack<Reference<Expression> > stack;
        *source = s;
        if (!Space::parseCharacter(source, ')')) {
            do {
                Reference<Expression> e = Expression::parse(source, scope);
                if (!e.valid()) {
                    static String expression("Expected expression");
                    source->location().throwError(expression);
                }
                stack.push(e);
                ++n;
                if (Space::parseCharacter(source, ')'))
                    break;
                Space::assertCharacter(source, ',');
            } while (true);
        }
        Space::assertCharacter(source, ';');
        Reference<FunctionCallStatement> functionCall = new FunctionCallStatement(scope, functionName, n, location);
        stack.toArray(&functionCall->_arguments);
        return functionCall;
    }
    void resolveTypes() { }
    void compile()
    {
        TypeList argumentTypes;
        for (int i = 0; i < _arguments.count(); ++i) {
            _arguments[i]->compile();
            argumentTypes.push(_arguments[i]->type());
        }
        argumentTypes.finalize();
        _function = _scope->resolveFunction(_functionName, argumentTypes, _location);
    }
    void run(Stack<Value>* stack)
    {
        for (int i = _arguments.count() - 1; i >= 0; --i)
            _arguments[i]->push(stack);
        _function->run(stack);
    }
private:
    FunctionCallStatement(Scope* scope, Reference<Identifier> functionName, int n, DiagnosticLocation location)
      : _scope(scope), _functionName(functionName), _location(location)
    {
        _arguments.allocate(n);
    }

    Scope* _scope;
    Reference<Identifier> _functionName;
    FunctionDeclarationStatement* _function;
    Array<Reference<Expression> > _arguments;
    DiagnosticLocation _location;
};

class VariableDeclarationStatement : public Statement
{
public:
    static Reference<VariableDeclarationStatement> parse(CharacterSource* source, Scope* scope)
    {
        Reference<Scope> inner = new Scope(scope);
        DiagnosticLocation location = source->location();
        CharacterSource s = *source;
        Reference<TypeSpecifier> typeSpecifier = TypeSpecifier::parse(&s, inner);
        if (!typeSpecifier.valid())
            return 0;
        Reference<Identifier> identifier = Identifier::parse(&s, inner);
        if (!identifier.valid())
            return 0;
        *source = s;
        Reference<Expression> initializer;
        if (Space::parseCharacter(source, '=')) {
            initializer = Expression::parse(source, inner);
            if (!initializer.valid()) {
                static String expression("Expected expression");
                source->location().throwError(expression);
            }
        }
        Space::assertCharacter(source, ';');
        return new VariableDeclarationStatement(inner, typeSpecifier, identifier, initializer, location);
    }
    void resolveTypes()
    {
        _typeSpecifier->type();
    }
    void compile()
    {
        _variable = _scope->outer()->addVariable(_identifier->name(), _typeSpecifier->type(), _location);
    }
    void run(Stack<Value>* stack)
    {
        if (_initializer.valid()) {
            _initializer->push(stack);
            _variable->setValue(stack->pop());
        }
    }
private:
    VariableDeclarationStatement(Reference<Scope> scope, Reference<TypeSpecifier> typeSpecifier, Reference<Identifier> identifier, Reference<Expression> initializer, DiagnosticLocation location)
      : _scope(scope), _typeSpecifier(typeSpecifier), _identifier(identifier), _initializer(initializer), _location(location)
    { }
    Reference<Scope> _scope;
    Reference<TypeSpecifier> _typeSpecifier;
    Reference<Identifier> _identifier;
    Reference<Expression> _initializer;
    DiagnosticLocation _location;
    Reference<Variable> _variable;
};

class AssignmentStatement : public Statement
{
public:
    static Reference<AssignmentStatement> parse(CharacterSource* source, Scope* scope)
    {
        DiagnosticLocation location = source->location();
        CharacterSource s = *source;
        Reference<Identifier> identifier = Identifier::parse(&s, scope);
        if (!identifier.valid())
            return 0;
        if (!Space::parseCharacter(&s, '='))
            return 0;
        *source = s;
        Reference<Expression> e = Expression::parse(source, scope);
        if (!e.valid()) {
            static String expression("Expected expression");
            source->location().throwError(expression);
        }
        Space::assertCharacter(source, ';');
        return new AssignmentStatement(scope, identifier, e, location);
    }
    void resolveTypes() { }
    void compile()
    {
        String name = _identifier->name();
        _variable = _scope->resolveSymbol(name, _location);
        if (!_variable.valid()) {
            static String error(" is not a variable");
            _location.throwError(name + error);
        }
    }
    void run(Stack<Value>* stack)
    {
        _value->push(stack);
        _variable->setValue(stack->pop());
    }
private:
    AssignmentStatement(Scope* scope, Reference<Identifier> identifier, Reference<Expression> value, DiagnosticLocation location)
      : _scope(scope), _identifier(identifier), _value(value), _location(location)
    { }
    Scope* _scope;
    Reference<Identifier> _identifier;
    Reference<Expression> _value;
    DiagnosticLocation _location;
    Reference<Variable> _variable;
};

class PrintFunction : public FunctionDeclarationStatement
{
public:
    PrintFunction() : _consoleOutput(Handle::consoleOutput())
    { }
    void resolveTypes() { }
    void compile() { }
    void run(Stack<Value>* stack)
    {
        stack->pop().getString().write(_consoleOutput);
    }
    Type returnType() const { return VoidType(); }
private:
    Handle _consoleOutput;
};

class CompoundStatement : public Statement
{
public:
    static Reference<CompoundStatement> parse(CharacterSource* source, Scope* scope)
    {
        if (!Space::parseCharacter(source, '{'))
            return 0;
        Reference<Scope> inner = new Scope(scope, true);
        Reference<StatementSequence> sequence = StatementSequence::parse(source, inner);
        Space::assertCharacter(source, '}');
        return new CompoundStatement(inner, sequence);
    }
    void resolveTypes()
    {
        _sequence->resolveTypes();
    }
    void compile()
    {
        _sequence->compile();
    }
    void run(Stack<Value>* stack)
    {
        _sequence->run(stack);
    }
private:
    CompoundStatement(Reference<Scope> scope, Reference<StatementSequence> sequence)
      : _scope(scope), _sequence(sequence)
    { }
    Reference<Scope> _scope;
    Reference<StatementSequence> _sequence;
};

class TypeDefinitionStatement : public Statement
{
public:
    void resolveTypes() { }
    void compile() { }
    void run(Stack<Value>* stack) { }
    virtual Type type() = 0;
};

class TypeAliasStatement : public TypeDefinitionStatement
{
public:
    static Reference<TypeAliasStatement> parse(CharacterSource* source, Scope* scope)
    {
        CharacterSource s = *source;
        CharacterSource s2 = s;
        Reference<TypeIdentifier> typeIdentifier = TypeIdentifier::parse(&s, scope);
        if (!typeIdentifier.valid())
            return 0;
        if (!Space::parseCharacter(&s, '='))
            return 0;
        *source = s;
        Reference<TypeSpecifier> typeSpecifier = TypeSpecifier::parse(source, scope);
        Space::assertCharacter(source, ';');
        Reference<TypeAliasStatement> typeAliasStatement = new TypeAliasStatement(scope, typeIdentifier, typeSpecifier);
        scope->addType(typeIdentifier->name(), typeAliasStatement, s2.location());
        return typeAliasStatement;
    }
    void resolveTypes()
    {
        _typeIdentifier->type();
    }
    Type type() { return _typeSpecifier->type(); }
private:
    TypeAliasStatement(Scope* scope, Reference<TypeIdentifier> typeIdentifier, Reference<TypeSpecifier> typeSpecifier)
      : _scope(scope), _typeIdentifier(typeIdentifier), _typeSpecifier(typeSpecifier)
    { }
    Scope* _scope;
    Reference<TypeIdentifier> _typeIdentifier;
    Reference<TypeSpecifier> _typeSpecifier;
};

class StringTypeDefinitionStatement : public TypeDefinitionStatement
{
public:
    Type type() { return StringType(); }
};

class IntTypeDefinitionStatement : public TypeDefinitionStatement
{
public:
    Type type() { return IntType(); }
};

class VoidTypeDefinitionStatement : public TypeDefinitionStatement
{
public:
    Type type() { return VoidType(); }
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
        Reference<Scope> scope = new Scope(0, true);

        Reference<PrintFunction> print = new PrintFunction();
        TypeList printArgumentTypes;
        printArgumentTypes.push(StringType());
        printArgumentTypes.finalize();
        Type printFunctionType = FunctionType(VoidType(), printArgumentTypes);
        scope->addFunction(String("print"), printArgumentTypes, print, DiagnosticLocation());

        CharacterSource source(contents, file.path());
        Space::parse(&source);
        Reference<StatementSequence> program = StatementSequence::parse(&source, scope);
        CharacterSource s = source;
        if (s.get() != -1) {
            static String error("Expected end of file");
            source.location().throwError(error);
        }

        Reference<StringTypeDefinitionStatement> stringTypeDefinitionStatement = new StringTypeDefinitionStatement();
        scope->addType(String("String"), stringTypeDefinitionStatement, DiagnosticLocation());
        Reference<VoidTypeDefinitionStatement> voidTypeDefinitionStatement = new VoidTypeDefinitionStatement();
        scope->addType(String("Void"), voidTypeDefinitionStatement, DiagnosticLocation());
        Reference<IntTypeDefinitionStatement> intTypeDefinitionStatement = new IntTypeDefinitionStatement();
        scope->addType(String("Int"), intTypeDefinitionStatement, DiagnosticLocation());

        program->resolveTypes();
        program->compile();

        Stack<Value> stack;
        program->run(&stack);
    }
    END_CHECKED(Exception& e) {
        e.write(Handle::consoleOutput());
    }
}
