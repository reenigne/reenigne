Symbol parseStatement(CharacterSource* source);
Symbol parseStatementOrFail(CharacterSource* source);

Symbol parseExpressionStatement(CharacterSource* source)
{
    CharacterSource s = *source;
    Symbol expression = parseExpression(&s);
    if (!expression.valid())
        return Symbol();
    Span span;
    if (!Space::parseCharacter(&s, ';', &span))
        return Symbol();
    *source = s;
    if (expression.atom() != atomFunctionCall) {
        static String error("Statement has no effect");
        source->location().throwError(error);
    }
    return Symbol(atomExpressionStatement, expression,
        newSpan(spanOf(expression) + span));
}

Symbol parseParameter(CharacterSource* source)
{
    Symbol typeSpecifier = parseTypeSpecifier(source);
    if (!typeSpecifier.valid())
        return Symbol();
    Symbol name = parseIdentifier(source);
    if (!name.valid()) {
        static String error("Expected identifier");
        source->location().throwError(error);
    }
    return Symbol(atomParameter, typeSpecifier, name,
        new IdentifierCache(spanOf(typeSpecifier) + spanOf(name), Symbol::newLabel()));
}

SymbolArray parseParameterList(CharacterSource* source)
{
    SymbolList list;
    Symbol parameter = parseParameter(source);
    if (!parameter.valid())
        return list;
    list.add(parameter);
    Span span;
    while (Space::parseCharacter(source, ',', &span)) {
        Symbol parameter = parseParameter(source);
        if (!parameter.valid()) {
            static String error("Expected parameter");
            source->location().throwError(error);
        }
        list.add(parameter);
    }
    return list;
}

Symbol parseFunctionDefinitionStatement(CharacterSource* source)
{
    CharacterSource s = *source;
    Symbol returnTypeSpecifier = parseTypeSpecifier(&s);
    if (!returnTypeSpecifier.valid())
        return Symbol();
    Symbol name = parseIdentifier(&s);
    if (!name.valid())
        return Symbol();
    Span span;
    if (!Space::parseCharacter(&s, '(', &span))
        return Symbol();
    *source = s;
    SymbolArray parameterList = parseParameterList(source);
    Space::assertCharacter(source, ')', &span);

    static String from("from");
    if (Space::parseKeyword(source, from, &span)) {
        Symbol dll = parseExpressionOrFail(source);
        Span span2;
        Space::assertCharacter(source, ';', &span2);
        return Symbol(
            atomFunctionDefinitionStatement,
            returnTypeSpecifier,
            name,
            parameterList,
            Symbol(atomFromStatement, dll,
                new ExpressionCache(span + span2)),
            new FunctionDefinitionCache(spanOf(returnTypeSpecifier) + span2));
    }
    Symbol statement = parseStatementOrFail(source);
    statement = Symbol(
        atomFunctionDefinitionStatement,
        returnTypeSpecifier,
        name,
        parameterList,
        statement,
        new FunctionDefinitionCache(
            spanOf(returnTypeSpecifier) + spanOf(statement)));
    return statement;
}

Symbol parseVariableDefinitionStatement(CharacterSource* source)
{
    CharacterSource s = *source;
    Symbol typeSpecifier = parseTypeSpecifier(&s);
    if (!typeSpecifier.valid())
        return Symbol();
    Symbol identifier = parseIdentifier(&s);
    if (!identifier.valid())
        return Symbol();
    *source = s;
    Symbol initializer;
    Span span;
    if (Space::parseCharacter(source, '=', &span))
        initializer = parseExpressionOrFail(source);
    Space::assertCharacter(source, ';', &span);
    Symbol statement = Symbol(atomVariableDefinitionStatement,
        typeSpecifier,
        identifier,
        initializer,
        new IdentifierCache(spanOf(typeSpecifier) + span, Symbol::newLabel()));
    return statement;
}

