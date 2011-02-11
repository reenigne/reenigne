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
    int frameOffset() const { return _frameOffset; }
    void setframeOffset(int frameOffset) { _frameOffset = frameOffset; }
private:
    int _label;
    int _frameOffset;
};

SymbolEntry typeOf(SymbolEntry entry);

void assertTypeBoolean(Symbol expression)
{
    static String error("expression is of type ");
    static String error2(", Boolean expected");
    Symbol type = typeOf(expression).symbol();
    if (type.atom() != atomBoolean)
        expression.span().throwError(error + typeToString(type) + error2);
}

void checkTypes(SymbolEntry entry, Symbol returnType)
{
    if (entry.isArray()) {
        SymbolList list;
        SymbolArray array = entry.array();
        for (int i = 0; i < array.count(); ++i)
            checkTypes(array[i], returnType);
    }
    if (!entry.isSymbol())
        return;
    Symbol symbol = entry.symbol();
    switch (symbol.atom()) {
        case atomLogicalOr:
        case atomLogicalAnd:
            assertTypeBoolean(symbol[1].symbol());
            assertTypeBoolean(symbol[2].symbol());
            break;
        case atomFunctionCall:
            {
                Symbol function = symbol[1].symbol();
                SymbolArray parameterTypes = typeOf(function).symbol()[2].array();
                SymbolArray argumentTypes = typeOf(symbol[2]).array();
                if (parameterTypes != argumentTypes) {
                    static String error("function requires arguments of types ");
                    static String error2(" but passed arguments of types ");
                    symbol.span().throwError(error + typesToString(parameterTypes) + error2 + typesToString(argumentTypes));
                }
            }
            break;
        case atomFunctionDefinitionStatement:
            checkTypes(symbol[3], returnType);
            checkTypes(symbol[4], symbol[1].symbol());
            return;
        case atomVariableDefinitionStatement:
            {
                Symbol initializerType = typeOf(symbol[3]).symbol();
                Symbol variableType = typeOf(symbol[1]).symbol();
                if (variableType != initializerType) {
                    static String error("variable declared as type ");
                    static String error2(" but initialized with expression of type ");
                    symbol.span().throwError(error + typeToString(variableType) + error2 + typeToString(initializerType));
                }
            }
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
            {
                Symbol lValueType = typeOf(symbol[1]).symbol();
                Symbol rValueType = typeOf(symbol[2]).symbol();
                if (lValueType != rValueType) {
                    static String error("can't assign a expression of type ");
                    static String error2(" to a variable of type ");
                    symbol.span().throwError(error + typeToString(rValueType) + error2 + typeToString(lValueType));
                }
            }
            break;
        case atomIfStatement:
            assertTypeBoolean(symbol[1].symbol());
            break;
        case atomSwitchStatement:
            {
                Symbol type = typeOf(symbol[1]).symbol();
                SymbolArray cases = symbol[2].array();
                for (int i = 0; i < cases.count(); ++i) {
                    Symbol c = cases[i];
                    SymbolArray expressions = c[1].array();
                    for (int j = 0; j < expressions.count(); ++j) {
                        Symbol expression = expressions[j];
                        Symbol expressionType = typeOf(expression).symbol();
                        if (type != expressionType) {
                            static String error("can't compare an expression of type ");
                            static String error2(" to an epxression of type ");
                            expression.span().throwError(error + typeToString(type) + error2 + typeToString(expressionType));
                        }
                    }
                }
            }
            break;
        case atomReturnStatement:
            {
                Symbol expression = symbol[1].symbol();
                Symbol type;
                if (expression.valid())
                    type = typeOf(expression).symbol();
                else
                    type = Symbol(atomVoid);
                if (type != returnType) {
                    static String error("returning an expression of type ");
                    static String error2(" from a function with return type ");
                    symbol.span().throwError(error + typeToString(type) + error2 + typeToString(returnType));
                }
            }
            break;
        case atomIncludeStatement:
            {
                Symbol expression = symbol[1].symbol();
                Symbol type = typeOf(expression).symbol();
                if (type.atom() != atomString) {
                    static String error("argument to include is of type ");
                    static String error2(", expected String");
                    expression.span().throwError(error + typeToString(type) + error2);
                }
            }
            break;
        case atomWhileStatement:
        case atomUntilStatement:
        case atomForStatement:
            assertTypeBoolean(symbol[2].symbol());
            break;
    }

    const SymbolTail* tail = symbol.tail();
    while (tail != 0) {
        checkTypes(tail->head(), returnType);
        tail = tail->tail();
    }
}

