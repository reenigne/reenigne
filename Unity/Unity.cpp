#include "unity/string.h"
#include "unity/array.h"
#include "unity/file.h"
#include "unity/stack.h"
#include "unity/hash_table.h"
#include "unity/character_source.h"

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

#include "Symbol.cpp"
#include "Type.cpp"

class Variable;
class FunctionDeclarationStatement;

class Value
{
public:
    Value() { }
    Value(int intValue) : _intValue(intValue) { }
    Value(String stringValue) : _stringValue(stringValue) { }
    Value(Variable* pointerValue) : _pointerValue(pointerValue) { }
    Value(FunctionDeclarationStatement* functionDeclaration) : _pointerValue(functionDeclaration) { }
    int getInt() const { return _intValue; }
    String getString() const { return _stringValue; }
    Variable* getPointer() const { return static_cast<Variable*>(_pointerValue); }
    FunctionDeclarationStatement* getFunctionDeclaration() const { return static_cast<FunctionDeclarationStatement*>(_pointerValue); }
private:
    int _intValue;
    String _stringValue;
    void* _pointerValue;
};

class SymbolName : public ReferenceCounted
{
public:
    virtual Symbol type(String name, Location location) const = 0;
    virtual Value value() const = 0;
};

class FunctionDeclarationStatement;

template<class T> class FunctionNameTemplate : public SymbolName
{
public:
    void addOverload(SymbolList argumentTypes, FunctionDeclarationStatement* function, Location location)
    {
        if (_overloads.hasKey(argumentTypes)) {
            static String error("This overload has already been defined.");
            location.throwError(error);
        }
        _overloads.add(argumentTypes, function);
        if (_overloads.count() == 1)
            _functionType = FunctionType(function->returnType(), argumentTypes);
    }
    bool hasOverload(SymbolList argumentTypes)
    {
        return _overloads.hasKey(argumentTypes);
    }
    FunctionDeclarationStatement* lookUpOverload(SymbolList argumentTypes)
    {
        return _overloads.lookUp(argumentTypes);
    }
    Symbol type(String name, Location location) const
    {
        if (_overloads.count() > 1) {
            static String error(" is an overloaded function - I don't know which overload you mean.");
            location.throwError(name + error);
        }
        return _functionType;
    }
    Value value() const
    {
        return Value(_functionDeclaration);
    }
private:
    String _name;
    HashTable<SymbolList, FunctionDeclarationStatement*> _overloads;
    Symbol _functionType;
    FunctionDeclarationStatement* _functionDeclaration;
};

typedef FunctionNameTemplate<void> FunctionName;

class TypeDefinitionStatement;

