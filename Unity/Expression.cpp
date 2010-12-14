void throwError(CharacterSource* source)
{
    static String expected("Expected expression");
    source->location().throwError(expected);
}

Symbol parseExpression(CharacterSource* source);

Symbol combine(Symbol left, Symbol right, DiagnosticSpan span)
{
    if (left.valid())
        return Symbol(atomFunctionCall, span, Symbol(atomAdd, DiagnosticSpan()), SymbolList(left, right));
    return right;
}

int parseHexadecimalCharacter(CharacterSource* source)
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

Symbol parseIdentifier(CharacterSource* source)
{
    CharacterSource s = *source;
    DiagnosticLocation startLocation = s.location();
    int start = s.offset();
    int c = s.get();
    if (c < 'a' || c > 'z')
        return Symbol();
    CharacterSource s2;
    do {
        s2 = s;
        c = s.get();
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
            continue;
        break;
    } while (true);
    int end = s2.offset();
    DiagnosticLocation endLocation = s2.location();
    Space::parse(&s2);
    String name = s2.subString(start, end);
    static String keywords[] = {
        String("assembly"),
        String("break"),
        String("case"),
        String("catch"),
        String("continue"),
        String("default"),
        String("delete"),
        String("do"),
        String("done"),
        String("else"),
        String("elseIf"),
        String("elseUnless"),
        String("finally"),
        String("from"),
        String("for"),
        String("forever"),
        String("if"),
        String("in"),
        String("new"),
        String("nothing"),
        String("return"),
        String("switch"),
        String("this"),
        String("throw"),
        String("try"),
        String("unless"),
        String("until"),
        String("while")
    };
    for (int i = 0; i < sizeof(keywords)/sizeof(keywords[0]); ++i)
        if (name == keywords[i])
            return Symbol();
    *source = s2;
    return Symbol(atomIdentifier, DiagnosticSpan(startLocation, endLocation), name);
}

Symbol parseDoubleQuotedString(CharacterSource* source)
{
    static String endOfFile("End of file in string");
    static String endOfLine("End of line in string");
    static String printableCharacter("printable character");
    static String escapedCharacter("escaped character");
    static String hexadecimalDigit("hexadecimal digit");
    static String toString("toString");
    DiagnosticLocation startLocation = source->location();
    DiagnosticLocation stringStartLocation = startLocation;
    DiagnosticLocation stringEndLocation;
    if (!source->parse('"'))
        return Symbol();
    int start = source->offset();
    int end;
    String insert(empty);
    int n;
    int nn;
    String string(empty);
    Symbol expression;
    Symbol part;
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
                return combine(expression,
                    Symbol(atomStringConstant, DiagnosticSpan(stringStartLocation, s.location()), string),
                    DiagnosticSpan(startLocation, s.location()));
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
                            nn = parseHexadecimalCharacter(source);
                            if (nn == -1) {
                                s = *source;
                                source->throwUnexpected(hexadecimalDigit, String::codePoint(s.get()));
                            }
                            n = (n << 4) | nn;
                        }
                        nn = parseHexadecimalCharacter(source);
                        if (nn != -1) {
                            n = (n << 4) | nn;
                            nn = parseHexadecimalCharacter(source);
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
                stringEndLocation = source->location();
                part = parseIdentifier(source);
                if (!part.valid()) {
                    if (Space::parseCharacter(source, '(')) {
                        part = parseExpression(source);
                        source->assert(')');
                    }
                }
                part = Symbol(atomFunctionCall, part.span(),
                    Symbol(atomDot, part.span(),
                        SymbolList(part, Symbol(atomIdentifier, DiagnosticSpan(), toString))),
                    SymbolList());
                string += s.subString(start, end);
                start = source->offset();
                if (part.valid()) {
                    expression = combine(expression,
                        Symbol(atomStringConstant, DiagnosticSpan(stringStartLocation, stringEndLocation), string),
                        DiagnosticSpan(startLocation, stringEndLocation));
                    string = empty;
                    stringStartLocation = source->location();
                    expression = combine(expression, part, DiagnosticSpan(startLocation, stringStartLocation));
                }
                break;
        }
    } while (true);
}

