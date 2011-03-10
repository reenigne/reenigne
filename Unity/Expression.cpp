void throwError(CharacterSource* source)
{
    static String expected("Expected expression");
    source->location().throwError(expected);
}

Symbol parseExpression(CharacterSource* source);

Symbol parseExpressionOrFail(CharacterSource* source)
{
    Symbol expression = parseExpression(source);
    if (!expression.valid())
        throwError(source);
    return expression;
}

Symbol combine(Symbol left, Symbol right, SymbolCache* cache)
{
    if (left.valid())
        return Symbol(atomFunctionCall, Symbol(atomAdd), SymbolArray(left, right), cache);
    return right;
}

int parseHexadecimalCharacter(CharacterSource* source, Span* span)
{
    CharacterSource s = *source;
    int c = s.get(span);
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

SymbolArray parseTemplateArgumentList(CharacterSource* source);

String equalTo("==");
String notEqualTo("!=");
String lessThanOrEqualTo("<=");
String greaterThanOrEqualTo(">=");
String shiftLeft("<<");
String shiftRight(">>");
String addAssignment("+=");
String subtractAssignment("-=");
String multiplyAssignment("*=");
String divideAssignment("/=");
String moduloAssignment("%=");
String shiftLeftAssignment("<<=");
String shiftRightAssignment(">>=");
String bitwiseAndAssignment("&=");
String bitwiseOrAssignment("|=");
String bitwiseXorAssignment("~=");
String powerAssignment("^=");
String increment("++");
String decrement("--");

Symbol parseIdentifier(CharacterSource* source)
{
    CharacterSource s = *source;
    Location startLocation = s.location();
    int startOffset;
    Span startSpan;
    Span endSpan;
    int c = s.get(&startSpan);
    if (c < 'a' || c > 'z')
        return Symbol();
    CharacterSource s2;
    do {
        s2 = s;
        c = s.get(&endSpan);
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
            continue;
        break;
    } while (true);
    int endOffset = s2.offset();
    Location endLocation = s2.location();
    Space::parse(&s2);
    String name = s2.subString(startOffset, endOffset);
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
    String op("operator");
    if (name != op) {
        *source = s2;
        return Symbol(atomIdentifier, name, newSpan(startSpan + endSpan));
    }
    Span span3;
    Atom atom = atomLast;
    CharacterSource s3 = s2;
    if (Space::parseCharacter(&s2, '(', &endSpan)) {
        if (Space::parseCharacter(&s2, ')', &endSpan))
            atom = atomFunctionCall;
        else {
            static String expected("Expected )");
            s2.location().throwError(expected);
        }
    }
    else if (Space::parseCharacter(&s2, '[', &endSpan)) {
        if (Space::parseCharacter(&s2, ']', &endSpan))
            atom = atomFunctionCall;
        else {
            static String expected("Expected ]");
            s2.location().throwError(expected);
        }
    }
    else if (Space::parseOperator(&s2, equalTo, &endSpan))
        atom = atomEqualTo;
    else if (Space::parseCharacter(&s2, '=', &endSpan))
        atom = atomAssignment;
    else if (Space::parseOperator(&s2, addAssignment, &endSpan))
        atom = atomAddAssignment;
    else if (Space::parseOperator(&s2, subtractAssignment, &endSpan))
        atom = atomSubtractAssignment;
    else if (Space::parseOperator(&s2, multiplyAssignment, &endSpan))
        atom = atomMultiplyAssignment;
    else if (Space::parseOperator(&s2, divideAssignment, &endSpan))
        atom = atomDivideAssignment;
    else if (Space::parseOperator(&s2, moduloAssignment, &endSpan))
        atom = atomModuloAssignment;
    else if (Space::parseOperator(&s2, shiftLeftAssignment, &endSpan))
        atom = atomShiftLeftAssignment;
    else if (Space::parseOperator(&s2, shiftRightAssignment, &endSpan))
        atom = atomShiftRightAssignment;
    else if (Space::parseOperator(&s2, bitwiseAndAssignment, &endSpan))
        atom = atomBitwiseAndAssignment;
    else if (Space::parseOperator(&s2, bitwiseOrAssignment, &endSpan))
        atom = atomBitwiseOrAssignment;
    else if (Space::parseOperator(&s2, bitwiseXorAssignment, &endSpan))
        atom = atomBitwiseXorAssignment;
    else if (Space::parseOperator(&s2, powerAssignment, &endSpan))
        atom = atomPowerAssignment;
    else if (Space::parseCharacter(&s2, '|', &endSpan))
        atom = atomBitwiseOr;
    else if (Space::parseCharacter(&s2, '~', &endSpan))
        atom = atomBitwiseXor;
    else if (Space::parseCharacter(&s2, '&', &endSpan))
        atom = atomBitwiseAnd;
    else if (Space::parseOperator(&s2, notEqualTo, &endSpan))
        atom = atomNotEqualTo;
    else if (Space::parseOperator(&s2, lessThanOrEqualTo, &endSpan))
        atom = atomLessThanOrEqualTo;
    else if (Space::parseOperator(&s2, shiftRight, &endSpan))
        atom = atomShiftRight;
    else if (Space::parseCharacter(&s3, '<', &endSpan)) {
        atom = atomLessThan;
        // Only if we know it's not operator<<T>() can we try operator<<()
        CharacterSource s4 = s3;
        SymbolArray templateArgumentList = parseTemplateArgumentList(&s4);
        if (templateArgumentList.count() == 0 && 
            Space::parseOperator(&s2, shiftLeft, &endSpan))
            atom = atomShiftLeft;
        else
            s2 = s3;
    }
    else if (Space::parseOperator(&s2, greaterThanOrEqualTo, &endSpan))
        atom = atomGreaterThanOrEqualTo;
    else if (Space::parseCharacter(&s2, '>', &endSpan))
        atom = atomGreaterThan;
    else if (Space::parseCharacter(&s2, '+', &endSpan))
        atom = atomAdd;
    else if (Space::parseCharacter(&s2, '-', &endSpan))
        atom = atomSubtract;
    else if (Space::parseCharacter(&s2, '/', &endSpan))
        atom = atomDivide;
    else if (Space::parseCharacter(&s2, '*', &endSpan))
        atom = atomMultiply;
    else if (Space::parseCharacter(&s2, '%', &endSpan))
        atom = atomModulo;
    else {
        static String expected("Expected an operator");
        s2.location().throwError(expected);
    }
    return Symbol(atomIdentifier, Symbol(atom), newSpan(startSpan + endSpan));
}

Symbol parseDoubleQuotedString(CharacterSource* source)
{
    static String endOfFile("End of file in string");
    static String endOfLine("End of line in string");
    static String printableCharacter("printable character");
    static String escapedCharacter("escaped character");
    static String hexadecimalDigit("hexadecimal digit");
    static String toString("toString");
    Span span;
    Span startSpan;
    if (!source->parse('"', &startSpan))
        return Symbol();
    Span stringStartSpan = startSpan;
    Span stringEndSpan = startSpan;
    int startOffset = source->offset();
    int endOffset;
    String insert(empty);
    int n;
    int nn;
    String string(empty);
    Symbol expression;
    Symbol part;
    do {
        CharacterSource s = *source;
        endOffset = s.offset();
        int c = s.get(&span);
        if (c < 0x20 && c != 10) {
            if (c == -1)
                source->location().throwError(endOfFile);
            source->throwUnexpected(printableCharacter, String::hexadecimal(c, 2));
        }
        *source = s;
        switch (c) {
            case '"':
                string += s.subString(startOffset, endOffset);
                Space::parse(source);
                return combine(expression,
                    Symbol(atomStringConstant, string,
                        new ExpressionCache(stringStartSpan + span)),
                    new ExpressionCache(startSpan + span));
            case '\\':
                string += s.subString(startOffset, endOffset);
                c = s.get(&stringEndSpan);
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
                        source->assert('+', &stringEndSpan);
                        n = 0;
                        for (int i = 0; i < 4; ++i) {
                            nn = parseHexadecimalCharacter(source, &stringEndSpan);
                            if (nn == -1) {
                                s = *source;
                                source->throwUnexpected(hexadecimalDigit, String::codePoint(s.get()));
                            }
                            n = (n << 4) | nn;
                        }
                        nn = parseHexadecimalCharacter(source, &stringEndSpan);
                        if (nn != -1) {
                            n = (n << 4) | nn;
                            nn = parseHexadecimalCharacter(source, &stringEndSpan);
                            if (nn != -1)
                                n = (n << 4) | nn;
                        }
                        insert = String::codePoint(n);
                        break;
                    default:
                        source->throwUnexpected(escapedCharacter, String::codePoint(c));
                }
                string += insert;
                startOffset = source->offset();
                break;
            case '$':
                part = parseIdentifier(source);
                if (!part.valid()) {
                    if (Space::parseCharacter(source, '(', &span)) {
                        part = parseExpression(source);
                        source->assert(')', &span);
                    }
                    else {
                        static String error("Expected identifier or parenthesized expression");
                        source->location().throwError(error);
                    }
                }
                part = Symbol(atomFunctionCall,
                    Symbol(atomDot, part, Symbol(atomIdentifier, toString),
                        part.cache<ExpressionCache>()),
                    SymbolArray(), part.cache<ExpressionCache>());
                string += s.subString(startOffset, endOffset);
                startOffset = source->offset();
                if (part.valid()) {
                    expression = combine(expression,
                        Symbol(atomStringConstant, string,
                            new ExpressionCache(stringStartSpan + stringEndSpan)),
                        new ExpressionCache(startSpan + stringEndSpan));
                    string = empty;
                    expression = combine(expression, part,
                        new ExpressionCache(startSpan + span));
                }
                break;
            default:
                stringEndSpan = span;
        }
    } while (true);
}

