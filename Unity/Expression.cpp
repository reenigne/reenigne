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
    Location endLocation = s2.location();
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
    String op("operator");
    if (name != op) {
        *source = s2;
        return Symbol(atomIdentifier, name, newSpan(startLocation, endLocation));
    }
    Span span2;
    Atom atom = atomLast;
    if (Space::parseCharacter(&s2, '|', span2))
        atom = atomBitwiseOr;
    else if (Space::parseCharacter(&s2, '~', span2))
        atom = atomBitwiseXor;
    else if (Space::parseCharacter(&s2, '&', span2))
        atom = atomBitwiseAnd;
    else if (Space::parseOperator(&s2, equalTo, span2))
        atom = atomEqualTo;
    else if (Space::parseOperator(&s2, notEqualTo, span2))
        atom = atomNotEqualTo;
    else if (Space::parseOperator(&s2, lessThanOrEqualTo, span2))
        atom = atomLessThanOrEqualTo;
    else if (Space::parseCharacter(&s2, '<', span2))
        atom = atomLessThan;
    else if (Space::parseOperator(&s2, greaterThanOrEqualTo, span2))
        atom = atomGreaterThanOrEqualTo;
    else if (Space::parseCharacter(&s2, '>', span2))
        atom = atomGreaterThan;
    else if (Space::parseOperator(&s2, shiftLeft, span2))
        atom = atomShiftLeft;
    else if (Space::parseOperator(&s2, shiftRight, span2))
        atom = atomShiftRight;
    else if (Space::parseCharacter(&s2, '+', span2))
        atom = atomAdd;
    else if (Space::parseCharacter(&s2, '-', span2))
        atom = atomSubtract;
    else if (Space::parseCharacter(&s2, '/', span2))
        atom = atomDivide;
    else if (Space::parseCharacter(&s2, '*', span2))
        atom = atomMultiply;
    else if (Space::parseCharacter(&s2, '%', span2))
        atom = atomModulo;
    else if (Space::parseCharacter(&s2, '(', span2)) {
        if (Space::parseCharacter(&s2, ')', span2))
            atom = atomFunctionCall;
        else {
            static String expected("Expected )");
            s2.location().throwError(expected);
        }
    }
    else if (Space::parseCharacter(&s2, '[', span2)) {
        if (Space::parseCharacter(&s2, ']', span2))
            atom = atomFunctionCall;
        else {
            static String expected("Expected ]");
            s2.location().throwError(expected);
        }
    }
    else if (Space::parseCharacter(&s2, '=', span2))
        atom = atomAssignment;
    else if (Space::parseOperator(&s2, addAssignment, span2))
        atom = atomAddAssignment;
    else if (Space::parseOperator(&s2, subtractAssignment, span2))
        atom = atomSubtractAssignment;
    else if (Space::parseOperator(&s2, multiplyAssignment, span2))
        atom = atomMultiplyAssignment;
    else if (Space::parseOperator(&s2, divideAssignment, span2))
        atom = atomDivideAssignment;
    else if (Space::parseOperator(&s2, moduloAssignment, span2))
        atom = atomModuloAssignment;
    else if (Space::parseOperator(&s2, shiftLeftAssignment, span2))
        atom = atomShiftLeftAssignment;
    else if (Space::parseOperator(&s2, shiftRightAssignment, span2))
        atom = atomShiftRightAssignment;
    else if (Space::parseOperator(&s2, bitwiseAndAssignment, span2))
        atom = atomBitwiseAndAssignment;
    else if (Space::parseOperator(&s2, bitwiseOrAssignment, span2))
        atom = atomBitwiseOrAssignment;
    else if (Space::parseOperator(&s2, bitwiseXorAssignment, span2))
        atom = atomBitwiseXorAssignment;
    else if (Space::parseOperator(&s2, powerAssignment, span2))
        atom = atomPowerAssignment;
    else {
        static String expected("Expected an operator");
        s2.location().throwError(expected);
    }
    return Symbol(atom, newSpan(startLocation, span2.end()));
}

Symbol parseDoubleQuotedString(CharacterSource* source)
{
    static String endOfFile("End of file in string");
    static String endOfLine("End of line in string");
    static String printableCharacter("printable character");
    static String escapedCharacter("escaped character");
    static String hexadecimalDigit("hexadecimal digit");
    static String toString("toString");
    Location startLocation = source->location();
    Location stringStartLocation = startLocation;
    Location stringEndLocation;
    Span span;
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
                    Symbol(atomStringConstant, string,
                        new ExpressionCache(Span(stringStartLocation, s.location()))),
                    new ExpressionCache(Span(startLocation, s.location())));
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
                    if (Space::parseCharacter(source, '(', span)) {
                        part = parseExpression(source);
                        source->assert(')');
                    }
                }
                part = Symbol(atomFunctionCall,
                    Symbol(atomDot, part, Symbol(atomIdentifier, toString),
                        part.cache<ExpressionCache>()),
                    SymbolArray(), part.cache<ExpressionCache>());
                string += s.subString(start, end);
                start = source->offset();
                if (part.valid()) {
                    expression = combine(expression,
                        Symbol(atomStringConstant, string,
                            new ExpressionCache(Span(stringStartLocation, stringEndLocation))),
                        new ExpressionCache(Span(startLocation, stringEndLocation)));
                    string = empty;
                    stringStartLocation = source->location();
                    expression = combine(expression, part,
                        new ExpressionCache(Span(startLocation, stringStartLocation)));
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
            return Symbol(atomStringConstant, string,
                new ExpressionCache(Span(location, s2.location())));
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
                        return Symbol(atomStringConstant, string,
                            new ExpressionCache(Span(location, s2.location())));
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
    Location location = s.location();
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
            return Symbol(atomIntegerConstant, n,
                new ExpressionCache(Span(location, s.location())));
        }
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
    if (Space::parseCharacter(source, '(', span)) {
        e = parseExpression(source);
        Location end = Space::assertCharacter(source, ')');
        return Symbol(e.atom(), e.tail(),
            new ExpressionCache(Span(location, end)));
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
    while (Space::parseCharacter(source, ',', span))
        list.add(parseExpressionOrFail(source));
    return list;
}

