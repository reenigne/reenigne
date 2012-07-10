template<class T> class ExpressionTemplate : public ParseTreeObject
{
public:
    Expression(const Implementation* implementation)
      : ParseTreeObjec(implementation) { }

    static Expression parse(CharacterSource* source)
    {
        return LogicalOrExpression::parse(source);
    }
    static Expression parseOrFail(CharacterSource* source)
    {
        Symbol expression = parseExpression(source);
        if (!expression.valid())
            throwError(source);
        return expression;
    }
protected:
    static Expression parseElement(CharacterSource* source)
    {
        Location location = source->location();
        Expression e = parseDoubleQuotedString(source);
        if (e.valid())
            return e;
        e = parseEmbeddedLiteral(source);
        if (e.valid())
            return e;
        e = parseInteger(source);
        if (e.valid())
            return e;
        Identifier i = parseIdentifier(source);
        if (i.valid()) {
            String s = i.name();
            if (s == "true")
                return new TrueImplementation(i.span());
            if (s == "false")
                return new FalseImplementation(i.span());
            if (s == "null")
                return new NullImplementation(i.span());
            return i;
        }
        Span span;
        CharacterSource s2 = *source;
        if (Space::parseCharacter(&s2, '(', &span)) {
            e = parseExpression(&s2);
            if (!e.valid())
                return e;
            Span span2;
            Space::assertCharacter(&s2, ')', &span2);
            *source = s2;
            return e;
        }
        if (Space::parseCharacter(&s2, '{', &span)) {
            List<Expression> expressions;
            do {
                e = parseExpression(&s2);
                if (!e.valid())
                    return e;
                expressions.add(e);
                Span span2;
                bool seenComma = Space::parseCharacter(&s2, ',', &span2);
                if (Space::parseCharacter(&s2, '}', &span2)) {
                    *source = s2;
                    return new ArrayLiteralImplementation(expressions,
                        span + span2);
                }
                if (!seenComma)
                    return Expression();
            } while (true);
        }
        return e;
    }

    static void throwError(CharacterSource* source)
    {
        source->location().throwError("Expected expression");
    }
private:
    class TrueImplementation : public Implementation
    {
    public:
        TrueImplementation(const Span& span) : Implementation(span) { }
    };
    class FalseImplementation : public Implementation
    {
    public:
        FalseImplementation(const Span& span) : Implementation(span) { }
    };
    class NullImplementation : public Implementation
    {
    public:
        NullImplementation(const Span& span) : Implementation(span) { }
    };
    class ArrayLiteralImplementation : public Implementation
    {
    public:
        ArrayLiteralImplementation(const List<Expression>& expressions,
            const Span& span)
          : Implementation(span), _expressions(expressions) { }
    private:
        List<Expression> _expressions;
    };
};

template<class T> class BinaryExpression : public Expression
{
protected:
    template<class U> static Expression parse(CharacterSource* source,
        const String& op)
    {
        Expression e = U::parse(source);
        if (!e.valid())
            return e;
        do {
            Span span;
            if (Space::parseOperator(source, op, &span)) {
                Expression e2 = U::parse(source);
                if (!e2.valid())
                    throwError(source);
                e = new Implementation(e, span, e2);
                continue;
            }
            return e;
        } while (true);
    }
    class Implementation : public Expression::Implementation
    {
    public:
        Implementation(const Expression& left, const Span& operatorSpan,
            const Expression& right)
          : Expression::Implementation(left.span() + right.span()),
            _left(left), _right(right), _operatorSpan(operatorSpan) { }
    private:
        Expression _left;
        Expression _right;
        Span _operatorSpan;
    };
};

class FunctionCallExpression : public Expression
{
public:
    static List<Expression> parseList(CharacterSource* source)
    {
        List<Expression> list;
        Expression expression = Expression::parse(source);
        if (!expression.valid())
            return list;
        list.add(expression);
        Span span;
        while (Space::parseCharacter(source, ',', &span))
            list.add(parseOrFail(source));
        return list;
    }

    static Expression parse(CharacterSource* source)
    {
        Expression e = parseElement(source);
        if (!e.valid())
            return e;
        do {
            Span span;
            if (!Space::parseCharacter(source, '(', &span))
                break;
            List<Expression> arguments = parseList(source);
            Space::assertCharacter(source, ')', &span);
            e = FunctionCallExpression(
                new Implementation(e, arguments, e.span() + span));
        } while (true);
        return e;
    }