Symbol parseEmbeddedLiteral(CharacterSource* source)
{
    static String empty;
    static String endOfFile("End of file in string");
    if (!source->parse('#'))
        return Symbol();
    if (!source->parse('#'))
        return Symbol();
    if (!source->parse('#'))
        return Symbol();
    int start = source->offset();
    DiagnosticLocation location = source->location();
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
            return Symbol(atomStringConstant, DiagnosticSpan(location, s2.location()), string);
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
                        return Symbol(atomStringConstant, DiagnosticSpan(location, s2.location()), string);
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

Symbol parseInteger(CharacterSource* source)
{
    CharacterSource s = *source;
    DiagnosticLocation location = s.location();
    int n = 0;
    int c = s.get();
    if (c < '0' || c > '9')
        return Symbol();
    do {
        n = n*10 + c - '0';
        *source = s;
        c = s.get();
        if (c < '0' || c > '9') {
            Space::parse(source);
            return Symbol(atomIntegerConstant, DiagnosticSpan(location, s.location()), n);
        }
    } while (true);
}

Symbol parseExpressionElement(CharacterSource* source)
{
    DiagnosticLocation location = source->location();
    Symbol e = parseDoubleQuotedString(source);
    if (e.valid())
        return e;
    e = parseEmbeddedLiteral(source);
    if (e.valid())
        return e;
    e = parseInteger(source);
    if (e.valid())
        return e;
    e = parseIdentifier(source);
    if (e.valid()) {
        static String trueKeyword("true");
        if (e[1].string() == trueKeyword)
            return Symbol(atomTrue, e.span());
        static String falseKeyword("false");
        if (e[1].string() == falseKeyword)
            return Symbol(atomFalse, e.span());
        static String nullKeyword("null");
        if (e[1].string() == nullKeyword)
            return Symbol(atomNull, e.span());
        return e;
    }
    if (Space::parseCharacter(source, '(')) {
        e = parseExpression(source);
        DiagnosticLocation end = Space::assertCharacter(source, ')');
        return Symbol(e.atom(), DiagnosticSpan(location, end), e.tail());
    }
    return Symbol();
}

SymbolList parseExpressionList2(Symbol expression, CharacterSource* source)
{
    if (!Space::parseCharacter(source, ','))
        return SymbolList(expression);
    Symbol expression2 = parseExpression(source);
    if (!expression2.valid())
        throwError(source);
    return SymbolList(expression, parseExpressionList2(expression2, source));
}

SymbolList parseExpressionList(CharacterSource* source)
{
    Symbol expression = parseExpression(source);
    if (!expression.valid())
        return SymbolList();
    return parseExpressionList2(expression, source);
}

Symbol parseFunctionCallExpression(CharacterSource* source)
{
    DiagnosticLocation startLocation = source->location();
    Symbol e = parseExpressionElement(source);
    if (!e.valid())
        return Symbol();
    do {
        if (!Space::parseCharacter(source, '('))
            break;
        SymbolList arguments = parseExpressionList(source);
        DiagnosticLocation location = Space::assertCharacter(source, ')');
        e = Symbol(atomFunctionCall, DiagnosticSpan(startLocation, location), e, arguments);
    } while (true);
    return e;
}

Symbol binaryOperation(Atom atom, Symbol left, Symbol right)
{
    return Symbol(atomFunctionCall, DiagnosticSpan(left.span().start(), right.span().end()), Symbol(atom), SymbolList(left, right));
}

Symbol parsePowerExpression(CharacterSource* source)
{
    Symbol e = parseFunctionCallExpression(source);
    if (!e.valid())
        return Symbol();
    if (Space::parseCharacter(source, '^')) {
        Symbol e2 = parsePowerExpression(source);
        if (!e2.valid())
            throwError(source);
        e = binaryOperation(atomPower, e, e2);
    }
    return e;
}

Symbol unaryOperation(Atom atom, DiagnosticLocation location, Symbol e)
{
    return Symbol(atomFunctionCall, DiagnosticSpan(location, e.span().end()), Symbol(atom), SymbolList(e));
}

Symbol parseUnaryExpression(CharacterSource* source)
{
    DiagnosticLocation startLocation = source->location();
    if (Space::parseCharacter(source, '~') || Space::parseCharacter(source, '!')) {
        Symbol e = parseUnaryExpression(source);
        return unaryOperation(atomNot, startLocation, e);
    }
    if (Space::parseCharacter(source, '+')) {
        Symbol e = parseUnaryExpression(source);
        return unaryOperation(atomPositive, startLocation, e);
    }
    if (Space::parseCharacter(source, '-')) {
        Symbol e = parseUnaryExpression(source);
        return unaryOperation(atomNegative, startLocation, e);
    }
    if (Space::parseCharacter(source, '*')) {
        Symbol e = parseUnaryExpression(source);
        return unaryOperation(atomDereference, startLocation, e);
    }
    if (Space::parseCharacter(source, '&')) {
        Symbol e = parseUnaryExpression(source);
        return unaryOperation(atomAddressOf, startLocation, e);
    }
    return parsePowerExpression(source);
}

Symbol parseMultiplicativeExpression(CharacterSource* source)
{
    Symbol e = parseUnaryExpression(source);
    if (!e.valid())
        return Symbol();
    do {
        if (Space::parseCharacter(source, '*')) {
            Symbol e2 = parseUnaryExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomMultiply, e, e2);
            continue;
        }
        if (Space::parseCharacter(source, '/')) {
            Symbol e2 = parseUnaryExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomDivide, e, e2);
            continue;
        }
        if (Space::parseCharacter(source, '%')) {
            Symbol e2 = parseUnaryExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomModulo, e, e2);
            continue;
        }
        return e;
    } while (true);
}