Symbol parseAssignmentStatement(CharacterSource* source)
{
    CharacterSource s = *source;
    Symbol lValue = parseExpression(&s);
    Location operatorLocation = s.location();
    if (!lValue.valid())
        return Symbol();
    Symbol function;
    Span span;
    if (Space::parseCharacter(&s, '=', &span))
        function = Symbol(atomAssignment);
    else if (Space::parseOperator(&s, addAssignment, &span))
        function = Symbol(atomAddAssignment);
    else if (Space::parseOperator(&s, subtractAssignment, &span))
        function = Symbol(atomSubtractAssignment);
    else if (Space::parseOperator(&s, multiplyAssignment, &span))
        function = Symbol(atomMultiplyAssignment);
    else if (Space::parseOperator(&s, divideAssignment, &span))
        function = Symbol(atomDivideAssignment);
    else if (Space::parseOperator(&s, moduloAssignment, &span))
        function = Symbol(atomModuloAssignment);
    else if (Space::parseOperator(&s, shiftLeftAssignment, &span))
        function = Symbol(atomShiftLeftAssignment);
    else if (Space::parseOperator(&s, shiftRightAssignment, &span))
        function = Symbol(atomShiftRightAssignment);
    else if (Space::parseOperator(&s, bitwiseAndAssignment, &span))
        function = Symbol(atomBitwiseAndAssignment);
    else if (Space::parseOperator(&s, bitwiseOrAssignment, &span))
        function = Symbol(atomBitwiseOrAssignment);
    else if (Space::parseOperator(&s, bitwiseXorAssignment, &span))
        function = Symbol(atomBitwiseXorAssignment);
    else if (Space::parseOperator(&s, powerAssignment, &span))
        function = Symbol(atomPowerAssignment);
    if (!function.valid())
        return Symbol();

    *source = s;
    Symbol e = parseExpressionOrFail(source);
    Space::assertCharacter(source, ';', &span);

    return Symbol(atomFunctionCall, function, SymbolArray(Symbol(atomAddressOf, lValue), e),
        new ExpressionCache(spanOf(lValue) + span));
}

SymbolArray parseStatementSequence(CharacterSource* source)
{
    SymbolList list;
    do {
        Symbol statement = parseStatement(source);
        if (!statement.valid())
            return list;
        list.add(statement);
    } while (true);
}

Symbol parseCompoundStatement(CharacterSource* source)
{
    Span span;
    if (!Space::parseCharacter(source, '{', &span))
        return Symbol();
    SymbolArray sequence = parseStatementSequence(source);
    Span span2;
    Space::assertCharacter(source, '}', &span2);
    return Symbol(atomCompoundStatement, sequence, newSpan(span + span2));
}

Symbol parseTypeConstructorDefinitionStatement(CharacterSource* source)
{
    CharacterSource s = *source;
    CharacterSource s2 = s;
    Symbol typeConstructorSignifier = parseTypeConstructorSignifier(&s);
    if (!typeConstructorSignifier.valid())
        return Symbol();
    Span span;
    if (!Space::parseCharacter(&s, '=', &span))
        return Symbol();
    *source = s;
    Symbol typeConstructorSpecifier = parseTypeConstructorSpecifier(source);
    Space::assertCharacter(source, ';', &span);
    Symbol statement = Symbol(atomTypeConstructorDefinitionStatement,
        typeConstructorSignifier, typeConstructorSpecifier,
        new IdentifierCache(spanOf(typeConstructorSignifier) + span,
            Symbol::newLabel()));
    return statement;
}

Symbol parseNothingStatement(CharacterSource* source)
{
    static String nothing("nothing");
    Span span;
    if (!Space::parseKeyword(source, nothing, &span))
        return Symbol();
    Span span2;
    Space::assertCharacter(source, ';', &span2);
    return Symbol(atomNothingStatement, newSpan(span + span2));
}