void resolveIdentifiers(SymbolEntry entry);

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
        case atomParameter:
            type = typeOf(symbol[1]).symbol();
            break;
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
            type = Symbol(atomBoolean);
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
            type = typeOf(symbol[1]).symbol()[1].symbol();
            break;
        case atomNull:
            type = Symbol(atomPointer);
            break;
        case atomVariableDefinitionStatement:
            {
                type = typeOf(symbol[1]).symbol();
                if (type.atom() == atomAuto) {
                    Symbol initializer = symbol[3].symbol();
                    if (!initializer.valid()) {
                        static String error("Auto variable declarations must be initialized");
                        symbol.span().throwError(error);
                    }
                    type = typeOf(symbol[3]).symbol();
                    symbol[1].symbol().setType(type);
                }
            }
            break;
        case atomPrintFunction:
            type = Symbol(atomFunction, Symbol(atomVoid), SymbolArray(Symbol(atomString)));
            break;
    }
    symbol.setType(type);
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
    bool hasOverload(SymbolArray argumentTypes)
    {
        return _overloads.hasKey(argumentTypes);
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
        Reference<SymbolName> symbol = _symbolTable[name];
        FunctionName* functionName = dynamic_cast<FunctionName*>(static_cast<SymbolName*>(symbol));
        if (functionName == 0) {
            static String error(" is not a function");
            span.throwError(name + error);
        }
        if (!functionName->hasOverload(argumentTypes)) {
            static String error(" has no overload with argument types ");
            span.throwError(name + error + typesToString(argumentTypes));
        }
        return functionName->lookUpOverload(argumentTypes);
    }
    int resolveType(String name, Span span)
    {
        if (!_typeTable.hasKey(name)) {
            if (_outer == 0) {
                static String error("Undefined type ");
                span.throwError(error + name);
            }
            return _outer->resolveType(name, span);
        }
        return _typeTable[name];
    }
    void setStackOffset(int offset) { _offset = offset; }
    int getStackOffset() { return _offset; }
private:
    HashTable<String, Reference<SymbolName> > _symbolTable;
    HashTable<String, int> _typeTable;
    Scope* _outer;
    Scope* _functionScope;
    int _offset;
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
            {
                Symbol function = symbol[1].symbol();
                if (function.atom() == atomIdentifier) {
                    SymbolList list;
                    SymbolArray arguments = symbol[2].array();
                    for (int i = 0; i < arguments.count(); ++i)
                        list.add(typeOf(arguments[i]).symbol());
                    Symbol functionName = function[1].symbol();
                    int label = scope->resolveFunction(functionName.string(), list, functionName.span());
                    target = Symbol::target(label);
                    return;
                }
            }
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