Symbol parseAdditiveExpression(CharacterSource* source)
{
    Symbol e = parseMultiplicativeExpression(source);
    if (!e.valid())
        return Symbol();
    do {
        if (Space::parseCharacter(source, '+')) {
            Symbol e2 = parseMultiplicativeExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomAdd, e, e2);
            continue;
        }
        if (Space::parseCharacter(source, '-')) {
            Symbol e2 = parseMultiplicativeExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomSubtract, e, e2);
            continue;
        }
        return e;
    } while (true);
}

Symbol parseShiftExpression(CharacterSource* source)
{
    Symbol e = parseAdditiveExpression(source);
    if (!e.valid())
        return Symbol();
    do {
        static String leftShift("<<");
        if (Space::parseOperator(source, leftShift)) {
            Symbol e2 = parseAdditiveExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomLeftShift, e, e2);
            continue;
        }
        static String rightShift(">>");
        if (Space::parseOperator(source, rightShift)) {
            Symbol e2 = parseAdditiveExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomRightShift, e, e2);
            continue;
        }
        return e;
    } while (true);
}

Symbol parseComparisonExpression(CharacterSource* source)
{
    Symbol e = parseShiftExpression(source);
    if (!e.valid())
        return Symbol();
    do {
        static String lessThanOrEqualTo("<=");
        if (Space::parseOperator(source, lessThanOrEqualTo)) {
            Symbol e2 = parseShiftExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomLessThanOrEqualTo, e, e2);
            continue;
        }
        static String greaterThanOrEqualTo(">=");
        if (Space::parseOperator(source, greaterThanOrEqualTo)) {
            Symbol e2 = parseShiftExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomGreaterThanOrEqualTo, e, e2);
            continue;
        }
        if (Space::parseCharacter(source, '<')) {
            Symbol e2 = parseShiftExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomLessThan, e, e2);
            continue;
        }
        if (Space::parseCharacter(source, '>')) {
            Symbol e2 = parseShiftExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomGreaterThan, e, e2);
            continue;
        }
        return e;
    } while (true);
}

