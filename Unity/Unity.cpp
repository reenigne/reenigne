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

class SymbolName : public ReferenceCounted
{
public:
    virtual int resolveIdentifier(Span span) = 0;
};

class VariableName : public SymbolName
{
public:
    VariableName(int label) : _label(label) { }
    int resolveIdentifier(Span span)
    {
        return _label;
    }
private:
    int _label;
};

SymbolEntry typeOf(SymbolEntry entry)
{
    if (entry.isArray()) {
        SymbolList list;
        SymbolArray array = entry.array();
        for (int i = 0; i < array.count(); ++i)
            list.add(typeOf(array[i]).symbol());
        return SymbolArray(list);
    }
    if (!entry.isSymbol())
        return Symbol();
    Symbol symbol = entry.symbol();
    Symbol type = symbol.type();
    if (type.valid())
        return type;
    switch (symbol.atom()) {
        case atomFunctionDefinitionStatement:
            {
                Symbol returnType = symbol[1].symbol();
                SymbolList list;
                SymbolArray parameters = symbol[3].array();
                for (int i = 0; i < parameters.count(); ++i)
                    list.add(parameters[i][1].symbol());
                type = Symbol(atomFunction, returnType, SymbolArray(list));
            }
            break;
        case atomAuto:
        case atomBit:
        case atomBoolean:
        case atomByte:
        case atomCharacter:
        case atomInt:
        case atomString:
        case atomUInt:
        case atomVoid:
        case atomWord:
            type = symbol;
            break;
        case atomPointer:
            type = Symbol(atomPointer, typeOf(symbol[1]));
            break;
        case atomFunction:
            type = Symbol(atomFunction, typeOf(symbol[1]), typeOf(symbol[2]));
            break;
        case atomTypeIdentifier:
            resolveIdentifiers(symbol);
            type = symbol.type();
            break;
        case atomTypeOf:
            resolveIdentifiers(symbol);
            type = typeOf(symbol[1]).symbol();
            break;
        case atomLogicalOr:
        case atomLogicalAnd:
            {
                static String error("expression is of type ");
                static String error2(", Boolean expected");
                type = typeOf(symbol[1]).symbol();
                if (type.atom() != atomBoolean)
                    symbol[1].symbol().span().throwError(error + typeToString(type) + error2);
                type = typeOf(symbol[2]).symbol();
                if (type.atom() != atomBoolean)
                    symbol[2].symbol().span().throwError(error + typeToString(type) + error2);
            }
            break;
        case atomDot:
            // TODO: Resolve type of left, look up right identifier in class symbol table
            break;
        case atomTrue:
        case atomFalse:
            type = Symbol(atomBoolean);
            break;
        case atomStringConstant:
            type = Symbol(atomString);
            break;
        case atomIdentifier:
            resolveIdentifiers(symbol);
            type = symbol.type();
            break;
        case atomIntegerConstant:
            type = Symbol(atomInt);  // TODO: the type is actually one of several depending on the value
            break;
        case atomFunctionCall:
            {
                Symbol function = symbol[1].symbol();
                if (function.atom() == atomIdentifier) {
                    SymbolList list;
                    SymbolArray arguments = symbol[2].array();
                    for (int i = 0; i < arguments.count(); ++i)
                        list.add(typeOf(arguments[i]).symbol());
                    Scope* scope = dynamic_cast<Scope*>(symbol.cache());
                    Symbol functionName = function[1].symbol();
                    int label = scope->resolveFunction(functionName.string(), list, functionName.span());
                    Symbol target = Symbol::target(label);
                    typeOf(target);
                    type = target[1].symbol();
                }
                else
                    type = typeOf(function).symbol()[1].symbol();
            }
            break;
        case atomNull:
            type = Symbol(atomPointer);
            break;
        case atomVariableDefinitionStatement:
            // TODO: Check that type of initializer (3) is same as typeSpecifier (1)
            // TODO: Handle "Auto" variables
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
            // TODO: Check that types are the same
            break;
        case atomTypeAliasStatement:
            // TODO
            break;
        case atomIfStatement:
            // TODO: Check that condition has type Boolean
            break;
        case atomSwitchStatement:
            // TODO: Check that types of cases are same as type of expression
            break;
        case atomReturnStatement:
            // TODO: Check that type of expression matches return type of current function
            break;
        case atomIncludeStatement:
            // TODO
            break;
        case atomWhileStatement:
        case atomUntilStatement:
            // TODO: Check that condition has type Boolean
            break;
        case atomForStatement:
            // TODO: Check that condition has type Boolean
            break;
        case atomCase:
            // TODO
            break;
        case atomPrintFunction:
            type = Symbol(atomVoid);
            break;
    }
    symbol.setType(type);

    const SymbolTail* tail = symbol.tail();
    while (tail != 0) {
        typeOf(tail->head());
        tail = tail->tail();
    }

    return type;
}