Symbol parseEmbeddedLiteral(CharacterSource* source)
{
    Span startSpan;
    Span endSpan;
    static String empty;
    static String endOfFile("End of file in string");
    if (!source->parse('#', &startSpan))
        return Symbol();
    if (!source->parse('#', &endSpan))
        return Symbol();
    if (!source->parse('#', &endSpan))
        return Symbol();
    int startOffset = source->offset();
    Location location = source->location();
    CharacterSource s = *source;
    do {
        int c = s.get();
        if (c == -1)
            source->location().throwError(endOfFile);
        if (c == 10)
            break;
        *source = s;
    } while (true);
    int endOffset = source->offset();
    String terminator = source->subString(startOffset, endOffset);
    startOffset = s.offset();
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
            if (s2.get(&endSpan) != '#')
                continue;
            string += s.subString(startOffset, source->offset());
            *source = s2;
            Space::parse(source);
            return Symbol(atomStringConstant, string,
                new ExpressionCache(startSpan + endSpan));
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
                        if (s2.get(&endSpan) != '#')
                            break;
                        string += s.subString(startOffset, source->offset());
                        *source = s2;
                        Space::parse(source);
                        return Symbol(atomStringConstant, string,
                            new ExpressionCache(startSpan + endSpan));
                    }
                    int cs = s2.get();
                    if (ct != cs)
                        break;
                } while (true);
            }
        if (c == 10) {
            string += s.subString(startOffset, source->offset()) + String::codePoint(10);
            startOffset = s.offset();
        }
    } while (true);
}