Symbol parseEqualityExpression(CharacterSource* source)
{
    Symbol e = parseComparisonExpression(source);
    if (!e.valid())
        return Symbol();
    do {
        static String equalTo("==");
        if (Space::parseOperator(source, equalTo)) {
            Symbol e2 = parseComparisonExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomEqualTo, e, e2);
            continue;
        }
        static String notEqualTo("!=");
        if (Space::parseOperator(source, notEqualTo)) {
            Symbol e2 = parseComparisonExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomNotEqualTo, e, e2);
            continue;
        }
        return e;
    } while (true);
}

Symbol parseBitwiseAndExpression(CharacterSource* source)
{
    Symbol e = parseEqualityExpression(source);
    if (!e.valid())
        return Symbol();
    do {
        if (Space::parseCharacter(source, '&')) {
            Symbol e2 = parseEqualityExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomBitwiseAnd, e, e2);
            continue;
        }
        return e;
    } while (true);
}

Symbol parseXorExpression(CharacterSource* source)
{
    Symbol e = parseBitwiseAndExpression(source);
    if (!e.valid())
        return Symbol();
    do {
        DiagnosticLocation location = source->location();
        if (Space::parseCharacter(source, '~')) {
            Symbol e2 = parseBitwiseAndExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomBitwiseXor, e, e2);
            continue;
        }
        return e;
    } while (true);
}

Symbol parseBitwiseOrExpression(CharacterSource* source)
{
    Symbol e = parseXorExpression(source);
    if (!e.valid())
        return Symbol();
    do {
        if (Space::parseCharacter(source, '|')) {
            Symbol e2 = parseXorExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomBitwiseOr, e, e2);
            continue;
        }
        return e;
    } while (true);
}

Symbol parseLogicalAndExpression(CharacterSource* source)
{
    Symbol e = parseBitwiseOrExpression(source);
    if (!e.valid())
        return Symbol();
    do {
        static String logicalAnd("&&");
        if (Space::parseOperator(source, logicalAnd)) {
            Symbol e2 = parseBitwiseOrExpression(source);
            if (!e2.valid())
                throwError(source);
            e = Symbol(atomLogicalAnd, DiagnosticSpan(e.span().start(), e2.span().end()), e, e2);
            continue;
        }
        return e;
    } while (true);
}

Symbol parseExpression(CharacterSource* source)
{
    DiagnosticLocation startLocation = source->location();
    Symbol e = parseLogicalAndExpression(source);
    if (!e.valid())
        return Symbol();
    do {
        static String logicalOr("||");
        if (Space::parseOperator(source, logicalOr)) {
            Symbol e2 = parseLogicalAndExpression(source);
            if (!e2.valid())
                throwError(source);
            e = Symbol(atomLogicalOr, DiagnosticSpan(e.span().start(), e2.span().end()), e, e2);
            continue;
        }
        return e;
    } while (true);
}

void checkBoolean(Symbol expression)
{
    if (typeOfExpression(expression).atom() == atomBoolean)
        return;
    static String error("Expected an expression of type Boolean, this has type ");
    expression.span().throwError(error + typeToString(typeOfExpression(expression)));
}

Symbol typeOfExpression(Symbol expression)
{
    switch (expression.atom()) {
        case atomLogicalOr:
        case atomLogicalAnd:
            checkBoolean(expression[1].symbol());
            checkBoolean(expression[2].symbol());
            return Symbol(atomBoolean);
        case atomFunctionCall:
            // TODO
            break;
        case atomDot:
            // TODO
            break;
    }
}