class FunctionName : public SymbolName
{
public:
    int resolveIdentifier(Span span)
    {
        if (_overloads.count() > 1) {
            static String error(" is an overloaded function - I don't know which overload you mean.");
            span.throwError(_name + error);
        }
        return _label;
    }
    void addOverload(int label)
    {
        Symbol functionDefinition = Symbol::target(label);
        Symbol type = typeOf(functionDefinition).symbol();
        SymbolArray types = type[2].array();
        if (_overloads.hasKey(types)) {
            static String error("This overload has already been defined.");
            functionDefinition.span().throwError(error);
        }
        _overloads.add(types, label);
        if (_overloads.count() == 1)
            _argumentTypes = types;
    }
    int lookUpOverload(SymbolArray argumentTypes)
    {
        return _overloads[argumentTypes];
    }

private:
    HashTable<SymbolArray, int> _overloads;
    int _label;
    String _name;
    SymbolArray _argumentTypes;
};

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
    void addVariable(String name, int label, Span span)
    {
        if (_symbolTable.hasKey(name)) {
            static String error(" is already defined");
            span.throwError(name + error);
        }
        _symbolTable.add(name,new VariableName(label));
    }
    int resolveIdentifier(String name, Span span)
    {
        if (!_symbolTable.hasKey(name)) {
            if (_outer != 0)
                return _outer->resolveIdentifier(name, span);
            static String error("Undefined symbol ");
            span.throwError(error + name);
        }
        return _symbolTable[name]->resolveIdentifier(span);
    }
    void addFunction(String name, int label, Span span)
    {
        FunctionName* functionName;
        if (_symbolTable.hasKey(name)) {
            Reference<SymbolName> symbol = _symbolTable[name];
            functionName = dynamic_cast<FunctionName*>(static_cast<SymbolName*>(symbol));
            if (functionName == 0) {
                static String error(" is already defined as a variable");
                span.throwError(name + error);
            }
        }
        else {
            functionName = new FunctionName;
            _symbolTable.add(name, functionName);
        }
        functionName->addOverload(label);
    }
    void addType(String name, int label, Span span)
    {
        if (_typeTable.hasKey(name)) {
            static String error(" has already been defined.");
            span.throwError(name + error);
        }
        _typeTable.add(name, label);
    }
    int resolveFunction(String name, SymbolArray argumentTypes, Span span)
    {
        if (!_symbolTable.hasKey(name)) {
            if (_outer == 0) {
                static String error("Undefined function ");
                span.throwError(error + name);
            }
            return _outer->resolveFunction(name, argumentTypes, span);
        }
        Reference<SymbolName> symbol = _symbolTable.lookUp(name);
        FunctionName* functionName = dynamic_cast<FunctionName*>(static_cast<SymbolName*>(symbol));
        if (functionName == 0) {
            static String error(" is not a function");
            location.throwError(name + error);
        }
        if (!functionName->hasOverload(argumentTypes)) {
            static String error(" has no overload with argument types ");
            location.throwError(name + error + typesToString(argumentTypes);
        }
        return functionName->lookUpOverload(argumentTypes);
    }
    int resolveType(String name, Span span)
    {
        if (!_typeTable.hasKey(name)) {
            if (_outer == 0) {
                static String error("Undefined type ");
                location.throwError(error + name);
            }
            return _outer->resolveType(name, span);
        }
        return _typeTable.lookUp(name);
    }
private:
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

Scope* setScope(SymbolEntry entry, Scope* scope)
{
    if (entry.isArray()) {
        SymbolArray array = entry.array();
        for (int i = 0; i < array.count(); ++i)
            scope = setScope(array[i], scope);
        return scope;
    }
    if (!entry.isSymbol())
        return scope;
    Symbol symbol = entry.symbol();
    Reference<Scope> inner = scope;
    switch (symbol.atom()) {
        case atomFunctionDefinitionStatement:
            inner = new Scope(scope, true);
            symbol.setCache(inner);
            setScope(symbol[1], inner);
            scope->addFunction(symbol[2].string(), symbol.label(), symbol.span());
            setScope(symbol[3], inner);
            setScope(symbol[4], inner);
            break;
        case atomVariableDefinitionStatement:
        case atomParameter:
            scope = new Scope(scope);
            symbol.setCache(scope);
            setScope(symbol[1], scope);
            scope->addVariable(symbol[2].string(), symbol.label(), symbol.span());
            if (symbol.atom() == atomVariableDefinitionStatement)
                setScope(symbol[3], scope);
            break;
        case atomTypeAliasStatement:
            scope->addType(symbol[2].string(), symbol.label(), symbol.span());
            break;
        case atomIfStatement:
            symbol.setCache(scope);
            setScope(symbol[1], scope);
            inner = new Scope(scope, true);
            setScope(symbol[2], inner);
            inner = new Scope(scope, true);
            setScope(symbol[3], inner);
            return scope;
        case atomCompoundStatement:
        case atomForeverStatement:
        case atomWhileStatement:
        case atomUntilStatement:
        case atomForStatement:
        case atomCase:
        case atomDefaultCase:
            inner = new Scope(scope, true);
            break;

    }
    symbol.setCache(inner);
    const SymbolTail* tail = symbol.tail();
    while (tail != 0) {
        setScope(tail->head(), inner);
        tail = tail->tail();
    }
    return scope;
}

// Determine types of variables and expressions, add them to the Symbols
// Resolve identifiers to labels
void resolveIdentifiers(SymbolEntry entry)
{
    if (entry.isArray()) {
        SymbolArray array = entry.array();
        for (int i = 0; i < array.count(); ++i)
            resolveIdentifiers(array[i]);
        return;
    }
    if (!entry.isSymbol())
        return;
    Symbol symbol = entry.symbol();
    Scope* scope = dynamic_cast<Scope*>(symbol.cache());
    int label = symbol.label();
    if (label != -1)
        return;
    Symbol target;
    switch (symbol.atom()) {
        case atomIdentifier:
            label = scope->resolveIdentifier(symbol[1].string(), symbol.span());
            symbol.setLabel(label);
            target = Symbol::target(label);
            symbol.setType(typeOf(target).symbol());
            return;
        case atomFunctionCall:
            // TODO
            break;
        case atomTypeIdentifier:
            label = scope->resolveType(symbol[1].string(), symbol.span());
            symbol.setLabel(label);
            target = Symbol::target(label);
            symbol.setType(typeOf(target).symbol());
            return;
    }

    const SymbolTail* tail = symbol.tail();
    while (tail != 0) {
        resolveIdentifiers(tail->head());
        tail = tail->tail();
    }
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
        Symbol print(atomPrintFunction, Symbol(atomVoid), String("print"), SymbolArray(Symbol(atomString)));
        print.setLabelTarget(printLabel);
        scope->addFunction(String("print"), printLabel, Span());

        CharacterSource source(contents, file.path());
        Space::parse(&source);
        SymbolArray program = parseStatementSequence(&source);
        CharacterSource s = source;
        if (s.get() != -1) {
            static String error("Expected end of file");
            source.location().throwError(error);
        }

        setScope(program, scope);
        resolveIdentifiers(program);
    }
    END_CHECKED(Exception& e) {
        e.write(Handle::consoleOutput());
    }
}