class Compiler
{
public:
    void compileFunctionBody(SymbolArray program)
    {
        // TODO:
        //   Walk through the statement tree (but not child classes and functions)
        //     Assign a stack offset to each 
        compileStatementSequence(program);
    }
    void compileFunction(Symbol functionDefinitionStatement)
    {
        // TODO:
        //   Assign a stack offset to each parameter
        //     Compute number of stack slots for each type
        compileFunctionBody(functionDefinitionStatement[4].array());
    }
    SymbolList compiledProgram() const { return _compiledProgram; }
private:
    void compileStatementSequence(SymbolArray program)
    {
        for (int i = 0; i < program.count(); ++i)
            compileStatement(program[i]);
    }
    void finishBasicBlock(int nextLabel)
    {
        Symbol block(atomBasicBlock, SymbolArray(_basicBlock), nextLabel);
        block.setLabelTarget(_label);
        _compiledProgram.add(block);
        _basicBlock = SymbolList();
        _label = nextLabel;
    }
    void compileStatement(Symbol statement)
    {
        switch (statement.atom()) {
            case atomExpressionStatement:
                compileExpression(statement[1].symbol());
                break;            
            case atomFunctionDefinitionStatement:
                {
                    Compiler compiler;
                    compiler.compileFunction(statement);
                    _compiledProgram.add(compiler.compiledProgram());
                }
                break;
            case atomFromStatement:
            case atomVariableDefinitionStatement:
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
                // TODO
                break;
            case atomCompoundStatement:
                compileStatementSequence(statement[1].array());
                break;
            case atomTypeAliasStatement:
                break;
            case atomNothingStatement:
                break;
            case atomIncrementStatement:
            case atomDecrementStatement:
                // TODO
                break;
            case atomIfStatement:
                {
                    int falseClause = Symbol::newLabel();
                    int done = Symbol::newLabel();
                    compileExpression(statement[1].symbol());
                    add(Symbol(atomNot));
                    addJumpIfTrue(falseClause);
                    compileStatement(statement[2].symbol());
                    addGoto(done);
                    addLabel(falseClause);
                    compileStatement(statement[3].symbol());
                    addLabel(done);
                }
                break;
            case atomSwitchStatement:
            case atomReturnStatement:
            case atomIncludeStatement:
            case atomBreakStatement:
            case atomContinueStatement:
                // TODO
                break;
            case atomForeverStatement:
                {
                    int done = Symbol::newLabel();
                    int start = getLabel();
                    _breakContinueStack.push(BreakContinueStackEntry(done, start));
                    compileStatement(statement[1].symbol());
                    addGoto(start);
                    addLabel(done);
                    _breakContinueStack.pop();
                }
                break;
            case atomWhileStatement:
                {
                    int done = Symbol::newLabel();
                    int final = Symbol::newLabel();
                    int start = getLabel();
                    _breakContinueStack.push(BreakContinueStackEntry(final, start));
                    compileStatement(statement[1].symbol());
                    compileExpression(statement[2].symbol());
                    add(Symbol(atomNot));
                    addJumpIfTrue(done);
                    compileStatement(statement[3].symbol());
                    addGoto(start);
                    _breakContinueStack.pop();
                    addLabel(done);
                    compileStatement(statement[4].symbol());
                }
                break;
            case atomUntilStatement:
            case atomForStatement:
                // TODO
                break;
        }
    }
    // Add instructions to push the value of expression onto the stack.
    void compileExpression(Symbol expression)
    {
        switch (expression.atom()) {
            case atomLogicalOr:
                {
                    int pushRight = Symbol::newLabel();
                    int pushTrue = Symbol::newLabel();
                    int done = Symbol::newLabel();
                    compileExpression(expression[1].symbol());
                    addJumpIfTrue(pushTrue);
                    addLabel(pushRight);
                    compileExpression(expression[2].symbol());
                    addGoto(done);
                    addLabel(pushTrue);
                    add(Symbol(atomTrue));
                    addLabel(done);
                }
                break;
            case atomLogicalAnd:
                {
                    int pushRight = Symbol::newLabel();
                    int pushFalse = Symbol::newLabel();
                    int done = Symbol::newLabel();
                    compileExpression(expression[1].symbol());
                    add(Symbol(atomNot));
                    addJumpIfTrue(pushFalse);
                    compileExpression(expression[2].symbol());
                    addGoto(done);
                    addLabel(pushFalse);
                    add(Symbol(atomFalse));
                    addLabel(done);
                }
                break;
            case atomDot:
                // TODO
                break;
            case atomDereference:
                // TODO
                break;
            case atomAddressOf:
                // TODO
                break;
            case atomFunctionCall:
                {
                    SymbolArray arguments = expression[2].array();
                    for (int i = arguments.count() - 1; i >= 0; --i)
                        compileExpression(arguments[i]);
                    compileExpression(expression[1].symbol());
                    add(Symbol(atomCall));
                }
                break;
            case atomIntegerConstant:
            case atomStringConstant:
                add(expression);
                break;
            case atomIdentifier:
                // TODO
                break;
            case atomTrue:
            case atomFalse:
            case atomNull:
                add(expression);
                break;
        }
    }
    void add(Symbol symbol)
    {
        _basicBlock.add(symbol);
        _blockEnds = false;
        _atBlockStart = false;
    }
    void addGoto(int destination)
    {
        add(Symbol(atomIntegerConstant, destination));
        add(Symbol(atomGoto));
        _blockEnds = true;
    }
    void addJumpIfTrue(int destination)
    {
        add(Symbol(atomIntegerConstant, destination));
        add(Symbol(atomJumpIfTrue));
    }
    void addLabel(int label)
    {
        int follows = label;
        if (_blockEnds)
            follows = -1;
        Symbol block(atomBasicBlock, SymbolArray(_basicBlock), follows);
        block.setLabelTarget(_label);
        _compiledProgram.add(block);
        _basicBlock = SymbolList();
        _label = label;
        _atBlockStart = true;
    }
    int getLabel()
    {
        if (!_atBlockStart)
            addLabel(Symbol::newLabel());
        return _label;
    }

