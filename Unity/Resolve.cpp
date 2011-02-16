class SpanCache : public SymbolCache
{
public:
    SpanCache(Span span) : _span(span) { }
    Span span() const { return _span; }
private:
    Span _span;
};

Span spanOf(Symbol symbol) { return symbol.cache<SpanCache>()->span(); }

SpanCache* newSpan(Location start, Location end)
{
    return new SpanCache(Span(start, end));
}

SpanCache* newSpan(Symbol symbol) { return new SpanCache(spanOf(symbol)); }

SpanCache* newSpan(Span span) { return new SpanCache(span); }

class ExpressionCache : public SpanCache
{
public:
    ExpressionCache(Span span) : SpanCache(span) { }
    void setType(Symbol type) { _type = type; }
    Symbol type() const { return _type; }
private:
    Symbol _type;
};

Symbol typeOf(SymbolEntry expression)
{
    return expression.symbol().cache<ExpressionCache>()->type();
}

void setType(Symbol symbol, Symbol type)
{
    symbol.cache<ExpressionCache>()->setType(type);
}

class Scope;

class IdentifierCache : public ExpressionCache
{
public:
    IdentifierCache(Span span, int label = -1)
      : ExpressionCache(span), _label(label), _offset(-1)
    { }
    void setLabel(int label) { _label = label; }
    int label() const { return _label; }
    void setScope(Scope* scope) { _scope = scope; }
    Scope* scope() { return _scope; }
    void setOffset(int offset) { _offset = offset; }
    int offset() const { return _offset; }
private:
    int _label;
    Reference<Scope> _scope;
    int _offset;
};

int labelOf(Symbol symbol)
{
    return symbol.cache<IdentifierCache>()->label();
}

void setLabel(Symbol symbol, int label)
{
    symbol.cache<IdentifierCache>()->setLabel(label);
}

Scope* scopeOf(Symbol symbol)
{
    return symbol.cache<IdentifierCache>()->scope();
}

void setOffset(Symbol symbol, int offset)
{
    symbol.cache<IdentifierCache>()->setOffset(offset);
}