class Variable : public SymbolName
{
public:
    Variable(Symbol type) : _type(type) { }
    Symbol type(String name, Location location) const { return _type; }
    Value value() const { return _value; }
    void setValue(Value value) { _value = value; }
private:
    Symbol _type;
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
//    Reference<Variable> addVariable(String name, Symbol type, Location location)
//    {
//        if (_symbolTable.hasKey(name)) {
//            static String error(" is already defined");
//            location.throwError(name + error);
//        }
//        Reference<Variable> variable = new Variable(type);
//        _symbolTable.add(name, variable);
//        return variable;
//    }
//    Reference<SymbolName> resolveSymbolName(String name, Location location)
//    {
//        if (!_symbolTable.hasKey(name)) {
//            if (_outer != 0)
//                return _outer->resolveSymbolName(name, location);
//            static String error("Undefined symbol ");
//            location.throwError(error + name);
//        }
//        return _symbolTable.lookUp(name);
//    }
    void addFunction(String name, int label)
    {
        _functionScope->doAddFunction(name, label);
    }
//    void addType(String name, TypeDefinitionStatement* type, Location location)
//    {
//        _functionScope->doAddType(name, type, location);
//    }
//    FunctionDeclarationStatement* resolveFunction(Reference<Identifier> identifier, SymbolList typeList, Location location)
//    {
//        return _functionScope->doResolveFunction(identifier, typeList, location);
//    }
//    TypeDefinitionStatement* resolveType(String name, Location location)
//    {
//        return _functionScope->doResolveType(name, location);
//    }
private:
    void doAddFunction(String name, int label)
    {
        FunctionName* functionName;
        if (_symbolTable.hasKey(name)) {
            Reference<SymbolName> symbol = _symbolTable.lookUp(name);
            functionName = dynamic_cast<FunctionName*>(static_cast<SymbolName*>(symbol));
            if (functionName == 0) {
                static String error(" is already defined as a variable");
                location.throwError(name + error);
            }
        }
        else {
            functionName = new FunctionName;
            _symbolTable.add(name, functionName);
        }
        functionName->addOverload(label);
    }
//    void doAddType(String name, TypeDefinitionStatement* type, Location location)
//    {
//        if (_typeTable.hasKey(name)) {
//            static String error(" has already been defined.");
//            location.throwError(name + error);
//        }
//        _typeTable.add(name, type);
//    }
//    FunctionDeclarationStatement* doResolveFunction(Reference<Identifier> identifier, SymbolList typeList, Location location)
//    {
//        String name = identifier->name();
//        if (!_symbolTable.hasKey(name)) {
//            if (_outer == 0) {
//                static String error("Undefined function ");
//                location.throwError(error + name);
//            }
//            return _outer->resolveFunction(identifier, typeList, location);
//        }
//        Reference<SymbolName> symbol = _symbolTable.lookUp(name);
//        FunctionName* functionName = dynamic_cast<FunctionName*>(static_cast<SymbolName*>(symbol));
//        if (functionName == 0) {
//            static String error(" is not a function");
//            location.throwError(name + error);
//        }
//        if (!functionName->hasOverload(typeList)) {
//            static String error(" has no overload with argument types ");
//            location.throwError(name + error + typeList.toString());
//        }
//        return functionName->lookUpOverload(typeList);
//    }
//    TypeDefinitionStatement* doResolveType(String name, Location location)
//    {
//        if (!_typeTable.hasKey(name)) {
//            if (_outer == 0) {
//                static String error("Undefined type ");
//                location.throwError(error + name);
//            }
//            return _outer->resolveType(name, location);
//        }
//        return _typeTable.lookUp(name);
//    }
    HashTable<String, Reference<SymbolName> > _symbolTable;
    HashTable<String, int> _typeTable;
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
    static bool parseCharacter(CharacterSource* source, int character, Span& span)
    {
        Location start = source->location();
        if (!source->parse(character))
            return false;
        span = Span(start, source->location());
        parse(source);
        return true;
    }
    static Location assertCharacter(CharacterSource* source, int character)
    {
        source->assert(character);
        Location l = source->location();
        parse(source);
        return l;
    }
    static bool parseOperator(CharacterSource* source, String op, Span& span)
    {
        Location start = source->location();
        static String empty("");
        CharacterSource s = *source;
        CharacterSource o(op, empty);
        do {
            int c = o.get();
            if (c == -1)
                break;
            if (s.get() != c)
                return false;
        } while (true);
        *source = s;
        span = Span(start, source->location());
        parse(source);
        return true;
    }
    static bool parseKeyword(CharacterSource* source, String keyword, Span& span)
    {
        Location start = source->location();
        static String empty("");
        CharacterSource s = *source;
        CharacterSource o(keyword, empty);
        do {
            int c = o.get();
            if (c == -1)
                break;
            if (s.get() != c)
                return false;
        } while (true);
        CharacterSource s2 = s;
        int c = s2.get();
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')
            return false;
        *source = s;
        span = Span(start, source->location());
        parse(source);
        return true;
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

#include "TypeSpecifier.cpp"
#include "Statement.cpp"

void setExpressionScope(Symbol expression, Scope* scope)
{
    expression.cache() = scope;
    // TODO: set scope of subexpressions
}

void setTypeSpecifierScope(Symbol typeSpecifier, Scope* scope)
{
    typeSpecifier.cache() = scope;
    // TODO: set scope of members
}

void setParametersScope(SymbolArray parameters, Scope* scope)
{
    for (int i = 0; i < parameters.count(); ++i)
        setTypeSpecifierScope(parameters[i][1].symbol(), scope);
}

Scope* setStatementScope(Symbol statement, Scope* scope)
{
    statement.cache() = scope;
    Reference<Scope> inner;
    switch (statement.atom()) {
        case atomExpressionStatement:
            setExpressionScope(statement[1].symbol(), scope);
            break;
        case atomAssignmentStatement:
        case atomAddAssignmentStatement:
        case atomSubtractAssignmentStatement:
        case atomMultiplyAssignmentStatement:
        case atomDivideAssignmentStatement:
        case atomModuloAssignmentStatement:
        case atomShiftLeftAssignmentStatement:
        case atomShiftRightAssignmentStatement:
        case atomAndAssignmentStatement:
        case atomOrAssignmentStatement:
        case atomXorAssignmentStatement:
        case atomPowerAssignmentStatement:
            setExpressionScope(statement[1].symbol(), scope);
            setExpressionScope(statement[2].symbol(), scope);
            break;
        case atomFromStatement:
            setExpressionScope(statement[1].symbol(), scope);
            break;
        case atomTypeAliasStatement:
            setTypeSpecifierScope(statement[2].symbol(), scope);
            break;
        case atomNothingStatement:
            break;
        case atomIncrementStatement:
        case atomDecrementStatement:
            setExpressionScope(statement[1].symbol(), scope);
            break;
        case atomFunctionDefinitionStatement:
            inner = new Scope(scope, true);
            statement.cache() = inner;
            setTypeSpecifierScope(statement[1].symbol(), inner);
            setParametersScope(statement[3].symbol(), inner);
            setStatementScope(statement[4].symbol(), inner);
            scope->addFunction(statement[2].string(), statement.label());
            break;
        case atomVariableDefinitionStatement:
            scope = new Scope(scope);
            statement.cache() = scope;
            setTypeSpecifierScope(statement[1].symbol(), scope);
            setExpressionScope(statement[3].symbol(), scope);
            scope->addVariable(statement[2].string(), statement.label());
            break;
        case atomCompoundStatement:
            inner = new Scope(scope, true);
            statement.cache() = inner;
            setStatementArrayScope(statement[1].array(), inner);
            break;
        case atomIfStatement:
            setExpressionScope(statement[1].symbol(), scope);
            inner = new Scope(scope, true);
            setStatementScope(statement[2].symbol(), inner);
            inner = new Scope(scope, true);
            setStatementScope(statement[3].symbol(), inner);
            break;
        case atomSwitchStatement:

        case atomReturnStatement:
        case atomIncludeStatement:
        case atomBreakStatement:
        case atomContinueStatement:
        case atomForeverStatement:
        case atomWhileStatement:
        case atomUntilStatement:
        case atomForStatement:
    }
    return scope;
}

void setStatementArrayScope(SymbolArray program, Scope* scope)
{
    for (int i = 0; i < program.count(); ++i)
        scope = setStatementScope(program[i], scope);
}

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

        int printLabel = Symbol::newLabel();
        Symbol print(printLabel, atomPrintFunction, Span(), Symbol(atomVoid, Span()), String("print"), SymbolArray(Symbol(atomString, Span())));
        scope->addFunction(String("print"), printLabel);

        CharacterSource source(contents, file.path());
        Space::parse(&source);
        SymbolArray program = parseStatementSequence(&source);
        CharacterSource s = source;
        if (s.get() != -1) {
            static String error("Expected end of file");
            source.location().throwError(error);
        }

        setStatementArrayScope(program, scope);
    }
    END_CHECKED(Exception& e) {
        e.write(Handle::consoleOutput());
    }
}