    SymbolList _compiledProgram;
    SymbolList _basicBlock;
    int _label;
    bool _blockEnds;
    bool _atBlockStart;
    
    class BreakContinueStackEntry
    {
    public:
        BreakContinueStackEntry(int breakLabel, int continueLabel)
          : _breakLabel(breakLabel), _continueLabel(continueLabel) { }
        int _breakLabel;
        int _continueLabel;
    };
    Stack<BreakContinueStackEntry> _breakContinueStack;
};

class RunTimeStack
{
public:
    RunTimeStack(int bytes)
    {
        int entries = bytes >> 2;
        _data.allocate(entries);
        _sp = &_data[entries];

    }
    template<class T> T pop()
    {
        UInt32* p = _sp;
        ++_sp;
        return *reinterpret_cast<T*>(p);
    }
    template<> String pop<String>()
    {
        return pop<StringImplementation*>();
    }
    template<class T> void push(T value)
    {
        --_sp;
        *reinterpret_cast<T*>(_sp) = value;
    }
    template<> void push<String>(String value)
    {
        push(static_cast<StringImplementation*>(value.implementation()));
    }
    UInt32* pointer() const { return _sp; }
private:
    Array<UInt32> _data;
    UInt32* _sp;
};

int power(int a, int b)
{
    if (b < 0)
        return 1/power(a, -b);
    int r = 1;
    while (b != 0) {
        if ((b & 1) != 0)
            r *= a;
        b >>= 1;
        a *= a;
    }
    return r;
}

