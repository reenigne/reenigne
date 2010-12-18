Symbol parseStatement(CharacterSource* source);
Symbol parseStatementOrFail(CharacterSource* source);

Symbol parseExpressionStatement(CharacterSource* source)
{
    CharacterSource s = *source;
    Symbol expression = parseExpression(&s);
    if (!expression.valid())
        return Symbol();
    Span span;
    if (!Space::parseCharacter(&s, ';', span))
        return Symbol();
    *source = s;
    if (expression.atom() != atomFunctionCall) {
        static String error("Statement has no effect");
        source->location().throwError(error);
    }
    return Symbol(atomExpressionStatement, Span(expression.span().start(), span.end()), expression);
}

Symbol parseParameter(CharacterSource* source)
{
    Location location = source->location();
    Symbol typeSpecifier = parseTypeSpecifier(source);
    if (!typeSpecifier.valid())
        return Symbol();
    Symbol name = parseIdentifier(source);
    if (!name.valid()) {
        static String error("Expected identifier");
        source->location().throwError(error);
    }
    return Symbol(atomParameter, Span(location, name.span().end()), typeSpecifier, name);
}

SymbolArray parseParameterList(CharacterSource* source)
{
    SymbolList list;
    Symbol parameter = parseParameter(source);
    if (!parameter.valid())
        return list;
    list.add(parameter);
    Span span;
    while (Space::parseCharacter(source, ',', span)) {
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
    if (!Space::parseCharacter(&s, '(', span))
        return Symbol();
    *source = s;
    SymbolArray parameterList = parseParameterList(source);
    Space::assertCharacter(source, ')');

    static String from("from");
    if (Space::parseKeyword(source, from, span)) {
        Symbol dll = parseExpressionOrFail(source);
        return Symbol(
            atomFunctionDefinitionStatement,
            Span(returnTypeSpecifier.span().start(), dll.span().end()),
            returnTypeSpecifier,
            name,
            parameterList,
            Symbol(atomFromStatement, Span(span.start(), dll.span().end()), dll));
    }
    Symbol statement = parseStatementOrFail(source);
    return Symbol(
        Symbol::newLabel(),
        atomFunctionDefinitionStatement,
        Span(returnTypeSpecifier.span().start(), statement.span().end()),
        returnTypeSpecifier,
        name,
        parameterList,
        statement);
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
    if (Space::parseCharacter(source, '=', span))
        initializer = parseExpressionOrFail(source);
    Location end = Space::assertCharacter(source, ';');
    return Symbol(
        Symbol::newLabel(),
        atomVariableDefinitionStatement,
        Span(typeSpecifier.span().start(), end),
        typeSpecifier,
        identifier,
        initializer);
}

Symbol parseAssignmentStatement(CharacterSource* source)
{
    CharacterSource s = *source;
    Symbol lValue = parseExpression(&s);
    Location operatorLocation = s.location();
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

    Span span;
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
    Symbol e = parseExpressionOrFail(source);
    Location end = Space::assertCharacter(source, ';');

    Span span(lValue.span().start(), end);
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
    if (!Space::parseCharacter(source, '{', span))
        return Symbol();
    SymbolArray sequence = parseStatementSequence(source);
    Location end = Space::assertCharacter(source, '}');
    return Symbol(atomCompoundStatement, Span(span.start(), end), sequence);
}

Symbol parseTypeAliasStatement(CharacterSource* source)
{
    CharacterSource s = *source;
    CharacterSource s2 = s;
    Symbol typeIdentifier = parseTypeIdentifier(&s);
    if (!typeIdentifier.valid())
        return Symbol();
    Span span;
    if (!Space::parseCharacter(&s, '=', span))
        return Symbol();
    *source = s;
    Symbol typeSpecifier = parseTypeSpecifier(source);
    Location end = Space::assertCharacter(source, ';');
    return Symbol(Symbol::newLabel(), atomTypeAliasStatement, Span(typeIdentifier.span().start(), end), typeIdentifier, typeSpecifier);
}

Symbol parseNothingStatement(CharacterSource* source)
{
    static String nothing("nothing");
    Span span;
    if (!Space::parseKeyword(source, nothing, span))
        return Symbol();
    Location end = Space::assertCharacter(source, ';');
    return Symbol(atomNothingStatement, Span(span.start(), end));
}

Symbol parseIncrementStatement(CharacterSource* source)
{
    static String incrementOperator("++");
    Span span;
    if (!Space::parseOperator(source, incrementOperator, span))
        return Symbol();
    CharacterSource s = *source;
    Symbol lValue = parseExpression(&s);
    *source = s;
    Location end = Space::assertCharacter(source, ';');
    return Symbol(atomIncrementStatement, Span(span.start(), end), lValue);
}

Symbol parseDecrementStatement(CharacterSource* source)
{
    static String decrementOperator("--");
    Span span;
    if (!Space::parseOperator(source, decrementOperator, span))
        return Symbol();
    CharacterSource s = *source;
    Symbol lValue = parseExpression(&s);
    *source = s;
    Location end = Space::assertCharacter(source, ';');
    return Symbol(atomIncrementStatement, Span(span.start(), end), lValue);
}

Symbol parseConditionalStatement2(CharacterSource* source, Location start, bool unlessStatement)
{
    static String elseKeyword("else");
    static String elseIfKeyword("elseIf");
    static String elseUnlessKeyword("elseUnless");
    Space::assertCharacter(source, '(');
    Span span;
    Symbol condition = parseExpressionOrFail(source);
    Space::assertCharacter(source, ')');
    Symbol conditionedStatement = parseStatementOrFail(source);
    Symbol elseStatement;
    if (Space::parseKeyword(source, elseKeyword, span))
        elseStatement = parseStatementOrFail(source);
    else
        if (Space::parseKeyword(source, elseIfKeyword, span))
            elseStatement = parseConditionalStatement2(source, span.start(), false);
        else
            if (Space::parseKeyword(source, elseUnlessKeyword, span))
                elseStatement = parseConditionalStatement2(source, span.start(), true);
    if (unlessStatement)
        return Symbol(atomIfStatement, Span(start, elseStatement.span().end()), condition, elseStatement, conditionedStatement);
    return Symbol(atomIfStatement, Span(start, elseStatement.span().end()), condition, conditionedStatement, elseStatement);
}

Symbol parseConditionalStatement(CharacterSource* source)
{
    static String ifKeyword("if");
    static String unlessKeyword("unless");
    Span span;
    if (Space::parseKeyword(source, ifKeyword, span))
        return parseConditionalStatement2(source, span.start(), false);
    if (Space::parseKeyword(source, unlessKeyword, span))
        return parseConditionalStatement2(source, span.start(), true);
    return Symbol();
}

Symbol parseCase(CharacterSource* source)
{
    static String caseKeyword("case");
    static String defaultKeyword("default");
    SymbolList expressions;
    bool defaultType;
    Span span;
    if (Space::parseKeyword(source, caseKeyword, span)) {
        defaultType = false;
        do {
            Symbol expression = parseExpressionOrFail(source);
            expressions.add(expression);
            Span span;
            if (!Space::parseCharacter(source, ',', span))
                break;
        } while (true);
    }
    else {
        defaultType = true;
        if (!Space::parseKeyword(source, defaultKeyword, span)) {
            static String error("Expected case or default");
            source->location().throwError(error);
        }
    }
    Space::assertCharacter(source, ':');
    Symbol statement = parseStatementOrFail(source);
    span = Span(span.start(), statement.span().end());
    if (defaultType)
        return Symbol(atomDefaultCase, span, statement);
    return Symbol(atomCase, span, SymbolArray(expressions), statement);
}

Symbol parseSwitchStatement(CharacterSource* source)
{
    Span startSpan;
    static String switchKeyword("switch");
    if (!Space::parseKeyword(source, switchKeyword, startSpan))
        return Symbol();
    Space::assertCharacter(source, '(');
    Symbol expression = parseExpressionOrFail(source);
    Space::assertCharacter(source, ')');
    Space::assertCharacter(source, '{');
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
    Location end = Space::assertCharacter(source, '}');
    return Symbol(atomSwitchStatement, Span(startSpan.start(), end), expression, defaultCase, SymbolArray(cases));
}

Symbol parseReturnStatement(CharacterSource* source)
{
    static String keyword("return");
    Span span;
    if (!Space::parseKeyword(source, keyword, span))
        return Symbol();
    Symbol expression = parseExpression(source);
    Location end = Space::assertCharacter(source, ';');
    return Symbol(atomReturnStatement, Span(span.start(), end), expression);
}

Symbol parseIncludeStatement(CharacterSource* source)
{
    static String include("include");
    Span span;
    if (!Space::parseKeyword(source, include, span))
        return Symbol();
    Symbol expression = parseExpressionOrFail(source);
    Location end = Space::assertCharacter(source, ';');
    return Symbol(atomIncludeStatement, Span(span.start(), end), expression);
}

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
    if (!Space::parseKeyword(source, keyword, span))
        return Symbol();
    Symbol statement = parseBreakOrContinueStatement(source);
    Location end;
    if (!statement.valid())
        end = Space::assertCharacter(source, ';');
    else
        end = statement.span().end();
    return Symbol(atomBreakStatement, Span(span.start(), end), statement);
}