Symbol parseIncrementDecrementStatement(CharacterSource* source)
{
    Span span;
    Symbol function;
    if (Space::parseOperator(source, increment, &span))
        function = Symbol(atomIncrement);
    else if (Space::parseOperator(source, decrement, &span))
        function = Symbol(atomDecrement);
    else
        return Symbol();
    CharacterSource s = *source;
    Symbol lValue = parseExpression(&s);
    *source = s;
    Span span2;
    Space::assertCharacter(source, ';', &span2);
    return Symbol(atomFunctionCall, function, SymbolArray(lValue),
        newSpan(span + span2));
}

Symbol parseConditionalStatement2(CharacterSource* source, Span startSpan,
    bool unlessStatement)
{
    static String elseKeyword("else");
    static String elseIfKeyword("elseIf");
    static String elseUnlessKeyword("elseUnless");
    Span span;
    Space::assertCharacter(source, '(', &span);
    Symbol condition = parseExpressionOrFail(source);
    Space::assertCharacter(source, ')', &span);
    Symbol conditionedStatement = parseStatementOrFail(source);
    Symbol elseStatement;
    if (Space::parseKeyword(source, elseKeyword, &span))
        elseStatement = parseStatementOrFail(source);
    else
        if (Space::parseKeyword(source, elseIfKeyword, &span))
            elseStatement =
                parseConditionalStatement2(source, span, false);
        else
            if (Space::parseKeyword(source, elseUnlessKeyword, &span))
                elseStatement =
                    parseConditionalStatement2(source, span, true);
    SpanCache* cache = newSpan(startSpan + spanOf(elseStatement));
    if (unlessStatement)
        return Symbol(atomIfStatement, condition, elseStatement,
            conditionedStatement, cache);
    return Symbol(atomIfStatement, condition, conditionedStatement,
        elseStatement, cache);
}

Symbol parseConditionalStatement(CharacterSource* source)
{
    static String ifKeyword("if");
    static String unlessKeyword("unless");
    Span span;
    if (Space::parseKeyword(source, ifKeyword, &span))
        return parseConditionalStatement2(source, span, false);
    if (Space::parseKeyword(source, unlessKeyword, &span))
        return parseConditionalStatement2(source, span, true);
    return Symbol();
}

Symbol parseCase(CharacterSource* source)
{
    static String caseKeyword("case");
    static String defaultKeyword("default");
    SymbolList expressions;
    bool defaultType;
    Span span;
    Span span2;
    if (Space::parseKeyword(source, caseKeyword, &span)) {
        defaultType = false;
        do {
            Symbol expression = parseExpressionOrFail(source);
            expressions.add(expression);
            if (!Space::parseCharacter(source, ',', &span2))
                break;
        } while (true);
    }
    else {
        defaultType = true;
        if (!Space::parseKeyword(source, defaultKeyword, &span)) {
            static String error("Expected case or default");
            source->location().throwError(error);
        }
    }

    Space::assertCharacter(source, ':', &span2);
    Symbol statement = parseStatementOrFail(source);
    SpanCache* cache = newSpan(span + spanOf(statement));
    if (defaultType)
        return Symbol(atomDefaultCase, statement, cache);
    return Symbol(atomCase, SymbolArray(expressions), statement, cache);
}

Symbol parseSwitchStatement(CharacterSource* source)
{
    Span startSpan;
    static String switchKeyword("switch");
    if (!Space::parseKeyword(source, switchKeyword, &startSpan))
        return Symbol();
    Span span;
    Space::assertCharacter(source, '(', &span);
    Symbol expression = parseExpressionOrFail(source);
    Space::assertCharacter(source, ')', &span);
    Space::assertCharacter(source, '{', &span);
    Symbol defaultCase;

    CharacterSource s = *source;
    SymbolList cases;
    do {
        Symbol c = parseCase(source);
        if (!c.valid())
            break;
        if (c.atom() == atomDefaultCase) {
            if (defaultCase.valid()) {
                static String error("This switch statement already has a default case");
                s.location().throwError(error);
            }
            defaultCase = c;
        }
        else
            cases.add(c);
    } while (true);
    Space::assertCharacter(source, '}', &span);
    return Symbol(atomSwitchStatement, expression, defaultCase,
        SymbolArray(cases), newSpan(startSpan + span));
}

