//class SpanCache : public SymbolCache
//{
//public:
//    SpanCache(Span span) : _span(span) { }
//    Span span() const { return _span; }
//private:
//    Span _span;
//};
//
//Span spanOf(Symbol symbol) { return symbol.cache<SpanCache>()->span(); }
//
//SpanCache* newSpan(Span span) { return new SpanCache(span); }
//
//SpanCache* newSpan(Location start, Location end)
//{
//    return newSpan(Span(start, end));
//}
//
//SpanCache* newSpan(Symbol symbol) { return newSpan(spanOf(symbol)); }
//
//class ExpressionCache : public SpanCache
//{
//public:
//    ExpressionCache(Span span) : SpanCache(span) { }
//    void setType(Symbol type) { _type = type; }
//    Symbol type() const { return _type; }
//private:
//    Symbol _type;
//};
//
//Symbol typeOf(SymbolEntry expression)
//{
//    return expression.symbol().cache<ExpressionCache>()->type();
//}
//
//void setType(Symbol symbol, Symbol type)
//{
//    symbol.cache<ExpressionCache>()->setType(type);
//}
//
//class Scope;
//
//class IdentifierCache : public ExpressionCache
//{
//public:
//    IdentifierCache(Span span, SymbolLabel label = SymbolLabel())
//      : ExpressionCache(span), _label(label), _offset(-1)
//    { }
//    void setLabel(SymbolLabel label) { _label = label; }
//    SymbolLabel label() const { return _label; }
//    void setScope(Scope* scope) { _scope = scope; }
//    Scope* scope() { return _scope; }
//    void setOffset(int offset) { _offset = offset; }
//    int offset() const { return _offset; }
//private:
//    SymbolLabel _label;
//    Reference<Scope> _scope;
//    int _offset;
//};
//
//SymbolLabel labelOf(Symbol symbol)
//{
//    return symbol.cache<IdentifierCache>()->label();
//}
//
//void setLabel(Symbol symbol, SymbolLabel label)
//{
//    symbol.cache<IdentifierCache>()->setLabel(label);
//}
//
//Scope* scopeOf(Symbol symbol)
//{
//    return symbol.cache<IdentifierCache>()->scope();
//}
//
//void setOffset(Symbol symbol, int offset)
//{
//    symbol.cache<IdentifierCache>()->setOffset(offset);
//}
//
//int offsetOf(Symbol symbol)
//{
//    return symbol.cache<IdentifierCache>()->offset();
//}
//
//class TypeCache : public IdentifierCache
//{
//public:
//    TypeCache(Span span, int size = -1, int alignment = -1, SymbolLabel label = SymbolLabel())
//      : IdentifierCache(span, label) { }
//    void setSize(int size) { _size = size; }
//    int size() const { return _size; }
//    void setAlignment(int alignment) { _alignment = alignment; }
//    int alignment() const { return _alignment; }
//private:
//    int _size;
//    int _alignment;
//};
//
//void setSize(Symbol symbol, int size)
//{
//    symbol.cache<TypeCache>()->setSize(size);
//}
//
//int sizeOf(Symbol symbol)
//{
//    return symbol.cache<TypeCache>()->size();
//}
//
//void setAlignment(Symbol symbol, int alignment)
//{
//    symbol.cache<TypeCache>()->setAlignment(alignment);
//}
//
//int alignmentOf(Symbol symbol)
//{
//    return symbol.cache<TypeCache>()->alignment();
//}
//
//class FunctionDefinitionCache : public IdentifierCache
//{
//public:
//    FunctionDefinitionCache(Span span)
//      : IdentifierCache(span, SymbolLabel()), _compiling(false)
//    { }
//    void setCompilingFlag(bool compiling) { _compiling = compiling; }
//    bool getCompilingFlag() const { return _compiling; }
//    void setBasicBlockLabel(SymbolLabel label) { _basicBlockLabel = label; }
//    SymbolLabel getBasicBlockLabel() const { return _basicBlockLabel; }
//private:
//    bool _compiling;
//    SymbolLabel _basicBlockLabel;
//};
//
//SymbolLabel basicBlockLabelOf(Symbol symbol)
//{
//    return symbol.cache<FunctionDefinitionCache>()->getBasicBlockLabel();
//}
//
//void setBasicBlockLabel(Symbol symbol, SymbolLabel label)
//{
//    symbol.cache<FunctionDefinitionCache>()->setBasicBlockLabel(label);
//}
//
//class SymbolName : public ReferenceCounted
//{
//public:
//    virtual SymbolLabel resolveIdentifier(Span span) = 0;
//};
//
//class VariableName : public SymbolName
//{
//public:
//    VariableName(SymbolLabel label) : _label(label) { }
//    SymbolLabel resolveIdentifier(Span span)
//    {
//        return _label;
//    }
//    int frameOffset() const { return _frameOffset; }
//    void setframeOffset(int frameOffset) { _frameOffset = frameOffset; }
//private:
//    SymbolLabel _label;
//    int _frameOffset;
//};
//
//class FunctionName : public SymbolName
//{
//public:
//    SymbolLabel resolveIdentifier(Span span)
//    {
//        if (_overloads.count() > 1)
//            span.throwError(_name + " is an overloaded function - I don't "
//                "know which overload you mean.");
//        return _label;
//    }
//    void addOverload(SymbolLabel label)
//    {
//        Symbol functionDefinition = label.target();
//        Symbol type = typeOf(functionDefinition);
//        SymbolArray types = type[2].array();
//        if (_overloads.hasKey(types))
//            spanOf(functionDefinition).throwError("This overload has already "
//                "been defined.");
//        _overloads.add(types, label);
//        if (_overloads.count() == 1)
//            _argumentTypes = types;
//    }
//    bool hasOverload(SymbolArray argumentTypes)
//    {
//        return _overloads.hasKey(argumentTypes);
//    }
//    SymbolLabel lookUpOverload(SymbolArray argumentTypes)
//    {
//        return _overloads[argumentTypes];
//    }
//
//private:
//    HashTable<SymbolArray, SymbolLabel> _overloads;
//    SymbolLabel _label;
//    String _name;
//    SymbolArray _argumentTypes;
//};
//
//String identifierToString(SymbolEntry identifier)
//{
//    if (!identifier.isSymbol())
//        return identifier.string();
//    return "operator" + atomToString(identifier.symbol().atom());
//}
//
//String typesToString(SymbolArray array);
//
//class Scope : public ReferenceCounted
//{
//public:
//    Scope(Scope* outer, bool functionScope = false)
//      : _outer(outer)
//    {
//        if (functionScope)
//            _functionScope = this;
//        else
//            _functionScope = outer->_functionScope;
//    }
//    Scope* outer() const { return _outer; }
//    Scope* functionScope() const { return _functionScope; }
//    void addVariable(SymbolEntry identifier, SymbolLabel label, Span span)
//    {
//        if (_symbolTable.hasKey(identifier))
//            span.throwError(identifierToString(identifier) +
//                " is already defined");
//        _symbolTable.add(identifier, new VariableName(label));
//    }
//    SymbolLabel resolveIdentifier(SymbolEntry identifier, Span span)
//    {
//        if (!_symbolTable.hasKey(identifier)) {
//            if (_outer != 0)
//                return _outer->resolveIdentifier(identifier, span);
//            span.throwError("Undefined symbol " +
//                identifierToString(identifier));
//        }
//        return _symbolTable[identifier]->resolveIdentifier(span);
//    }
//    void addFunction(SymbolEntry identifier, SymbolLabel label, Span span)
//    {
//        FunctionName* functionName;
//        if (_symbolTable.hasKey(identifier)) {
//            Reference<SymbolName> symbol = _symbolTable[identifier];
//            functionName = dynamic_cast<FunctionName*>(
//                static_cast<SymbolName*>(symbol));
//            if (functionName == 0)
//                span.throwError(identifierToString(identifier) +
//                    " is already defined as a variable");
//        }
//        else {
//            functionName = new FunctionName;
//            _symbolTable.add(identifier, functionName);
//        }
//        functionName->addOverload(label);
//    }
//    void addType(SymbolEntry identifier, SymbolLabel label, Span span)
//    {
//        if (_typeTable.hasKey(identifier))
//            span.throwError(identifierToString(identifier) +
//                " has already been defined.");
//        _typeTable.add(identifier, label);
//    }
//    SymbolLabel resolveFunction(SymbolEntry identifier,
//        SymbolArray argumentTypes, Span span)
//    {
//        if (!_symbolTable.hasKey(identifier)) {
//            if (_outer == 0)
//                span.throwError("Undefined function " +
//                    identifierToString(identifier));
//            return _outer->resolveFunction(identifier, argumentTypes, span);
//        }
//        Reference<SymbolName> symbol = _symbolTable[identifier];
//        FunctionName* functionName = dynamic_cast<FunctionName*>(
//            static_cast<SymbolName*>(symbol));
//        if (functionName == 0)
//            span.throwError(identifierToString(identifier) +
//                " is not a function");
//        if (!functionName->hasOverload(argumentTypes))
//            span.throwError(identifierToString(identifier) +
//                " has no overload with argument types " +
//                typesToString(argumentTypes));
//        return functionName->lookUpOverload(argumentTypes);
//    }
//    SymbolLabel resolveType(SymbolEntry identifier, Span span)
//    {
//        if (!_typeTable.hasKey(identifier)) {
//            if (_outer == 0)
//                span.throwError("Undefined type " +
//                    identifierToString(identifier));
//            return _outer->resolveType(identifier, span);
//        }
//        return _typeTable[identifier];
//    }
//    void setStackOffset(int offset) { _offset = offset; }
//    int getStackOffset() { return _offset; }
//private:
//    HashTable<SymbolEntry, Reference<SymbolName> > _symbolTable;
//    HashTable<SymbolEntry, SymbolLabel> _typeTable;
//    Scope* _outer;
//    Scope* _functionScope;
//    int _offset;
//};
//
//Scope* setScopes(SymbolEntry entry, Scope* scope)
//{
//    if (entry.isArray()) {
//        SymbolArray array = entry.array();
//        for (int i = 0; i < array.count(); ++i)
//            scope = setScopes(array[i], scope);
//        return scope;
//    }
//    if (!entry.isSymbol())
//        return scope;
//    Symbol symbol = entry.symbol();
//    Reference<Scope> inner = scope;
//    switch (symbol.atom()) {
//        case atomFunctionDefinitionStatement:
//            inner = new Scope(scope, true);
//            scope->functionScope()->addFunction(symbol[2], labelOf(symbol), spanOf(symbol));
//            break;
//        case atomVariableDefinitionStatement:
//            scope = new Scope(scope);
//            inner = scope;
//            scope->addVariable(symbol[2], labelOf(symbol), spanOf(symbol));
//            break;
//        case atomParameter:
//            scope = new Scope(scope);
//            inner = scope;
//            scope->addVariable(symbol[2], labelOf(symbol), spanOf(symbol));
//            break;
//        case atomTypeAliasStatement:
//            scope->functionScope()->addType(symbol[2], labelOf(symbol), spanOf(symbol));
//            break;
//        case atomIfStatement:
//            setScopes(symbol[1], scope);
//            inner = new Scope(scope, true);
//            setScopes(symbol[2], inner);
//            inner = new Scope(scope, true);
//            setScopes(symbol[3], inner);
//            return scope;
//        case atomCompoundStatement:
//        case atomForeverStatement:
//        case atomCase:
//        case atomDefaultCase:
//            inner = new Scope(scope, true);
//            break;
//        case atomWhileStatement:
//        case atomUntilStatement:
//            inner = new Scope(scope, true);
//            setScopes(symbol[1], inner);
//            setScopes(symbol[2], scope);
//            inner = new Scope(scope, true);
//            setScopes(symbol[3], inner);
//            inner = new Scope(scope, true);
//            setScopes(symbol[4], inner);
//            return scope;
//        case atomForStatement:
//            {
//                inner = new Scope(scope, true);
//                setScopes(symbol[1], inner);
//                setScopes(symbol[2], inner);
//                Reference<Scope> inner2 = new Scope(inner, true);
//                setScopes(symbol[3], inner2);
//                Reference<Scope> inner3 = new Scope(inner2, true);
//                setScopes(symbol[4], inner3);
//                inner3 = new Scope(inner3, true);
//                setScopes(symbol[5], inner3);
//            }
//            return scope;
//        case atomIdentifier:
//        case atomTypeConstructorIdentifier:
//            symbol.cache<IdentifierCache>()->setScope(scope);
//            break;
//        case atomLabelStatement:
//            scope->functionScope()->addVariable(symbol[1], labelOf(symbol), spanOf(symbol));
//            break;
//    }
//    const SymbolTail* tail = symbol.tail();
//    while (tail != 0) {
//        setScopes(tail->head(), inner);
//        tail = tail->tail();
//    }
//    return scope;
//}
//
//// resolve offsets in "symbol"
//int resolveOffsets(Symbol symbol, int o, int* highWaterMark)
//{
//    int offset = offsetOf(symbol);
//    if (offset != -1)
//        return 0;
//    switch (symbol.atom()) {
//        case atomFunctionDefinitionStatement:
//            {
//                SymbolArray parameters = symbol[3].array();
//                offset = 0;
//                for (int i = 0; i < parameters.count(); ++i) {
//                    Symbol parameter = parameters[i];
//                    Symbol type = typeOf(parameter);
//                    offset &= -alignmentOf(type);
//                    setOffset(parameter, offset + 4);
//                    offset += sizeOf(typeOf(parameter));
//                    // TODO: Move the following line after the loop to compress the arguments to a Class-like layout
//                    offset = (offset + 3) & -4;  // Minimum stack alignment is 4 words
//                }
//                // TODO: adjust offset for variables and outgoing parameters
//                int h = 0;
//                resolveOffsets(symbol[4].symbol(), 0, &h);
//                setOffset(symbol, h);
//            }
//            return o;
//        case atomVariableDefinitionStatement:
//            {
//                Symbol type = symbol[1].symbol();
//                setOffset(symbol, o);
//                int size = sizeOf(type);
//                size = (size + 3) & -4;
//                o += size;
//                *highWaterMark = max(o, *highWaterMark);
//            }
//            return o;
//        case atomIfStatement:
//            resolveOffsets(symbol[2].symbol(), o, highWaterMark);
//            resolveOffsets(symbol[3].symbol(), o, highWaterMark);
//            return o;
//        case atomCompoundStatement:
//            {
//                SymbolArray statements = symbol[1].array();
//                offset = o;
//                for (int i = 0; i < statements.count(); ++i)
//                    offset = resolveOffsets(statements[i].symbol(), offset, highWaterMark);
//            }
//            return offset;
//        case atomForeverStatement:
//            resolveOffsets(symbol[1].symbol(), o, highWaterMark);
//            return o;
//        case atomWhileStatement:
//        case atomUntilStatement:
//            resolveOffsets(symbol[1].symbol(), o, highWaterMark);
//            resolveOffsets(symbol[3].symbol(), o, highWaterMark);
//            resolveOffsets(symbol[4].symbol(), o, highWaterMark);
//            return o;
//        case atomForStatement:
//            offset = resolveOffsets(symbol[1].symbol(), o, highWaterMark);
//            resolveOffsets(symbol[3].symbol(), offset, highWaterMark);
//            resolveOffsets(symbol[4].symbol(), offset, highWaterMark);
//            resolveOffsets(symbol[5].symbol(), offset, highWaterMark);
//            return o;
//        case atomSwitchStatement:
//            {
//                resolveOffsets(symbol[2].symbol(), o, highWaterMark);
//                SymbolArray cases = symbol[3].array();
//                for (int i = 0; i < cases.count(); ++i)
//                    resolveOffsets(cases[i], o, highWaterMark);
//            }
//            return o;
//        case atomDefaultCase:
//            resolveOffsets(symbol[1].symbol(), o, highWaterMark);
//            return o;
//        case atomCase:
//            resolveOffsets(symbol[2].symbol(), o, highWaterMark);
//            return o;
//    }
//    return o;
//}
//
//void resolveIdentifier(Symbol identifier);
//void resolveTypeOf(Symbol symbol);
//
//void resolveIdentifiersAndTypes(SymbolEntry entry)
//{
//    if (entry.isArray()) {
//        SymbolArray array = entry.array();
//        for (int i = 0; i < array.count(); ++i)
//            resolveIdentifiersAndTypes(array[i]);
//    }
//    if (!entry.isSymbol())
//        return;
//    Symbol symbol = entry.symbol();
//    switch (symbol.atom()) {
//        case atomIdentifier:
//        case atomTypeConstructorIdentifier:
//            resolveIdentifier(symbol);
//            resolveTypeOf(symbol);
//            return;
//        case atomFunctionCall:
//            {
//                SymbolArray arguments = symbol[2].array();
//                for (int i = 0; i < arguments.count(); ++i)
//                    resolveIdentifiersAndTypes(arguments[i]);
//                Symbol function = symbol[1].symbol();
//                if (function.atom() == atomIdentifier)
//                    resolveIdentifier(symbol);
//                else
//                    resolveIdentifiersAndTypes(function);
//                resolveTypeOf(symbol);
//                int highWaterMark = 0;
//                resolveOffsets(symbol, 0, &highWaterMark);
//            }
//            return;
//        case atomFunctionDefinitionStatement:
//            resolveIdentifiersAndTypes(symbol[1]);
//            resolveIdentifiersAndTypes(symbol[3]);
//            resolveIdentifiersAndTypes(symbol[4]);
//            resolveTypeOf(symbol);
//            return;
//        case atomVariableDefinitionStatement:
//            resolveIdentifiersAndTypes(symbol[1]);
//            resolveIdentifiersAndTypes(symbol[3]);
//            resolveTypeOf(symbol);
//            return;
//        case atomParameter:
//        case atomPointer:
//        case atomTypeOf:
//            resolveIdentifiersAndTypes(symbol[1]);
//            resolveTypeOf(symbol);
//            return;
//        case atomBit:
//        case atomBoolean:
//        case atomByte:
//        case atomCharacter:
//        case atomInt:
//        case atomString:
//        case atomUInt:
//        case atomVoid:
//        case atomWord:
//        case atomStringConstant:
//        case atomIntegerConstant:
//        case atomNull:
//        case atomPrintFunction:
//        case atomTrue:
//        case atomFalse:
//            resolveTypeOf(symbol);
//            break;
//        case atomFunction:
//        case atomLogicalOr:
//        case atomLogicalAnd:
//            resolveIdentifiersAndTypes(symbol[1]);
//            resolveIdentifiersAndTypes(symbol[2]);
//            resolveTypeOf(symbol);
//            break;
//        case atomDot:
//            resolveIdentifiersAndTypes(symbol[1]);
//            resolveTypeOf(symbol);
//            // TODO: look up right identifier in class symbol table
//            break;
//        case atomLabelStatement:
//            resolveTypeOf(symbol);
//            return;
//    }
//
//    const SymbolTail* tail = symbol.tail();
//    while (tail != 0) {
//        resolveIdentifiersAndTypes(tail->head());
//        tail = tail->tail();
//    }
//}
//
//void resolveIdentifier(Symbol symbol)
//{
//    SymbolLabel label = labelOf(symbol);
//    if (label.target().valid())
//        return;
//    switch (symbol.atom()) {
//        case atomIdentifier:
//            {
//                Scope* scope = scopeOf(symbol);
//                label = scope->resolveIdentifier(symbol[1], spanOf(symbol));
//                setLabel(symbol, label);
//            }
//            return;
//        case atomTypeConstructorIdentifier:
//            {
//                Scope* scope = scopeOf(symbol);
//                label = scope->resolveType(symbol[1], spanOf(symbol));
//                setLabel(symbol, label);
//            }
//            return;
//        case atomFunctionCall:
//            {
//                Symbol function = symbol[1].symbol();
//                if (function.atom() == atomIdentifier) {
//                    Scope* scope = scopeOf(function);
//                    SymbolList list;
//                    SymbolArray arguments = symbol[2].array();
//                    for (int i = 0; i < arguments.count(); ++i)
//                        list.add(typeOf(arguments[i]));
//                    label = scope->resolveFunction(function[1], list, spanOf(function));
//                    setLabel(function, label);
//                    return;
//                }
//            }
//            break;
//    }
//}
//
//void resolveTypeOf(Symbol symbol)
//{
//    Symbol type = typeOf(symbol);
//    if (type.valid()) {
//        if (type.atom() == atomAuto) {
//            // TODO - improve error message to explain cycle
//            spanOf(symbol).throwError("Cycle in type resolution");
//        }
//        return;
//    }
//    setType(symbol, Symbol(atomAuto));
//    switch (symbol.atom()) {
//        case atomIdentifier:
//        case atomTypeConstructorIdentifier:
//            {
//                resolveIdentifier(symbol);
//                SymbolLabel label = labelOf(symbol);
//                Symbol definition = label.target();
//                resolveTypeOf(definition);
//                type = typeOf(definition);
//            }
//            break;
//        case atomFunctionCall:
//            {
//                Symbol function = symbol[1].symbol();
//                resolveTypeOf(function);
//                type = typeOf(function)[1].symbol();  // return type
//            }
//            break;
//        case atomVariableDefinitionStatement:
//            type = symbol[1].symbol();
//            if (type.atom() == atomAuto) {
//                Symbol initializer = symbol[3].symbol();
//                if (!initializer.valid())
//                    spanOf(symbol).throwError(
//                        "Auto variable declarations must be initialized");
//                type = typeOf(initializer);
//            }
//            else {
//                resolveTypeOf(type);
//                type = typeOf(type);
//            }
//            break;
//        case atomParameter:
//            type = symbol[1].symbol();
//            resolveTypeOf(type);
//            type = typeOf(type);
//            break;
//        case atomFunctionDefinitionStatement:
//            {
//                Symbol returnType = symbol[1].symbol();
//                resolveTypeOf(returnType);
//                SymbolList list;
//                SymbolArray parameters = symbol[3].array();
//                for (int i = 0; i < parameters.count(); ++i) {
//                    Symbol parameter = parameters[i].symbol();
//                    resolveTypeOf(parameter);
//                    list.add(typeOf(parameter));
//                }
//                type = Symbol(atomFunction, typeOf(returnType), SymbolArray(list));
//            }
//            break;
//        case atomBit:
//        case atomBoolean:
//        case atomByte:
//        case atomCharacter:
//        case atomInt:
//        case atomString:
//        case atomUInt:
//        case atomVoid:
//        case atomWord:
//            type = symbol;
//            break;
//        case atomPointer:
//            {
//                Symbol referentType = symbol[1].symbol();
//                resolveTypeOf(referentType);
//                type = Symbol(atomPointer, typeOf(referentType));
//            }
//            break;
//        case atomFunction:
//            {
//                Symbol returnType = symbol[1].symbol();
//                resolveTypeOf(returnType);
//                SymbolList list;
//                SymbolArray parameterTypes = symbol[2].array();
//                for (int i = 0; i < parameterTypes.count(); ++i) {
//                    Symbol parameterType = parameterTypes[i].symbol();
//                    resolveTypeOf(parameterType);
//                    list.add(typeOf(parameterType));
//                }
//                type = Symbol(atomFunction, typeOf(returnType), SymbolArray(list));
//            }
//            break;
//        case atomTypeOf:
//            {
//                Symbol expression = symbol[1].symbol();
//                resolveTypeOf(expression);
//                type = typeOf(expression);
//            }
//            break;
//        case atomLogicalOr:
//        case atomLogicalAnd:
//        case atomTrue:
//        case atomFalse:
//            type = Symbol(atomBoolean);
//            break;
//        case atomDot:
//            resolveTypeOf(symbol[1].symbol());
//            // TODO: look up right identifier in class symbol table
//            break;
//        case atomStringConstant:
//            type = Symbol(atomString);
//            break;
//        case atomIntegerConstant:
//            type = Symbol(atomInt);  // TODO: the type is actually one of several depending on the value
//            break;
//        case atomNull:
//            type = Symbol(atomPointer);
//            break;
//        case atomPrintFunction:
//            type = Symbol(atomFunction, Symbol(atomVoid), SymbolArray(Symbol(atomString)));
//            break;
//        case atomLabelStatement:
//            type = Symbol(atomLabel);
//            break;
//    }
//    setType(symbol, type);
//}
