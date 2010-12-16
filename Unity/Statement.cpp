Symbol parseExpressionStatement(CharacterSource* source)
{
    CharacterSource s = *source;
    Symbol expression = parseExpression(&s);
    if (!expression.valid())
        return Symbol();
    DiagnosticSpan span;
    if (!Space::parseCharacter(&s, ';', span))
        return Symbol();
    *source = s;
    if (expression.atom() != atomFunctionCall) {
        static String error("Statement has no effect");
        source->location().throwError(error);
    }
    return Symbol(atomExpressionStatement, DiagnosticSpan(expression.span().start(), span.end()), expression);
}

Symbol parseParameter(CharacterSource* source)
{
    DiagnosticLocation location = source->location();
    Symbol typeSpecifier = parseTypeSpecifier(source);
    if (!typeSpecifier.valid()) {
        static String error("Expected type specifier");
        source->location().throwError(error);
    }
    Symbol name = parseIdentifier(source);
    if (!name.valid()) {
        static String error("Expected identifier");
        source->location().throwError(error);
    }
    return Symbol(atomParameter, DiagnosticSpan(location, name.span().end()), typeSpecifier, name);
}
//        Type type() { return _typeSpecifier->type(); }
//        void addToScope(Scope* scope)
//        {
//            scope->addVariable(_name->name(), type(), _location);
//        }
//        void setValue(Value value, Scope* scope)
//        {
//            Reference<Variable> variable = scope->resolveSymbolName(_name->name(), _location);
//            variable->setValue(value);
//        }
//    private:
//        Argument(Reference<TypeSpecifier> typeSpecifier, Reference<Identifier> name, DiagnosticLocation location)
//          : _typeSpecifier(typeSpecifier), _name(name), _location(location)
//        { }
//        Reference<TypeSpecifier> _typeSpecifier;
//        Reference<Identifier> _name;
//        DiagnosticLocation _location;
//    };

SymbolList parseParameterList2(Symbol parameter, CharacterSource* source)
{
    DiagnosticSpan span;
    if (!Space::parseCharacter(source, ',', span))
        return SymbolList(parameter);
    Symbol parameter2 = parseParameter(source);
    if (!parameter2.valid()) {
        static String error("Type specifier expected");
        source->location().throwError(error);
    }
    return SymbolList(parameter, parseTypeListSpecifier2(parameter2, source));
}

SymbolList parseParameterList(CharacterSource* source)
{
    Symbol parameter = parseParameter(source);
    if (!parameter.valid())
        return SymbolList();
    return parseParameterList2(parameter, source);
}

Symbol parseFunctionDefinitionStatement(CharacterSource* source)
{
    DiagnosticLocation location = source->location();
    CharacterSource s = *source;
    Symbol returnTypeSpecifier = parseTypeSpecifier(&s);
    if (!returnTypeSpecifier.valid())
        return Symbol();
    Symbol name = parseIdentifier(&s);
    if (!name.valid())
        return Symbol();
    DiagnosticSpan span;
    if (!Space::parseCharacter(&s, '(', span))
        return Symbol();
    *source = s;
    SymbolList parameterList = parseParameterList(source);
    Space::assertCharacter(source, ')');

    static String from("from");
    if (Space::parseKeyword(source, from, span)) {
        Symbol dll = parseExpression(source);
        if (dll.valid())
            return Symbol(
                atomFunctionDefinitionStatement,
                DiagnosticSpan(location, dll.span().end()),
                returnTypeSpecifier,
                name,
                parameterList,
                Symbol(atomFromStatement, DiagnosticSpan(span.start(), dll.span().end()), dll));
        static String error("Expected expression");
        source->location().throwError(error);
    }
    Symbol statement = parseStatement(source);
    if (!statement.valid()) {
        static String error("Expected statement");
        source->location().throwError(error);
    }
    return Symbol(
        atomFunctionDefinitionStatement,
        DiagnosticSpan(location, statement.span().end()),
        returnTypeSpecifier,
        name,
        parameterList,
        statement);
}
//    void resolveTypes()
//    {
//        _returnType = _returnTypeSpecifier->type();
//        TypeList typeList;
//        for (int i = 0; i < _arguments.count(); ++i) {
//            _arguments[i]->addToScope(_scope);
//            typeList.push(_arguments[i]->type());
//        }
//        typeList.finalize();
//        _scope->outer()->addFunction(_name->name(), typeList, this, _location);
//    }
//    ExtricationStatement* run(Stack<Value>* stack) { return 0; }
//    virtual Type returnType() const { return _returnTypeSpecifier->type(); }
//    virtual void call(Stack<Value>* stack)
//    {
//        if (_statement.valid()) {
//            for (int i = 0; i < _arguments.count(); ++i)
//                _arguments[i]->setValue(stack->pop(), _scope);
//            _statement->run(stack);
//        }
//    }