Symbol parseInteger(CharacterSource* source)
{
    CharacterSource s = *source;
    int n = 0;
    Span span;
    int c = s.get(&span);
    if (c < '0' || c > '9')
        return Symbol();
    do {
        n = n*10 + c - '0';
        *source = s;
        Span span2;
        c = s.get(&span2);
        if (c < '0' || c > '9') {
            Space::parse(source);
            return Symbol(atomIntegerConstant, n, new ExpressionCache(span));
        }
        span += span2;
    } while (true);
}

Symbol parseExpressionElement(CharacterSource* source)
{
    Location location = source->location();
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
        String s = e[1].string();
        static String trueKeyword("true");
        if (s == trueKeyword)
            return Symbol(atomTrue, new ExpressionCache(spanOf(e)));
        static String falseKeyword("false");
        if (s == falseKeyword)
            return Symbol(atomFalse, new ExpressionCache(spanOf(e)));
        static String nullKeyword("null");
        if (s == nullKeyword)
            return Symbol(atomNull, new ExpressionCache(spanOf(e)));
        return e;
    }
    Span span;
    if (Space::parseCharacter(source, '(', &span)) {
        e = parseExpression(source);
        if (!e.valid())
            return Symbol();
        Span span2;
        Space::assertCharacter(source, ')', &span2);
        return Symbol(e.atom(), e.tail(), new ExpressionCache(span + span2));
    }
    CharacterSource s2 = *source;
    if (Space::parseCharacter(&s2, '{', &span)) {
        SymbolList expressions;
        do {
            e = parseExpression(&s2);
            if (!e.valid())
                return Symbol();
            expressions.add(e);
            Span span2;
            bool seenComma = Space::parseCharacter(&s2, ',', &span2);
            if (Space::parseCharacter(&s2, '}', &span2)) {
                *source = s2;
                return Symbol(atomArrayLiteral, SymbolArray(expressions),
                    newSpan(span + span2));
            }
            if (!seenComma)
                return Symbol();
        } while (true);

    }
    return Symbol();
}