Symbol parseContinueStatement(CharacterSource* source)
{
    static String keyword("continue");
    Span span;
    if (!Space::parseKeyword(source, keyword, span))
        return Symbol();
    Location end = Space::assertCharacter(source, ';');
    return Symbol(atomContinueStatement, Span(span.start(), end));
}

Symbol parseForeverStatement(CharacterSource* source)
{
    static String forever("forever");
    Span span;
    if (!Space::parseKeyword(source, forever, span))
        return Symbol();
    Symbol statement = parseStatementOrFail(source);
    return Symbol(atomForeverStatement, Span(span.start(), statement.span().end()), statement);
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
    if (Space::parseKeyword(source, doKeyword, span)) {
        foundDo = true;
        doStatement = parseStatementOrFail(source);
        start = span.start();
    }
    bool foundWhile = false;
    bool foundUntil = false;
    if (Space::parseKeyword(source, whileKeyword, span)) {
        foundWhile = true;
        if (!doStatement.valid())
            start = span.start();
    }
    else
        if (Space::parseKeyword(source, untilKeyword, span)) {
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
    Space::assertCharacter(source, '(');
    Symbol condition = parseExpression(source);
    Space::assertCharacter(source, ')');
    Symbol statement = parseStatementOrFail(source);
    Symbol doneStatement;
    Location end = statement.span().end();
    if (Space::parseKeyword(source, doneKeyword, span)) {
        doneStatement = parseStatementOrFail(source);
        end = doneStatement.span().end();
    }
    if (foundWhile)
        return Symbol(atomWhileStatement, Span(start, end), doStatement, condition, statement, doneStatement);
    return Symbol(atomUntilStatement, Span(start, end), doStatement, condition, statement, doneStatement);
}

Symbol parseForStatement(CharacterSource* source)
{
    static String forKeyword("for");
    static String doneKeyword("done");
    Span span;
    if (!Space::parseKeyword(source, forKeyword, span))
        return Symbol();
    Space::assertCharacter(source, '(');
    Symbol preStatement = parseStatement(source);
    if (!preStatement.valid())
        Space::assertCharacter(source, ';');
    Symbol expression = parseExpression(source);
    Space::assertCharacter(source, ';');
    Symbol postStatement = parseStatement(source);
    Space::parseCharacter(source, ')');
    Symbol statement = parseStatementOrFail(source);
    Symbol doneStatement;
    Location end = statement.span().end();
    if (Space::parseKeyword(source, doneKeyword, span)) {
        doneStatement = parseStatementOrFail(source);
        end = doneStatement.span().end();
    }
    return Symbol(atomForStatement, Span(span.start(), end), preStatement, expression, postStatement, statement, doneStatement);
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