Symbol parseVariableDefinitionStatement(CharacterSource* source)
{
    //Reference<Scope> inner = new Scope(scope);
    CharacterSource s = *source;
    Symbol typeSpecifier = parseTypeSpecifier(&s);
    if (!typeSpecifier.valid())
        return Symbol();
    Symbol identifier = parseIdentifier(&s);
    if (!identifier.valid())
        return Symbol();
    *source = s;
    Symbol initializer;
    DiagnosticSpan span;
    if (Space::parseCharacter(source, '=', span)) {
        initializer = parseExpression(source);
        if (!initializer.valid()) {
            static String expression("Expected expression");
            source->location().throwError(expression);
        }
    }
    DiagnosticLocation end = Space::assertCharacter(source, ';');
    return Symbol(
        atomVariableDefinitionStatement,
        DiagnosticSpan(typeSpecifier.span().start(), end),
        typeSpecifier,
        identifier,
        initializer);
}
//    void resolveTypes()
//    {
//        _typeSpecifier->type();
//    }
//    void compile()
//    {
//        Type type = _typeSpecifier->type();
//        AutoTypeSpecifier* autoTypeSpecifier = dynamic_cast<AutoTypeSpecifier*>(static_cast<TypeSpecifier*>(_typeSpecifier));
//        if (autoTypeSpecifier != 0)
//            type = _initializer->type();
//        _initializer->compile();
//        _variable = _scope->outer()->addVariable(_identifier->name(), type, _location);
//    }
//    ExtricationStatement* run(Stack<Value>* stack)
//    {
//        if (_initializer.valid()) {
//            _initializer->push(stack);
//            _variable->setValue(stack->pop());
//        }
//        return 0;
//    }