int offsetOf(Symbol symbol)
{
    return symbol.cache<IdentifierCache>()->offset();
}


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
        Symbol functionDefinition = Symbol::labelled(label);
        Symbol type = typeOf(functionDefinition);
        SymbolArray types = type[2].array();
        if (_overloads.hasKey(types)) {
            static String error("This overload has already been defined.");
            spanOf(functionDefinition).throwError(error);
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

class Scope : public ReferenceCounted
{
public:
    Scope(Scope* outer, bool functionScope = false)
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

Scope* setScopes(SymbolEntry entry, Scope* scope)
{
    if (entry.isArray()) {
        SymbolArray array = entry.array();
        for (int i = 0; i < array.count(); ++i)
            scope = setScopes(array[i], scope);
        return scope;
    }
    if (!entry.isSymbol())
        return scope;
    Symbol symbol = entry.symbol();
    Reference<Scope> inner = scope;
    switch (symbol.atom()) {
        case atomFunctionDefinitionStatement:
            inner = new Scope(scope, true);
            scope->addFunction(symbol[2].string(), labelOf(symbol), spanOf(symbol));
            break;
        case atomVariableDefinitionStatement:
            scope = new Scope(scope);
            inner = scope;
            scope->addVariable(symbol[2].string(), labelOf(symbol), spanOf(symbol));
            break;
        case atomParameter:
            scope = new Scope(scope);
            inner = scope;
            scope->addVariable(symbol[2].string(), labelOf(symbol), spanOf(symbol));
            break;
        case atomTypeAliasStatement:
            scope->addType(symbol[2].string(), labelOf(symbol), spanOf(symbol));
            break;
        case atomIfStatement:
            setScopes(symbol[1], scope);
            inner = new Scope(scope, true);
            setScopes(symbol[2], inner);
            inner = new Scope(scope, true);
            setScopes(symbol[3], inner);
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
        case atomIdentifier:
        case atomTypeIdentifier:
            symbol.cache<IdentifierCache>()->setScope(scope);
            break;
    }
    const SymbolTail* tail = symbol.tail();
    while (tail != 0) {
        setScopes(tail->head(), inner);
        tail = tail->tail();
    }
    return scope;
}

void resolveIdentifier(Symbol identifier);
void resolveTypeOf(Symbol symbol);

void resolveIdentifiersAndTypes(SymbolEntry entry)
{
    if (entry.isArray()) {
        SymbolArray array = entry.array();
        for (int i = 0; i < array.count(); ++i)
            resolveIdentifiersAndTypes(array[i]);
    }
    if (!entry.isSymbol())
        return;
    Symbol symbol = entry.symbol();
    switch (symbol.atom()) {
        case atomIdentifier:
        case atomTypeIdentifier:
            resolveIdentifier(symbol);
            resolveTypeOf(symbol);
            return;
        case atomFunctionCall:
            {
                SymbolArray arguments = symbol[2].array();
                for (int i = 0; i < arguments.count(); ++i)
                    resolveIdentifiersAndTypes(arguments[i]);
                Symbol function = symbol[1].symbol();
                if (function.atom() == atomIdentifier)
                    resolveIdentifier(symbol);
                else
                    resolveIdentifiersAndTypes(function);
                resolveTypeOf(symbol);
            }
            return;
        case atomFunctionDefinitionStatement:
            resolveIdentifiersAndTypes(symbol[1]);
            resolveIdentifiersAndTypes(symbol[3]);
            resolveIdentifiersAndTypes(symbol[4]);
            resolveTypeOf(symbol);
            return;
        case atomVariableDefinitionStatement:
            resolveIdentifiersAndTypes(symbol[1]);
            resolveIdentifiersAndTypes(symbol[3]);
            resolveTypeOf(symbol);
            return;
        case atomParameter:
        case atomPointer:
        case atomTypeOf:
            resolveIdentifiersAndTypes(symbol[1]);
            resolveTypeOf(symbol);
            return;
        case atomBit:
        case atomBoolean:
        case atomByte:
        case atomCharacter:
        case atomInt:
        case atomString:
        case atomUInt:
        case atomVoid:
        case atomWord:
        case atomStringConstant:
        case atomIntegerConstant:
        case atomNull:
        case atomPrintFunction:
        case atomTrue:
        case atomFalse:
            resolveTypeOf(symbol);
            break;
        case atomFunction:
        case atomLogicalOr:
        case atomLogicalAnd:
            resolveIdentifiersAndTypes(symbol[1]);
            resolveIdentifiersAndTypes(symbol[2]);
            resolveTypeOf(symbol);
            break;
        case atomDot:
            resolveIdentifiersAndTypes(symbol[1]);
            resolveTypeOf(symbol);
            // TODO: look up right identifier in class symbol table
            break;
    }

    const SymbolTail* tail = symbol.tail();
    while (tail != 0) {
        resolveIdentifiersAndTypes(tail->head());
        tail = tail->tail();
    }
}

void resolveIdentifier(Symbol symbol)
{
    int label = labelOf(symbol);
    if (label != -1)
        return;
    switch (symbol.atom()) {
        case atomIdentifier:
            {
                Scope* scope = scopeOf(symbol);
                label = scope->resolveIdentifier(symbol[1].string(), spanOf(symbol));
                setLabel(symbol, label);
            }
            return;
        case atomTypeIdentifier:
            {
                Scope* scope = scopeOf(symbol);
                label = scope->resolveType(symbol[1].string(), spanOf(symbol));
                setLabel(symbol, label);
            }
            return;
        case atomFunctionCall:
            {
                Symbol function = symbol[1].symbol();
                if (function.atom() == atomIdentifier) {
                    Scope* scope = scopeOf(function);
                    SymbolList list;
                    SymbolArray arguments = symbol[2].array();
                    for (int i = 0; i < arguments.count(); ++i)
                        list.add(typeOf(arguments[i]));
                    int label = scope->resolveFunction(function[1].string(), list, spanOf(function));
                    setLabel(function, label);
                    return;
                }
            }
            break;
    }
}

void resolveTypeOf(Symbol symbol)
{
    Symbol type = typeOf(symbol);
    if (type.valid()) {
        if (type.atom() == atomAuto)
            spanOf(symbol).throwError(String("Cycle in type resolution")); // TODO - improve error message to explain cycle
        return;
    }
    setType(symbol, Symbol(atomAuto));
    switch (symbol.atom()) {
        case atomIdentifier:
        case atomTypeIdentifier:
            {
                resolveIdentifier(symbol);
                int label = labelOf(symbol);
                Symbol definition = Symbol::labelled(label);
                resolveTypeOf(definition);
                type = typeOf(definition);
            }
            break;
        case atomFunctionCall:
            {
                Symbol function = symbol[1].symbol();
                resolveTypeOf(function);
                type = typeOf(function)[1].symbol();  // return type
            }
            break;
        case atomVariableDefinitionStatement:
            type = symbol[1].symbol();
            if (type.atom() == atomAuto) {
                Symbol initializer = symbol[3].symbol();
                if (!initializer.valid()) {
                    static String error("Auto variable declarations must be initialized");
                    spanOf(symbol).throwError(error);
                }
                type = typeOf(initializer);
            }
            else {
                resolveTypeOf(type);
                type = typeOf(type);
            }
            break;
        case atomParameter:
            type = symbol[1].symbol();
            resolveTypeOf(type);
            type = typeOf(type);
            break;
        case atomFunctionDefinitionStatement:
            {
                Symbol returnType = symbol[1].symbol();
                resolveTypeOf(returnType);
                SymbolList list;
                SymbolArray parameters = symbol[3].array();
                for (int i = 0; i < parameters.count(); ++i) {
                    Symbol parameter = parameters[i].symbol();
                    resolveTypeOf(parameter);
                    list.add(typeOf(parameter));
                }
                type = Symbol(atomFunction, typeOf(returnType), SymbolArray(list));
            }
            break;
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
            {
                Symbol referentType = symbol[1].symbol();
                resolveTypeOf(referentType);
                type = Symbol(atomPointer, typeOf(referentType));
            }
            break;
        case atomFunction:
            {
                Symbol returnType = symbol[1].symbol();
                resolveTypeOf(returnType);
                SymbolList list;
                SymbolArray parameterTypes = symbol[2].array();
                for (int i = 0; i < parameterTypes.count(); ++i) {
                    Symbol parameterType = parameterTypes[i].symbol();
                    resolveTypeOf(parameterType);
                    list.add(typeOf(parameterType));
                }
                type = Symbol(atomFunction, typeOf(returnType), SymbolArray(list));
            }
            break;
        case atomTypeOf:
            {
                Symbol expression = symbol[1].symbol();
                resolveTypeOf(expression);
                type = typeOf(expression);
            }
            break;
        case atomLogicalOr:
        case atomLogicalAnd:
        case atomTrue:
        case atomFalse:
            type = Symbol(atomBoolean);
            break;
        case atomDot:
            resolveTypeOf(symbol[1].symbol());
            // TODO: look up right identifier in class symbol table
            break;
        case atomStringConstant:
            type = Symbol(atomString);
            break;
        case atomIntegerConstant:
            type = Symbol(atomInt);  // TODO: the type is actually one of several depending on the value
            break;
        case atomNull:
            type = Symbol(atomPointer);
            break;
        case atomPrintFunction:
            type = Symbol(atomFunction, Symbol(atomVoid), SymbolArray(Symbol(atomString)));
            break;
    }
    setType(symbol, type);
}

void resolveOffset(Symbol symbol)
{
    int offset = offsetOf(symbol);
    if (offset != -1)
        return;
    switch (symbol.atom()) {
        case atomBit:
        case atomBoolean:
        case atomByte:
        case atomCharacter:
        case atomInt:
        case atomPointer:
            offset = 4;

        case atomClass:
        case atomFunction:
        case atomString:
        case atomTypeIdentifier:
        case atomTypeOf:
        case atomUInt:
        case atomVoid:
        case atomWord:

        case atomLogicalOr:
        case atomLogicalAnd:
        case atomDot:

        case atomBitwiseOr:
        case atomBitwiseXor:
        case atomBitwiseAnd:
        case atomEqualTo:
        case atomNotEqualTo:
        case atomLessThanOrEqualTo:
        case atomGreaterThanOrEqualTo:
        case atomLessThan:
        case atomGreaterThan:
        case atomLeftShift:
        case atomRightShift:
        case atomAdd:
        case atomSubtract:
        case atomMultiply:
        case atomDivide:
        case atomModulo:
        case atomNot:
        case atomPositive:
        case atomNegative:
        case atomDereference:
        case atomAddressOf:
        case atomPower:
        case atomFunctionCall:

        case atomStringConstant:
        case atomIdentifier:
        case atomIntegerConstant:
        case atomTrue:
        case atomFalse:
        case atomNull:

        case atomParameter:

        case atomExpressionStatement:
        case atomFunctionDefinitionStatement:
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
        case atomCompoundStatement:
        case atomTypeAliasStatement:
        case atomNothingStatement:
        case atomIncrementStatement:
        case atomDecrementStatement:
        case atomIfStatement:
        case atomSwitchStatement:
        case atomReturnStatement:
        case atomIncludeStatement:
        case atomBreakStatement:
        case atomContinueStatement:
        case atomForeverStatement:
        case atomWhileStatement:
        case atomUntilStatement:
        case atomForStatement:

        case atomCase:
        case atomDefaultCase:
    }
    setOffset(symbol, offset);
}