    static Expression parsePower(CharacterSource* source)
    {
        Expression e = parse(source);
        if (!e.valid())
            return e;
        Span span;
        if (Space::parseCharacter(source, '^', &span)) {
            Expression e2 = parsePower(source);
            if (!e2.valid())
                throwError(source);
            e = binary(OperatorPower(), span, e, e2);
        }
        return e;
    }

    static Expression parseUnary(CharacterSource* source)
    {
        Span span;
        if (Space::parseCharacter(source, '~', &span) ||
            Space::parseCharacter(source, '!', &span)) {
            Expression e = parseUnary(source);
            return unary(OperatorNot(), span, e);
        }
        if (Space::parseCharacter(source, '+', &span)) {
            Expression e = parseUnary(source);
            return unary(OperatorPositive(), span, e);
        }
        if (Space::parseCharacter(source, '-', &span)) {
            Expression e = parseUnary(source);
            return unary(OperatorSubtract(), span, e);
        }
        if (Space::parseCharacter(source, '*', &span)) {
            Expression e = parseUnary(source);
            return unary(OperatorDereference(), span, e);
        }
        if (Space::parseCharacter(source, '&', &span)) {
            Expression e = parseUnary(source);
            return unary(OperatorAddressOf(), span, e);
        }
        return parsePower(source);
    }

    static Expression parseMultiplicative(CharacterSource* source)
    {
        Expression e = parseUnary(source);
        if (!e.valid())
            return e;
        do {
            Span span;
            if (Space::parseCharacter(source, '*', &span)) {
                Expression e2 = parseUnary(source);
                if (!e2.valid())
                    throwError(source);
                e = binary(OperatorMultiply(), span, e, e2);
                continue;
            }
            if (Space::parseCharacter(source, '/', &span)) {
                Expression e2 = parseUnary(source);
                if (!e2.valid())
                    throwError(source);
                e = binary(OperatorDivide(), span, e, e2);
                continue;
            }
            if (Space::parseCharacter(source, '%', &span)) {
                Expression e2 = parseUnary(source);
                if (!e2.valid())
                    throwError(source);
                e = binary(OperatorModulo(), span, e, e2);
                continue;
            }
            return e;
        } while (true);
    }

    static Expression parseAdditive(CharacterSource* source)
    {
        Expression e = parseMultiplicative(source);
        if (!e.valid())
            return e;
        do {
            Span span;
            if (Space::parseCharacter(source, '+', &span)) {
                Expression e2 = parseMultiplicative(source);
                if (!e2.valid())
                    throwError(source);
                e = binary(OperatorAdd(), span, e, e2);
                continue;
            }
            if (Space::parseCharacter(source, '-', &span)) {
                Expression e2 = parseMultiplicative(source);
                if (!e2.valid())
                    throwError(source);
                e = binary(OperatorSubtract(), span, e, e2);
                continue;
            }
            return e;
        } while (true);
    }

    static Expression parseShift(CharacterSource* source)
    {
        Expression e = parseAdditive(source);
        if (!e.valid())
            return e;
        do {
            Span span;
            if (Space::parseOperator(source, shiftLeft, &span)) {
                Expression e2 = parseAdditive(source);
                if (!e2.valid())
                    throwError(source);
                e = binary(OperatorShiftLeft(), span, e, e2);
                continue;
            }
            if (Space::parseOperator(source, shiftRight, &span)) {
                Expression e2 = parseAdditive(source);
                if (!e2.valid())
                    throwError(source);
                e = binary(OperatorShiftRight(), span, e, e2);
                continue;
            }
            return e;
        } while (true);
    }

    static Expression parseComparison(CharacterSource* source)
    {
        Expression e = parseShift(source);
        if (!e.valid())
            return e;
        do {
            Span span;
            if (Space::parseOperator(source, lessThanOrEqualTo, &span)) {
                Expression e2 = parseShift(source);
                if (!e2.valid())
                    throwError(source);
                e = binary(OperatorLessThanOrEqualTo(), span, e, e2);
                continue;
            }
            if (Space::parseOperator(source, greaterThanOrEqualTo, &span)) {
                Expression e2 = parseShift(source);
                if (!e2.valid())
                    throwError(source);
                e = binary(OperatorGreaterThanOrEqualTo(), span, e, e2);
                continue;
            }
            if (Space::parseCharacter(source, '<', &span)) {
                Expression e2 = parseShift(source);
                if (!e2.valid())
                    throwError(source);
                e = binary(OperatorLessThan(), span, e, e2);
                continue;
            }
            if (Space::parseCharacter(source, '>', &span)) {
                Expression e2 = parseShift(source);
                if (!e2.valid())
                    throwError(source);
                e = binary(OperatorGreaterThan(), span, e, e2);
                continue;
            }
            return e;
        } while (true);
    }