Symbol parseAssignmentStatement(CharacterSource* source)
{
    CharacterSource s = *source;
    Symbol lValue = parseExpression(&s);
    DiagnosticLocation operatorLocation = s.location();
    if (!lValue.valid())
        return Symbol();
    int type = 0;
    static String addAssignment("+=");
    static String subtractAssignment("-=");
    static String multiplyAssignment("*=");
    static String divideAssignment("/=");
    static String moduloAssignment("%=");
    static String shiftLeftAssignment("<<=");
    static String shiftRightAssignment(">>=");
    static String andAssignment("&=");
    static String orAssignment("|=");
    static String xorAssignment("~=");
    static String powerAssignment("^=");

    DiagnosticSpan span;
    if (Space::parseCharacter(&s, '=', span))
        type = 1;
    else if (Space::parseOperator(&s, addAssignment, span))
        type = 2;
    else if (Space::parseOperator(&s, subtractAssignment, span))
        type = 3;
    else if (Space::parseOperator(&s, multiplyAssignment, span))
        type = 4;
    else if (Space::parseOperator(&s, divideAssignment, span))
        type = 5;
    else if (Space::parseOperator(&s, moduloAssignment, span))
        type = 6;
    else if (Space::parseOperator(&s, shiftLeftAssignment, span))
        type = 7;
    else if (Space::parseOperator(&s, shiftRightAssignment, span))
        type = 8;
    else if (Space::parseOperator(&s, andAssignment, span))
        type = 9;
    else if (Space::parseOperator(&s, orAssignment, span))
        type = 10;
    else if (Space::parseOperator(&s, xorAssignment, span))
        type = 11;
    else if (Space::parseOperator(&s, powerAssignment, span))
        type = 12;
    if (type == 0)
        return Symbol();

    *source = s;
    Symbol e = parseExpression(source);
    if (!e.valid()) {
        static String expression("Expected expression");
        source->location().throwError(expression);
    }
    DiagnosticLocation end = Space::assertCharacter(source, ';');

    DiagnosticSpan span(lValue.span().start(), end);
    switch (type) {
        case 1:
            return Symbol(atomAssignmentStatement, span, lValue, e);
        case 2:
            return Symbol(atomAddAssignmentStatement, span, lValue, e);
        case 3:
            return Symbol(atomSubtractAssignmentStatement, span, lValue, e);
        case 4:
            return Symbol(atomMultiplyAssignmentStatement, span, lValue, e);
        case 5:
            return Symbol(atomDivideAssignmentStatement, span, lValue, e);
        case 6:
            return Symbol(atomModuloAssignmentStatement, span, lValue, e);
        case 7:
            return Symbol(atomShiftLeftAssignmentStatement, span, lValue, e);
        case 8:
            return Symbol(atomShiftRightAssignmentStatement, span, lValue, e);
        case 9:
            return Symbol(atomAndAssignmentStatement, span, lValue, e);
        case 10:
            return Symbol(atomOrAssignmentStatement, span, lValue, e);
        case 11:
            return Symbol(atomXorAssignmentStatement, span, lValue, e);
        case 12:
            return Symbol(atomPowerAssignmentStatement, span, lValue, e);
    }
    return Symbol();
}
//    void resolveTypes() { }
//    void compile()
//    {
//        _lValue->compile();
//        _value->compile();
//        if (!_lValue->isLValue()) {
//            static String error("LValue required");
//            _location.throwError(error);
//        }
//        Type l = _lValue->type();
//        Type r = _value->type();
//        if (l != r) {
//            static String error1("Cannot convert ");
//            static String error2(" to ");
//            _location.throwError(error1 + l.toString() + error2 + r.toString());
//        }
//    }

SymbolList parseStatementSequence(CharacterSource* source)
{
    Symbol statement = parseStatement(source);
    if (!statement.valid())
        return SymbolList();
    return SymbolList(statement, parseStatementSequence(source));
}
//    void resolveTypes()
//    {
//        for (int i = 0; i < _statements.count(); ++i)
//            _statements[i]->resolveTypes();
//    }
//    void compile()
//    {
//        for (int i = 0; i < _statements.count(); ++i)
//            _statements[i]->compile();
//    }
//    ExtricationStatement* run(Stack<Value>* stack)
//    {
//        for (int i = 0; i < _statements.count(); ++i) {
//            ExtricationStatement* statement = _statements[i]->run(stack);
//            if (statement != 0)
//                return statement;
//        }
//        return 0;
//    }

Symbol parseCompoundStatement(CharacterSource* source)
{
    DiagnosticSpan span;
    if (!Space::parseCharacter(source, '{', span))
        return Symbol();
    SymbolList sequence = parseStatementSequence(source);
    DiagnosticLocation end = Space::assertCharacter(source, '}');
    return Symbol(atomCompoundStatement, DiagnosticSpan(span.start(), end), sequence);
}
//    void resolveTypes()
//    {
//        _sequence->resolveTypes();
//    }
//    void compile()
//    {
//        _sequence->compile();
//    }
//    ExtricationStatement* run(Stack<Value>* stack)
//    {
//        return _sequence->run(stack);
//    }

Symbol parseTypeAliasStatement(CharacterSource* source)
{
    CharacterSource s = *source;
    CharacterSource s2 = s;
    Symbol typeIdentifier = parseTypeIdentifier(&s);
    if (!typeIdentifier.valid())
        return Symbol();
    DiagnosticSpan span;
    if (!Space::parseCharacter(&s, '=', span))
        return Symbol();
    *source = s;
    Symbol typeSpecifier = parseTypeSpecifier(source);
    DiagnosticLocation end = Space::assertCharacter(source, ';');
    return Symbol(atomTypeAliasStatement, DiagnosticSpan(typeIdentifier.span().start(), end), typeIdentifier, typeSpecifier);
}
//    void resolveTypes()
//    {
//        _typeIdentifier->type();
//    }
//    Type type() { return _typeSpecifier->type(); }