SymbolArray parseExpressionList(CharacterSource* source)
{
    SymbolList list;
    Symbol expression = parseExpression(source);
    if (!expression.valid())
        return list;
    list.add(expression);
    Span span;
    while (Space::parseCharacter(source, ',', &span))
        list.add(parseExpressionOrFail(source));
    return list;
}

Symbol parseFunctionCallExpression(CharacterSource* source)
{
    Symbol e = parseExpressionElement(source);
    if (!e.valid())
        return Symbol();
    do {
        Span span;
        if (!Space::parseCharacter(source, '(', &span))
            break;
        SymbolArray arguments = parseExpressionList(source);
        Space::assertCharacter(source, ')', &span);
        e = Symbol(atomFunctionCall, e, arguments,
            new ExpressionCache(spanOf(e) + span));
    } while (true);
    return e;
}

Symbol parseEmitExpression(CharacterSource* source)
{
    Span span;
    String emitKeyword("_emit");
    if (Space::parseKeyword(source, emitKeyword, &span)) {
        SymbolArray argumentList = parseTemplateArgumentList(source);
        if (!argumentList.valid() || argumentList.count() != 1) {
            static String error("Expected type list with single type");
            source->location().throwError(error);
        }
        Symbol e = parseEmitExpression(source);
        return Symbol(atomEmit, argumentList, e, 
            new ExpressionCache(span + spanOf(e)));
    }
    return parseFunctionCallExpression(source);
}

Symbol binaryOperation(Atom atom, Span span, Symbol left, Symbol right)
{
    return Symbol(atomFunctionCall, Symbol(atom, newSpan(span)),
        SymbolArray(left, right),
        new ExpressionCache(spanOf(left) + spanOf(right)));
}

Symbol parsePowerExpression(CharacterSource* source)
{
    Symbol e = parseEmitExpression(source);
    if (!e.valid())
        return Symbol();
    Span span;
    if (Space::parseCharacter(source, '^', &span)) {
        Symbol e2 = parsePowerExpression(source);
        if (!e2.valid())
            throwError(source);
        e = binaryOperation(atomPower, span, e, e2);
    }
    return e;
}

Symbol unaryOperation(Atom atom, Span span, Symbol e)
{
    return Symbol(atomFunctionCall, Symbol(atom, newSpan(span)),
        SymbolArray(e), new ExpressionCache(span + spanOf(e)));
}

Symbol parseUnaryExpression(CharacterSource* source)
{
    Span span;
    if (Space::parseCharacter(source, '~', &span) || 
        Space::parseCharacter(source, '!', &span)) {
        Symbol e = parseUnaryExpression(source);
        return unaryOperation(atomNot, span, e);
    }
    if (Space::parseCharacter(source, '+', &span)) {
        Symbol e = parseUnaryExpression(source);
        return unaryOperation(atomPositive, span, e);
    }
    if (Space::parseCharacter(source, '-', &span)) {
        Symbol e = parseUnaryExpression(source);
        return unaryOperation(atomSubtract, span, e);
    }
    if (Space::parseCharacter(source, '*', &span)) {
        Symbol e = parseUnaryExpression(source);
        return unaryOperation(atomDereference, span, e);
    }
    if (Space::parseCharacter(source, '&', &span)) {
        Symbol e = parseUnaryExpression(source);
        return unaryOperation(atomAddressOf, span, e);
    }
    return parsePowerExpression(source);
}