    static Expression parseEquality(CharacterSource* source)
    {
        Expression e = parseComparison(source);
        if (!e.valid())
            return e;
        do {
            Span span;
            if (Space::parseOperator(source, "==", &span)) {
                Expression e2 = parseComparison(source);
                if (!e2.valid())
                    throwError(source);
                e = binary(OperatorEqualTo(), span, e, e2);
                continue;
            }
            if (Space::parseOperator(source, "!=", &span)) {
                Expression e2 = parseComparison(source);
                if (!e2.valid())
                    throwError(source);
                e = binary(OperatorNotEqualTo(), span, e, e2);
                continue;
            }
            return e;
        } while (true);
    }

    static Expression parseBitwiseAnd(CharacterSource* source)
    {
        Expression e = parseEquality(source);
        if (!e.valid())
            return e;
        do {
            Span span;
            if (Space::parseCharacter(source, '&', &span)) {
                Expression e2 = parseEquality(source);
                if (!e2.valid())
                    throwError(source);
                e = binary(OperatorBitwiseAnd(), span, e, e2);
                continue;
            }
            return e;
        } while (true);
    }

    static Expression parseXor(CharacterSource* source)
    {
        Expression e = parseBitwiseAnd(source);
        if (!e.valid())
            return e;
        do {
            Span span;
            if (Space::parseCharacter(source, '~', &span)) {
                Expression e2 = parseBitwiseAnd(source);
                if (!e2.valid())
                    throwError(source);
                e = binary(OperatorBitwiseXor(), span, e, e2);
                continue;
            }
            return e;
        } while (true);
    }

    static Expression parseBitwiseOr(CharacterSource* source)
    {
        Expression e = parseXor(source);
        if (!e.valid())
            return e;
        do {
            Span span;
            if (Space::parseCharacter(source, '|', &span)) {
                Expression e2 = parseXor(source);
                if (!e2.valid())
                    throwError(source);
                e = binary(OperatorBitwiseOr(), span, e, e2);
                continue;
            }
            return e;
        } while (true);
    }

private:
    FunctionCallExpression(const Implementation* implementation)
      : Expression(implementation) { }

    static FunctionCallExpression unary(const Operator& op, const Span& span,
        const Expression& expression)
    {
        Identifier identifier(op, span);
        List<Expression> arguments;
        arguments.add(expression);
        return FunctionCallExpression(new Implementation(identifier, arguments,
            span + expression.span()));
    }

    static FunctionCallExpression binary(const Operator& op, const Span& span,
        const Expression& left, const Expression& right)
    {
        Identifier identifier(op, span);
        List<Expression> arguments;
        arguments.add(left);
        arguments.add(right);
        return FunctionCallExpression(new Implementation(identifier, arguments,
            left.span() + right.span()));
    }

    class Implementation : public Expression::Implementation
    {
    public:
        Implementation(const Expression& function,
            const List<Expression>& arguments, const Span& span)
          : Expression::Implementation(span), _function(function),
            _arguments(arguments) { }
    private:
        Expression _function;
        List<Expression> _arguments;
    };
};

class LogicalAndExpression : public BinaryExpression<LogicalAndExpression>
{
public:
    static Expression parse(CharacterSource* source)
    {
        Expression e = FunctionCallExpression::parseBitwiseOr(source);
        if (!e.valid())
            return e;
        do {
            Span span;
            if (Space::parseOperator(source, "&&", &span)) {
                Expression e2 = FunctionCallExpression::parseBitwiseOr(source);
                if (!e2.valid())
                    throwError(source);
                e = new Implementation(e, e2, span);
                continue;
            }
            return e;
        } while (true);
    }
private:
    class Implementation : public BinaryExpression::Implementation
    {
    public:
        Implementation(const Expression& left, const Expression& right,
            const Span& operatorSpan)
          : BinaryExpression::Implementation(left, operatorSpan, right) { }
    };
};