Symbol parseNothingStatement(CharacterSource* source)
{
    static String nothing("nothing");
    DiagnosticSpan span;
    if (!Space::parseKeyword(source, nothing, span))
        return Symbol();
    DiagnosticLocation end = Space::assertCharacter(source, ';');
    return Symbol(atomNothingStatement, DiagnosticSpan(span.start(), end));
}

Symbol parseIncrementStatement(CharacterSource* source)
{
    static String incrementOperator("++");
    DiagnosticSpan span;
    if (!Space::parseOperator(source, incrementOperator, span))
        return Symbol();
    CharacterSource s = *source;
    Symbol lValue = parseExpression(&s);
    *source = s;
    DiagnosticLocation end = Space::assertCharacter(source, ';');
    return Symbol(atomIncrementStatement, DiagnosticSpan(span.start(), end), lValue);
}
//    void resolveTypes() { _lValue->type(); }
//    void compile()
//    {
//        _lValue->compile();
//        if (_lValue->isLValue()) {
//            static String error("LValue required");
//            _location.throwError(error);
//        }
//    }
//    ExtricationStatement* run(Stack<Value>* stack)
//    {
//        _lValue->push(stack);
//        _lValue->variable(stack)->setValue(Value(stack->pop().getInt() + 1));
//        return 0;
//    }

Symbol parseDecrementStatement(CharacterSource* source)
{
    static String decrementOperator("--");
    DiagnosticSpan span;
    if (!Space::parseOperator(source, decrementOperator, span))
        return Symbol();
    CharacterSource s = *source;
    Symbol lValue = parseExpression(&s);
    *source = s;
    DiagnosticLocation end = Space::assertCharacter(source, ';');
    return Symbol(atomIncrementStatement, DiagnosticSpan(span.start(), end), lValue);
}
//    void resolveTypes() { _lValue->type(); }
//    void compile()
//    {
//        _lValue->compile();
//        if (_lValue->isLValue()) {
//            static String error("LValue required");
//            _location.throwError(error);
//        }
//    }
//    ExtricationStatement* run(Stack<Value>* stack)
//    {
//        _lValue->push(stack);
//        _lValue->variable(stack)->setValue(Value(stack->pop().getInt() - 1));
//        return 0;
//    }

Symbol parseConditionalStatement2(CharacterSource* source, DiagnosticLocation start, bool unlessStatement)
{
    static String elseKeyword("else");
    static String elseIfKeyword("elseIf");
    static String elseUnlessKeyword("elseUnless");
    Space::assertCharacter(source, '(');
    DiagnosticSpan span;
    Symbol condition = parseExpression(source);
    if (!condition.valid()) {
        static String error("Expected expression");
        source->location().throwError(error);
    }
    Space::assertCharacter(source, ')');
    Symbol conditionedStatement = parseStatement(source);
    static String expectedStatement("Expected statement");
    if (!conditionedStatement.valid())
        source->location().throwError(expectedStatement);
    Symbol elseStatement;
    if (Space::parseKeyword(source, elseKeyword, span)) {
        elseStatement = parseStatement(source);
        if (!elseStatement.valid())
            source->location().throwError(expectedStatement);
    }
    else
        if (Space::parseKeyword(source, elseIfKeyword, span))
            elseStatement = parseConditionalStatement2(source, span.start(), false);
        else
            if (Space::parseKeyword(source, elseUnlessKeyword, span))
                elseStatement = parseConditionalStatement2(source, span.start(), true);
    if (unlessStatement)
        return Symbol(atomIfStatement, DiagnosticSpan(start, elseStatement.span().end()), condition, elseStatement, conditionedStatement);
    return Symbol(atomIfStatement, DiagnosticSpan(start, elseStatement.span().end()), condition, conditionedStatement, elseStatement);
}