Symbol parseReturnStatement(CharacterSource* source)
{
    static String keyword("return");
    Span span;
    if (!Space::parseKeyword(source, keyword, &span))
        return Symbol();
    Symbol expression = parseExpression(source);
    Span span2;
    Space::assertCharacter(source, ';', &span2);
    return Symbol(atomReturnStatement, expression, newSpan(span + span2));
}

Symbol parseIncludeStatement(CharacterSource* source)
{
    static String include("include");
    Span span;
    if (!Space::parseKeyword(source, include, &span))
        return Symbol();
    Symbol expression = parseExpressionOrFail(source);
    Span span2;
    Space::assertCharacter(source, ';', &span2);
    return Symbol(atomIncludeStatement, expression, newSpan(span + span2));
}

Symbol parseBreakStatement(CharacterSource* source);
Symbol parseContinueStatement(CharacterSource* source);

Symbol parseBreakOrContinueStatement(CharacterSource* source)
{
    Symbol breakStatement = parseBreakStatement(source);
    if (breakStatement.valid())
        return breakStatement;
    return parseContinueStatement(source);
}

Symbol parseBreakStatement(CharacterSource* source)
{
    static String keyword("break");
    Span span;
    if (!Space::parseKeyword(source, keyword, &span))
        return Symbol();
    Symbol statement = parseBreakOrContinueStatement(source);
    Span span2;
    if (!statement.valid())
        Space::assertCharacter(source, ';', &span2);
    else
        span2 = spanOf(statement);
    return Symbol(atomBreakStatement, statement, newSpan(span + span2));
}

Symbol parseContinueStatement(CharacterSource* source)
{
    static String keyword("continue");
    Span span;
    if (!Space::parseKeyword(source, keyword, &span))
        return Symbol();
    Span span2;
    Space::assertCharacter(source, ';', &span2);
    return Symbol(atomContinueStatement, newSpan(span + span2));
}

Symbol parseForeverStatement(CharacterSource* source)
{
    static String forever("forever");
    Span span;
    if (!Space::parseKeyword(source, forever, &span))
        return Symbol();
    Symbol statement = parseStatementOrFail(source);
    return Symbol(atomForeverStatement, statement, newSpan(span + spanOf(statement)));
}

Symbol parseWhileStatement(CharacterSource* source)
{
    static String doKeyword("do");
    static String whileKeyword("while");
    static String untilKeyword("until");
    static String doneKeyword("done");
    Span span;
    Symbol doStatement;
    Location start;
    bool foundDo = false;
    if (Space::parseKeyword(source, doKeyword, &span)) {
        foundDo = true;
        doStatement = parseStatementOrFail(source);
        start = span.start();
    }
    bool foundWhile = false;
    bool foundUntil = false;
    if (Space::parseKeyword(source, whileKeyword, &span)) {
        foundWhile = true;
        if (!doStatement.valid())
            start = span.start();
    }
    else
        if (Space::parseKeyword(source, untilKeyword, &span)) {
            foundUntil = true;
            if (!doStatement.valid())
                start = span.start();
        }
    if (!foundWhile && !foundUntil) {
        if (foundDo) {
            static String error("Expected while or until");
            source->location().throwError(error);
        }
        return Symbol();
    }
    Span span2;
    Space::assertCharacter(source, '(', &span2);
    Symbol condition = parseExpression(source);
    Space::assertCharacter(source, ')', &span2);
    Symbol statement = parseStatementOrFail(source);
    Symbol doneStatement;
    span2 = spanOf(statement);
    if (Space::parseKeyword(source, doneKeyword, &span2)) {
        doneStatement = parseStatementOrFail(source);
        span2 = spanOf(doneStatement);
    }
    SpanCache* cache = newSpan(span + span2);
    if (foundWhile)
        return Symbol(atomWhileStatement, doStatement, condition, statement,
            doneStatement, cache);
    return Symbol(atomUntilStatement, doStatement, condition, statement,
        doneStatement, cache);
}