Symbol parseMultiplicativeExpression(CharacterSource* source)
{
    Symbol e = parseUnaryExpression(source);
    if (!e.valid())
        return Symbol();
    do {
        Span span;
        if (Space::parseCharacter(source, '*', &span)) {
            Symbol e2 = parseUnaryExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomMultiply, span, e, e2);
            continue;
        }
        if (Space::parseCharacter(source, '/', &span)) {
            Symbol e2 = parseUnaryExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomDivide, span, e, e2);
            continue;
        }
        if (Space::parseCharacter(source, '%', &span)) {
            Symbol e2 = parseUnaryExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomModulo, span, e, e2);
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
        Span span;
        if (Space::parseCharacter(source, '+', &span)) {
            Symbol e2 = parseMultiplicativeExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomAdd, span, e, e2);
            continue;
        }
        if (Space::parseCharacter(source, '-', &span)) {
            Symbol e2 = parseMultiplicativeExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomSubtract, span, e, e2);
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
        Span span;
        if (Space::parseOperator(source, shiftLeft, &span)) {
            Symbol e2 = parseAdditiveExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomShiftLeft, span, e, e2);
            continue;
        }
        if (Space::parseOperator(source, shiftRight, &span)) {
            Symbol e2 = parseAdditiveExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomShiftRight, span, e, e2);
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
        Span span;
        if (Space::parseOperator(source, lessThanOrEqualTo, &span)) {
            Symbol e2 = parseShiftExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomLessThanOrEqualTo, span, e, e2);
            continue;
        }
        if (Space::parseOperator(source, greaterThanOrEqualTo, &span)) {
            Symbol e2 = parseShiftExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomGreaterThanOrEqualTo, span, e, e2);
            continue;
        }
        if (Space::parseCharacter(source, '<', &span)) {
            Symbol e2 = parseShiftExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomLessThan, span, e, e2);
            continue;
        }
        if (Space::parseCharacter(source, '>', &span)) {
            Symbol e2 = parseShiftExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomGreaterThan, span, e, e2);
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
        Span span;
        if (Space::parseOperator(source, equalTo, &span)) {
            Symbol e2 = parseComparisonExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomEqualTo, span, e, e2);
            continue;
        }
        if (Space::parseOperator(source, notEqualTo, &span)) {
            Symbol e2 = parseComparisonExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomNotEqualTo, span, e, e2);
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
        Span span;
        if (Space::parseCharacter(source, '&', &span)) {
            Symbol e2 = parseEqualityExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomBitwiseAnd, span, e, e2);
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
        Span span;
        if (Space::parseCharacter(source, '~', &span)) {
            Symbol e2 = parseBitwiseAndExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomBitwiseXor, span, e, e2);
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
        Span span;
        if (Space::parseCharacter(source, '|', &span)) {
            Symbol e2 = parseXorExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomBitwiseOr, span, e, e2);
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
        Span span;
        static String logicalAnd("&&");
        if (Space::parseOperator(source, logicalAnd, &span)) {
            Symbol e2 = parseBitwiseOrExpression(source);
            if (!e2.valid())
                throwError(source);
            e = Symbol(atomLogicalAnd, Symbol(atomLogicalAnd, newSpan(span)),
                e, e2, new ExpressionCache(spanOf(e) + spanOf(e2)));
            continue;
        }
        return e;
    } while (true);
}

Symbol parseExpression(CharacterSource* source)
{
    Location startLocation = source->location();
    Symbol e = parseLogicalAndExpression(source);
    if (!e.valid())
        return Symbol();
    do {
        Span span;
        static String logicalOr("||");
        if (Space::parseOperator(source, logicalOr, &span)) {
            Symbol e2 = parseLogicalAndExpression(source);
            if (!e2.valid())
                throwError(source);
            e = Symbol(atomLogicalOr, Symbol(atomLogicalOr, newSpan(span)), e,
                e2, new ExpressionCache(spanOf(e) + spanOf(e2)));
            continue;
        }
        return e;
    } while (true);
}