Symbol parseConditionalStatement(CharacterSource* source)
{
    static String ifKeyword("if");
    static String unlessKeyword("unless");
    DiagnosticSpan span;
    if (Space::parseKeyword(source, ifKeyword, span))
        return parseConditionalStatement2(source, span.start(), false);
    if (Space::parseKeyword(source, unlessKeyword, span))
        return parseConditionalStatement2(source, span.start(), true);
    return Symbol();
}
//    void resolveTypes()
//    {
//        if (_trueStatement.valid())
//            _trueStatement->resolveTypes();
//        if (_falseStatement.valid())
//            _falseStatement->resolveTypes();
//    }
//    void compile()
//    {
//        _condition->compile();
//        if (_condition->type() != BooleanType()) {
//            static String error("Boolean expression expected");
//            _location.throwError(error);
//        }
//        if (_trueStatement.valid())
//            _trueStatement->compile();
//        if (_falseStatement.valid())
//            _falseStatement->compile();
//    }
//    ExtricationStatement* run(Stack<Value>* stack)
//    {
//        _condition->push(stack);
//        bool result = (stack->pop().getInt() != 0);
//        if (result) {
//            if (_trueStatement.valid())
//                return _trueStatement->run(stack);
//        }
//        else
//            if (_falseStatement.valid())
//                return _falseStatement->run(stack);
//        return 0;
//    }