Symbol parseFunctionCallExpression(CharacterSource* source)
{
    Location startLocation = source->location();
    Symbol e = parseExpressionElement(source);
    if (!e.valid())
        return Symbol();
    do {
        Span span;
        if (!Space::parseCharacter(source, '(', span))
            break;
        SymbolArray arguments = parseExpressionList(source);
        Location location = Space::assertCharacter(source, ')');
        e = Symbol(atomFunctionCall, e, arguments,
            new ExpressionCache(Span(startLocation, location)));
    } while (true);
    return e;
}

Symbol binaryOperation(Atom atom, Span span, Symbol left, Symbol right)
{
    return Symbol(atomFunctionCall, Symbol(atom, newSpan(span)),
        SymbolArray(left, right),
        new ExpressionCache(Span(spanOf(left).start(), spanOf(right).end())));
}

Symbol parsePowerExpression(CharacterSource* source)
{
    Symbol e = parseFunctionCallExpression(source);
    if (!e.valid())
        return Symbol();
    Span span;
    if (Space::parseCharacter(source, '^', span)) {
        Symbol e2 = parsePowerExpression(source);
        if (!e2.valid())
            throwError(source);
        e = binaryOperation(atomPower, span, e, e2);
    }
    return e;
}

Symbol unaryOperation(Atom atom, Span span, Location location, Symbol e)
{
    return Symbol(atomFunctionCall, Symbol(atom, newSpan(span)), SymbolArray(e),
        new ExpressionCache(Span(location, spanOf(e).end())));
}

Symbol parseUnaryExpression(CharacterSource* source)
{
    Location startLocation = source->location();
    Span span;
    if (Space::parseCharacter(source, '~', span) || Space::parseCharacter(source, '!', span)) {
        Symbol e = parseUnaryExpression(source);
        return unaryOperation(atomNot, span, startLocation, e);
    }
    if (Space::parseCharacter(source, '+', span)) {
        Symbol e = parseUnaryExpression(source);
        return unaryOperation(atomPositive, span, startLocation, e);
    }
    if (Space::parseCharacter(source, '-', span)) {
        Symbol e = parseUnaryExpression(source);
        return unaryOperation(atomSubtract, span, startLocation, e);
    }
    if (Space::parseCharacter(source, '*', span)) {
        Symbol e = parseUnaryExpression(source);
        return unaryOperation(atomDereference, span, startLocation, e);
    }
    if (Space::parseCharacter(source, '&', span)) {
        Symbol e = parseUnaryExpression(source);
        return unaryOperation(atomAddressOf, span, startLocation, e);
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
        if (Space::parseCharacter(source, '*', span)) {
            Symbol e2 = parseUnaryExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomMultiply, span, e, e2);
            continue;
        }
        if (Space::parseCharacter(source, '/', span)) {
            Symbol e2 = parseUnaryExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomDivide, span, e, e2);
            continue;
        }
        if (Space::parseCharacter(source, '%', span)) {
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
        if (Space::parseCharacter(source, '+', span)) {
            Symbol e2 = parseMultiplicativeExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomAdd, span, e, e2);
            continue;
        }
        if (Space::parseCharacter(source, '-', span)) {
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
        if (Space::parseOperator(source, shiftLeft, span)) {
            Symbol e2 = parseAdditiveExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomShiftLeft, span, e, e2);
            continue;
        }
        if (Space::parseOperator(source, shiftRight, span)) {
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
        if (Space::parseOperator(source, lessThanOrEqualTo, span)) {
            Symbol e2 = parseShiftExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomLessThanOrEqualTo, span, e, e2);
            continue;
        }
        if (Space::parseOperator(source, greaterThanOrEqualTo, span)) {
            Symbol e2 = parseShiftExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomGreaterThanOrEqualTo, span, e, e2);
            continue;
        }
        if (Space::parseCharacter(source, '<', span)) {
            Symbol e2 = parseShiftExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomLessThan, span, e, e2);
            continue;
        }
        if (Space::parseCharacter(source, '>', span)) {
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
        if (Space::parseOperator(source, equalTo, span)) {
            Symbol e2 = parseComparisonExpression(source);
            if (!e2.valid())
                throwError(source);
            e = binaryOperation(atomEqualTo, span, e, e2);
            continue;
        }
        if (Space::parseOperator(source, notEqualTo, span)) {
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
        if (Space::parseCharacter(source, '&', span)) {
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
        if (Space::parseCharacter(source, '~', span)) {
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
        if (Space::parseCharacter(source, '|', span)) {
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
        if (Space::parseOperator(source, logicalAnd, span)) {
            Symbol e2 = parseBitwiseOrExpression(source);
            if (!e2.valid())
                throwError(source);
            e = Symbol(atomLogicalAnd, Symbol(atomLogicalAnd, newSpan(span)),
                e, e2, new ExpressionCache(Span(spanOf(e).start(), spanOf(e2).end())));
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
        if (Space::parseOperator(source, logicalOr, span)) {
            Symbol e2 = parseLogicalAndExpression(source);
            if (!e2.valid())
                throwError(source);
            e = Symbol(atomLogicalOr, Symbol(atomLogicalOr, newSpan(span)), e,
                e2, new ExpressionCache(Span(spanOf(e).start(), spanOf(e2).end())));
            continue;
        }
        return e;
    } while (true);
}