void run(SymbolArray program)
{
    class InstructionPointer
    {
    public:
        InstructionPointer(SymbolArray program)
        {
            _block = program[0];
            setup();
        }
        void jump(int label)
        {
            _block = Symbol::target(label);
            setup();
        }
        Symbol instruction()
        {
            Symbol instruction = _instructions[_instruction];
            ++_instruction;
            if (_instruction == _instructionsInBlock) {
                _label = _block[2].integer();
                jump(_label);
            }
            return instruction;
        }
        int label() const { return _label; }
    private:
        void setup()
        {
            _instructions = _block[1].array();
            _instruction = 0;
            _instructionsInBlock = _instructions.count();
        }
        Symbol _block;
        SymbolArray _instructions;
        int _instruction;
        int _instructionsInBlock;
        int _label;
    };
    RunTimeStack stack(0x100000);
    InstructionPointer ip(program);
    do {
        Symbol instruction = ip.instruction();
        switch (instruction.atom()) {
            case atomExit:
                return;
            case atomPrintFunction:
                stack.pop<String>().write(Handle::consoleOutput());
                break;
            case atomIntegerConstant:
                stack.push(instruction[1].integer());
                break;
            case atomStringConstant:
                stack.push(instruction[1].string());
                break;
            case atomTrue:
                stack.push(true);
                break;
            case atomFalse:
                stack.push(false);
                break;
            case atomNull:
                stack.push(0);
                break;
            case atomCall:
                {
                    int label = stack.pop<int>();
                    stack.push(ip.label());
                    ip.jump(label);
                }
                break;
            case atomReturn:
            case atomGoto:
                ip.jump(stack.pop<int>());
                break;
            case atomJumpIfTrue:
                {
                    int label = stack.pop<int>();
                    if (stack.pop<bool>())
                        ip.jump(label);
                }
                break;
            // TODO: Need implementations of the following for each possible type.
            case atomBitwiseOr:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l - r);
                }
                break;
            case atomBitwiseXor:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l ^ r);
                }
                break;
            case atomBitwiseAnd:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l & r);
                }
                break;
            case atomEqualTo:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l == r);
                }
                break;
            case atomNotEqualTo:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l != r);
                }
                break;
            case atomLessThanOrEqualTo:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l <= r);
                }
                break;
            case atomGreaterThanOrEqualTo:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l >= r);
                }
                break;
            case atomLessThan:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l < r);
                }
                break;
            case atomGreaterThan:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l > r);
                }
                break;
            case atomLeftShift:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l << r);
                }
                break;
            case atomRightShift:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l >> r);
                }
                break;
            case atomAdd:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l + r);
                }
                break;
            case atomSubtract:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l - r);
                }
                break;
            case atomMultiply:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l * r);
                }
                break;
            case atomDivide:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l / r);
                }
                break;
            case atomModulo:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(l % r);
                }
                break;
            case atomNot:
                stack.push(~stack.pop<int>());
                break;
            case atomNegative:
                stack.push(-stack.pop<int>());
                break;
            case atomStackPointer:
                stack.push(stack.pointer());
                break;
            case atomDereference:
                stack.push(*stack.pop<int*>());
                break;
            case atomPower:
                {
                    int l = stack.pop<int>();
                    int r = stack.pop<int>();
                    stack.push(power(l, r));
                }
                break;

            case atomStringConcatenate:
                {
                    String l = stack.pop<String>();
                    String r = stack.pop<String>();
                    stack.push(l + r);
                }
                break;
            case atomStringEqualTo:
                {
                    String l = stack.pop<String>();
                    String r = stack.pop<String>();
                    stack.push(l == r);
                }
                break;
            case atomStringNotEqualTo:
                {
                    String l = stack.pop<String>();
                    String r = stack.pop<String>();
                    stack.push(l != r);
                }
                break;
            case atomStringLessThanOrEqualTo:
                {
                    String l = stack.pop<String>();
                    String r = stack.pop<String>();
                    stack.push(l <= r);
                }
                break;
            case atomStringGreaterThanOrEqualTo:
                {
                    String l = stack.pop<String>();
                    String r = stack.pop<String>();
                    stack.push(l >= r);
                }
                break;
            case atomStringLessThan:
                {
                    String l = stack.pop<String>();
                    String r = stack.pop<String>();
                    stack.push(l < r);
                }
                break;
            case atomStringGreaterThan:
                {
                    String l = stack.pop<String>();
                    String r = stack.pop<String>();
                    stack.push(l > r);
                }
                break;
            case atomStringIntegerMultiply:
                {
                    String l = stack.pop<String>();
                    int r = stack.pop<int>();
                    stack.push(l * r);
                }
        }
    } while (true);
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
        checkTypes(program, Symbol(atomVoid));
        Compiler compiler;
        compiler.compileFunctionBody(program);
        SymbolArray compiledProgram = compiler.compiledProgram();
        run(compiledProgram);
    }
    END_CHECKED(Exception& e) {
        e.write(Handle::consoleOutput());
    }
}