class LogicalOrExpression : public BinaryExpression<LogicalOrExpression>
{
public:
    static Expression parse(CharacterSource* source)
    {
        Expression e = LogicalAndExpression::parse(source);
        if (!e.valid())
            return e;
        do {
            Span span;
            if (Space::parseOperator(source, "&&", &span)) {
                Expression e2 = LogicalAndExpression::parse(source);
                if (!e2.valid())
                    throwError(source);
                e = new Implementation(e, e2, span);
                continue;
            }
            return e;
        } while (true);
    }
private:
    class Implementation : public BinaryExpression::Implementation
    {
    public:
        Implementation(const Expression& left, const Expression& right,
            const Span& operatorSpan)
          : BinaryExpression::Implementation(left, operatorSpan, right) { }
    };
};

Symbol combine(Symbol left, Symbol right, SymbolCache* cache)
{
    if (left.valid())
        return Symbol(atomFunctionCall, Symbol(atomAdd),
            SymbolArray(left, right), cache);
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

Symbol parseDoubleQuotedString(CharacterSource* source)
{
    Span span;
    Span startSpan;
    if (!source->parse('"', &startSpan))
        return Symbol();
    Span stringStartSpan = startSpan;
    Span stringEndSpan = startSpan;
    int startOffset = source->offset();
    int endOffset;
    String insert;
    int n;
    int nn;
    String string;
    Symbol expression;
    Symbol part;
    do {
        CharacterSource s = *source;
        endOffset = s.offset();
        int c = s.get(&span);
        if (c < 0x20 && c != 10) {
            if (c == -1)
                source->location().throwError("End of file in string");
            source->throwUnexpected("printable character");
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
                        source->location().throwError("End of line in string");
                    if (c == -1)
                        source->location().throwError("End of file in string");
                    source->throwUnexpected("escaped character");
                }
                *source = s;
                switch (c) {
                    case 'n':
                        insert = "\n";
                        break;
                    case 't':
                        insert = codePoint(9);
                        break;
                    case '$':
                        insert = "$";
                        break;
                    case '"':
                        insert = "\"";
                        break;
                    case '\'':
                        insert = "'";
                        break;
                    case '`':
                        insert = "`";
                        break;
                    case 'U':
                        source->assert('+', &stringEndSpan);
                        n = 0;
                        for (int i = 0; i < 4; ++i) {
                            nn = parseHexadecimalCharacter(source,
                                &stringEndSpan);
                            if (nn == -1) {
                                s = *source;
                                source->throwUnexpected("hexadecimal digit");
                            }
                            n = (n << 4) | nn;
                        }
                        nn = parseHexadecimalCharacter(source, &stringEndSpan);
                        if (nn != -1) {
                            n = (n << 4) | nn;
                            nn = parseHexadecimalCharacter(source,
                                &stringEndSpan);
                            if (nn != -1)
                                n = (n << 4) | nn;
                        }
                        insert = codePoint(n);
                        break;
                    default:
                        source->throwUnexpected("escaped character");
                }
                string += insert;
                startOffset = source->offset();
                break;
            case '$':
                part = parseIdentifier(source);
                if (!part.valid()) {
                    if (Space::parseCharacter(source, '(', &span)) {
                        part = parseExpressionOrFail(source);
                        source->assert(')', &span);
                    }
                    else
                        source->location().throwError("Expected identifier or "
                            "parenthesized expression");
                }
                part = Symbol(atomFunctionCall,
                    Symbol(atomDot, part,
                        Symbol(atomIdentifier, String("toString")),
                        part.cache<ExpressionCache>()),
                    SymbolArray(), part.cache<ExpressionCache>());
                string += s.subString(startOffset, endOffset);
                startOffset = source->offset();
                if (part.valid()) {
                    expression = combine(expression,
                        Symbol(atomStringConstant, string,
                            new ExpressionCache(stringStartSpan +
                                stringEndSpan)),
                        new ExpressionCache(startSpan + stringEndSpan));
                    string = "";
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
            source->location().throwError("End of file in string");
        if (c == 10)
            break;
        *source = s;
    } while (true);
    int endOffset = source->offset();
    String terminator = source->subString(startOffset, endOffset);
    startOffset = s.offset();
    CharacterSource terminatorSource(terminator, "");
    int cc = terminatorSource.get();
    String string;
    do {
        *source = s;
        int c = s.get();
        if (c == -1)
            source->location().throwError("End of file in string");
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
            string += s.subString(startOffset, source->offset()) +
                codePoint(10);
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