SymbolList parseCaseList(CharacterSource* source, Symbol& defaultCase)
{
    CharacterSource s = *source;
    Symbol c = parseCase(source);
    if (c.atom() == atomDefaultCase) {
        if (defaultCase.valid()) {
            static String error("This switch statement already has a default case");
            s.location().throwError(error);
        }
        defaultCase = c;
        return parseCaseList(source, defaultCase);
    }
    return SymbolList(c, parseCaseList(source, defaultCase);
}

Symbol parseSwitchStatement(CharacterSource* source)
{
    DiagnosticSpan startSpan;
    static String switchKeyword("switch");
    if (!Space::parseKeyword(source, switchKeyword, startSpan))
        return Symbol();
    Space::assertCharacter(source, '(');
    Symbol expression = parseExpression(source);
    if (!expression.valid()) {
        static String error("Expected expression");
        source->location().throwError(error);
    }
    Space::assertCharacter(source, ')');
    Space::assertCharacter(source, '{');
    Symbol defaultCase;
    SymbolList cases = parseCaseList(source, defaultCase);
    return new SwitchStatement(expression, defaultCase, &cases);
}
//    void resolveTypes()
//    {
//        for (int i = 0; i < _cases.count(); ++i)
//            _cases[i]->resolveTypes();
//        if (_default.valid())
//            _default->resolveTypes();
//    }
//    void compile()
//    {
//        _expression->compile();
//        _type = _expression->type();
//        for (int i = 0; i < _cases.count(); ++i)
//            _cases[i]->compile(_type);
//        if (_default.valid())
//            _default->resolveTypes();
//    }
//    ExtricationStatement* run(Stack<Value>* stack)
//    {
//        _expression->push(stack);
//        Value v = stack->pop();
//        for (int i = 0; i < _cases.count(); ++i)
//            if (_cases[i]->matches(_type, v, stack))
//                return _cases[i]->run(stack);
//        if (_default.valid())
//            return _default->run(stack);
//        return 0;
//    }
//private:
//    class Case : public ReferenceCounted
//    {
//    public:
//        static Reference<Case> parse(CharacterSource* source, Scope* scope)
//        {
//            static String caseKeyword("case");
//            static String defaultKeyword("default");
//            Stack<Reference<Expression> > expressions;
//            bool defaultType;
//            if (Space::parseKeyword(source, caseKeyword)) {
//                defaultType = false;
//                do {
//                    Reference<Expression> expression = Expression::parse(source, scope);
//                    if (!expression.valid()) {
//                        static String error("Expected expression");
//                        source->location().throwError(error);
//                    }
//                    expressions.push(expression);
//                    if (!Space::parseCharacter(source, ','))
//                        break;
//                } while (true);
//            }
//            else {
//                defaultType = true;
//                if (Space::parseKeyword(source, defaultKeyword))
//                    Space::assertCharacter(source, ':');
//                else {
//                    static String error("Expected case or default");
//                    source->location().throwError(error);
//                }
//            }
//            Reference<Statement> statement = Statement::parse(source, scope);
//            if (!statement.valid()) {
//                static String error("Expected statement");
//                source->location().throwError(error);
//            }
//            return new Case(defaultType, &expressions, statement);
//        }
//        bool isDefault() const { return _default; }
//        void resolveTypes()
//        {
//            _statement->resolveTypes();
//        }
//        void compile(Type type)
//        {
//            for (int i = 0; i < _expressions.count(); ++i) {
//                _expressions[i]->compile();
//                if (_expressions[i]->type() != type) {
//                    static String error("Type mismatch");
//                    _location.throwError(error);
//                }
//            }
//            _statement->compile();
//        }
//        bool matches(Type type, Value value, Stack<Value>* stack)
//        {
//            for (int i = 0; i < _expressions.count(); ++i) {
//                _expressions[i]->push(stack);
//                Value v = stack->pop();
//                if (type == StringType()) {
//                    if (v.getString() == value.getString()) {
//                        return true;
//                    }
//                }
//                else {
//                    if (v.getInt() == value.getInt())
//                        return true;
//                }
//            }
//            return false;
//        }
//        ExtricationStatement* run(Stack<Value>* stack)
//        {
//            return _statement->run(stack);
//        }
//    private:
//        Case(bool defaultType, Stack<Reference<Expression> >* stack, Reference<Statement> statement)
//          : _default(defaultType), _statement(statement)
//        {
//            stack->toArray(&_expressions);
//        }
//        bool _default;
//        Array<Reference<Expression> > _expressions;
//        Reference<Statement> _statement;
//        DiagnosticLocation _location;
//    };
//    SwitchStatement(Reference<Expression> expression, Reference<Case> defaultCase, Stack<Reference<Case> >* cases)
//      : _expression(expression), _default(defaultCase)
//    {
//        cases->toArray(&_cases);
//    }

Symbol parseStatement(CharacterSource* source)
{
    Symbol s = parseExpressionStatement(source);
    if (s.valid())
        return s;
    s = parseFunctionDefinitionStatement(source);
    if (s.valid())
        return s;
    s = parseVariableDefinitionStatement(source);
    if (s.valid())
        return s;
    s = parseAssignmentStatement(source);
    if (s.valid())
        return s;
    s = parseCompoundStatement(source);
    if (s.valid())
        return s;
    s = parseTypeAliasStatement(source);
    if (s.valid())
        return s;
    s = parseNothingStatement(source);
    if (s.valid())
        return s;
    s = parseIncrementStatement(source);
    if (s.valid())
        return s;
    s = parseDecrementStatement(source);
    if (s.valid())
        return s;
    s = parseConditionalStatement(source);
    if (s.valid())
        return s;
    s = parseSwitchStatement(source);
    if (s.valid())
        return s;
    return Symbol();
}

Symbol* run(Symbol statement, Stack<Value>* stack)
{
    Address a;
    switch (statement.atom()) {
        case atomExpressionStatement:
            pushValue(statement[1], stack);
            if (typeOfExpression(statement[1].symbol()).atom() != atomVoid)
                stack->pop();
            return 0;
        case atomFunctionDefinitionStatement:
            return 0;
        case atomVariableDefinitionStatement:
            if (statement[3].valid()) {
                pushValue(statement[3], stack);
                setValue(statement[2], stack->pop());
            }
            return 0;
        case atomAssignmentStatement:
            pushValue(statement[2], stack);
            setValue(statement[1], stack->pop());
            return 0;
        case atomAddAssignmentStatement:
            pushValue(statement[2], stack);
            pushAddress(statement[1], stack);
            a = stack->pop().address();
            setValue(a, a.getValue() + stack->pop());
            return 0;
        case atomSubtractAssignmentStatement:
            pushValue(statement[2], stack);
            pushAddress(statement[1], stack);
            a = stack->pop().address();
            setValue(a, a.getValue() - stack->pop());
            return 0;
        case atomMultiplyAssignmentStatement:
            pushValue(statement[2], stack);
            pushAddress(statement[1], stack);
            a = stack->pop().address();
            setValue(a, a.getValue() * stack->pop());
            return 0;
        case atomDivideAssignmentStatement:
            pushValue(statement[2], stack);
            pushAddress(statement[1], stack);
            a = stack->pop().address();
            setValue(a, a.getValue() / stack->pop());
            return 0;
        case atomModuloAssignmentStatement:
            pushValue(statement[2], stack);
            pushAddress(statement[1], stack);
            a = stack->pop().address();
            setValue(a, a.getValue() % stack->pop());
            return 0;
        case atomShiftLeftAssignmentStatement:
            pushValue(statement[2], stack);
            pushAddress(statement[1], stack);
            a = stack->pop().address();
            setValue(a, a.getValue() << stack->pop());
            return 0;
        case atomShiftRightAssignmentStatement:
            pushValue(statement[2], stack);
            pushAddress(statement[1], stack);
            a = stack->pop().address();
            setValue(a, a.getValue() >> stack->pop());
            return 0;
        case atomAndAssignmentStatement:
            pushValue(statement[2], stack);
            pushAddress(statement[1], stack);
            a = stack->pop().address();
            setValue(a, a.getValue() & stack->pop());
            return 0;
        case atomOrAssignmentStatement:
            pushValue(statement[2], stack);
            pushAddress(statement[1], stack);
            a = stack->pop().address();
            setValue(a, a.getValue() | stack->pop());
            return 0;
        case atomXorAssignmentStatement:
            pushValue(statement[2], stack);
            pushAddress(statement[1], stack);
            a = stack->pop().address();
            setValue(a, a.getValue() ^ stack->pop());
            return 0;
        case atomPowerAssignmentStatement:
            pushValue(statement[2], stack);
            pushAddress(statement[1], stack);
            a = stack->pop().address();
            setValue(a, power(a.getValue(), stack->pop()));
            return 0;
    }
}

//class ExtricationStatement : public Statement
//{
//};
//
//class ReturnStatement : public ExtricationStatement
//{
//public:
//    static Reference<ReturnStatement> parse(CharacterSource* source, Scope* scope)
//    {
//        static String keyword("return");
//        if (!Space::parseKeyword(source, keyword))
//            return 0;
//        Reference<Expression> expression = Expression::parse(source, scope);
//        Space::assertCharacter(source, ';');
//        return new ReturnStatement(expression);
//    }
//    void resolveTypes() { }
//    void compile()
//    {
//        if (_expression.valid())
//            _expression->compile();
//    }
//    ExtricationStatement* run(Stack<Value>* stack)
//    {
//        if (_expression.valid())
//            _expression->push(stack);
//        return this;
//    }
//private:
//    ReturnStatement(Reference<Expression> expression) : _expression(expression)
//    { }
//    Reference<Expression> _expression;
//};
//
//class PrintFunction : public FunctionDeclarationStatement
//{
//public:
//    PrintFunction() : _consoleOutput(Handle::consoleOutput())
//    { }
//    void resolveTypes() { }
//    void compile() { }
//    void call(Stack<Value>* stack)
//    {
//        stack->pop().getString().write(_consoleOutput);
//    }
//    Type returnType() const { return VoidType(); }
//private:
//    Handle _consoleOutput;
//};
//
//class TypeDefinitionStatement : public Statement
//{
//public:
//    void resolveTypes() { }
//    void compile() { }
//    ExtricationStatement* run(Stack<Value>* stack) { return 0; }
//    virtual Type type() = 0;
//};
//
//class IncludeStatement : public Statement
//{
//public:
//    static Reference<IncludeStatement> parse(CharacterSource* source, Scope* scope)
//    {
//        static String include("include");
//        if (!Space::parseKeyword(source, include))
//            return 0;
//        Reference<Expression> expression = Expression::parse(source, scope);
//        if (!expression.valid()) {
//            static String error("Expected expression");
//            source->location().throwError(error);
//        }
//        Space::assertCharacter(source, ';');
//        return new IncludeStatement(scope, expression);
//    }
//    void resolveTypes() { }
//    void compile() { }
//    ExtricationStatement* run(Stack<Value>* stack) { return 0; }
//private:
//    IncludeStatement(Scope* scope, Reference<Expression> expression)
//      : _scope(scope), _expression(expression)
//    { }
//    Scope* _scope;
//    Reference<Expression> _expression;
//};
//
//class BreakStatement;
//
//class ContinueStatement;
//
//template<class T> class BreakOrContinueStatementTemplate;
//
//typedef BreakOrContinueStatementTemplate<void> BreakOrContinueStatement;
//
//template<class T> class BreakOrContinueStatementTemplate : public ExtricationStatement
//{
//public:
//    static Reference<BreakOrContinueStatement> parse(CharacterSource* source, Scope* scope)
//    {
//        Reference<BreakStatement> breakStatement = BreakStatement::parse(source, scope);
//        if (breakStatement.valid())
//            return breakStatement;
//        return ContinueStatement::parse(source, scope);
//    }
//    void resolveTypes() { }
//    void compile() { }
//    ExtricationStatement* run(Stack<Value>* stack) { return this; }
//};
//
//class BreakStatement : public BreakOrContinueStatement
//{
//public:
//    static Reference<BreakStatement> parse(CharacterSource* source, Scope* scope)
//    {
//        static String keyword("break");
//        if (!Space::parseKeyword(source, keyword))
//            return 0;
//        Reference<BreakOrContinueStatement> statement = BreakOrContinueStatement::parse(source, scope);
//        if (!statement.valid())
//            Space::assertCharacter(source, ';');
//        return new BreakStatement(statement);
//    }
//    BreakOrContinueStatement* nextAction() { return _statement; }
//private:
//    BreakStatement(Reference<BreakOrContinueStatement> statement) : _statement(statement) { }
//    Reference<BreakOrContinueStatement> _statement;
//};
//
//class ContinueStatement : public BreakOrContinueStatement
//{
//public:
//    static Reference<ContinueStatement> parse(CharacterSource* source, Scope* scope)
//    {
//        static String keyword("continue");
//        if (!Space::parseKeyword(source, keyword))
//            return 0;
//        Space::assertCharacter(source, ';');
//        return new ContinueStatement();
//    }
//};
//
//class ForeverStatement : public Statement
//{
//public:
//    static Reference<ForeverStatement> parse(CharacterSource* source, Scope* scope)
//    {
//        static String forever("forever");
//        if (!Space::parseKeyword(source, forever))
//            return 0;
//        Reference<Statement> statement = Statement::parse(source, scope);
//        if (!statement.valid()) {
//            static String error("Expected statement");
//            source->location().throwError(error);
//        }
//        return new ForeverStatement(statement);
//    }
//    void resolveTypes() { _statement->resolveTypes(); }
//    void compile() { _statement->compile(); }
//    ExtricationStatement* run(Stack<Value>* stack)
//    {
//        do {
//            ExtricationStatement* statement = _statement->run(stack);
//            BreakStatement* breakStatement = dynamic_cast<BreakStatement*>(statement);
//            if (breakStatement != 0)
//                return breakStatement->nextAction();
//            ContinueStatement* continueStatement = dynamic_cast<ContinueStatement*>(statement);
//            if (continueStatement != 0)
//                continue;
//            return statement;  // must be a throw or return statement
//        } while (true);
//    }
//public:
//    ForeverStatement(Reference<Statement> statement)
//      : _statement(statement)
//    { }
//    Reference<Statement> _statement;
//};
//
//class StringTypeDefinitionStatement : public TypeDefinitionStatement
//{
//public:
//    Type type() { return StringType(); }
//};
//
//class IntTypeDefinitionStatement : public TypeDefinitionStatement
//{
//public:
//    Type type() { return IntType(); }
//};
//
//class VoidTypeDefinitionStatement : public TypeDefinitionStatement
//{
//public:
//    Type type() { return VoidType(); }
//};
//
//class BooleanTypeDefinitionStatement : public TypeDefinitionStatement
//{
//public:
//    Type type() { return BooleanType(); }
//};
//