Symbol parseForStatement(CharacterSource* source)
{
    static String forKeyword("for");
    static String doneKeyword("done");
    Span span;
    if (!Space::parseKeyword(source, forKeyword, &span))
        return Symbol();
    Span span2;
    Space::assertCharacter(source, '(', &span2);
    Symbol preStatement = parseStatement(source);
    if (!preStatement.valid())
        Space::assertCharacter(source, ';', &span2);
    Symbol expression = parseExpression(source);
    Space::assertCharacter(source, ';', &span2);
    Symbol postStatement = parseStatement(source);
    Space::parseCharacter(source, ')', &span2);
    Symbol statement = parseStatementOrFail(source);
    Symbol doneStatement;
    span2 = spanOf(statement);
    if (Space::parseKeyword(source, doneKeyword, &span2)) {
        doneStatement = parseStatementOrFail(source);
        span2 = spanOf(doneStatement);
    }
    return Symbol(atomForStatement, preStatement, expression, postStatement,
        statement, doneStatement, newSpan(span + span2));
}

Symbol parseEmitStatement(CharacterSource* source)
{
    static String emitKeyword("_emit");
    Span span;
    if (!Space::parseKeyword(source, emitKeyword, &span))
        return Symbol();
    Symbol expression = parseExpressionOrFail(source);
    Span span2;
    Space::assertCharacter(source, ';', &span2);
    return Symbol(atomEmit, expression, newSpan(span + span2));
}

Symbol parseLabelStatement(CharacterSource* source)
{
    CharacterSource s2 = *source;
    Symbol identifier = parseIdentifier(&s2);
    if (!identifier.valid())
        return Symbol();
    Span span;
    if (!Space::parseCharacter(&s2, ':', &span))
        return Symbol();
    return Symbol(atomLabelStatement, identifier,
        new FunctionDefinitionCache(spanOf(identifier) + span));
}

Symbol parseGotoStatement(CharacterSource* source)
{
    static String gotoKeyword("goto");
    Span span;
    if (!Space::parseKeyword(source, gotoKeyword, &span))
        return Symbol();
    Symbol expression = parseExpressionOrFail(source);
    Span span2;
    Space::parseCharacter(source, ';', &span2);
    return Symbol(atomLabelStatement, expression, newSpan(span + span2));
}

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
    s = parseIncrementDecrementStatement(source);
    if (s.valid())
        return s;
    s = parseConditionalStatement(source);
    if (s.valid())
        return s;
    s = parseSwitchStatement(source);
    if (s.valid())
        return s;
    s = parseReturnStatement(source);
    if (s.valid())
        return s;
    s = parseIncludeStatement(source);
    if (s.valid())
        return s;
    s = parseBreakOrContinueStatement(source);
    if (s.valid())
        return s;
    s = parseForeverStatement(source);
    if (s.valid())
        return s;
    s = parseWhileStatement(source);
    if (s.valid())
        return s;
    s = parseForStatement(source);
    if (s.valid())
        return s;
    s = parseEmitStatement(source);
    if (s.valid())
        return s;
    s = parseLabelStatement(source);
    if (s.valid())
        return s;
    s = parseGotoStatement(source);
    if (s.valid())
        return s;
    return Symbol();
}

Symbol parseStatementOrFail(CharacterSource* source)
{
    Symbol statement = parseStatement(source);
    if (!statement.valid()) {
        static String error("Expected statement");
        source->location().throwError(error);
    }
    return statement;
}