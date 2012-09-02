template<class T> class ExpressionTemplate : public ParseTreeObject
{
public:
    ExpressionTemplate() { }

    ExpressionTemplate(const Implementation* implementation)
      : ParseTreeObject(implementation) { }

    ExpressionTemplate(const String& string, const Span& span)
      : ParseTreeObject(new StringLiteralImplementation(string, span)) { }
    ExpressionTemplate(int n, const Span& span)
      : ParseTreeObject(new IntegerLiteralImplementation(n, span)) { }

    static Expression parse(CharacterSource* source)
    {
        return LogicalOrExpression::parse(source);
    }
    static Expression parseOrFail(CharacterSource* source)
    {
        Expression expression = parse(source);
        if (!expression.valid())
            throwError(source);
        return expression;
    }
    Expression operator+(const Expression& other) const
    {
        Expression e = *this;
        e += other;
        return e;
    }
    Expression operator-(const Expression& other) const
    {
        Expression e = *this;
        e -= other;      
        return e;
    }
    const Expression& operator+=(const Expression& other)
    {
        *this = FunctionCallExpression::binary(OperatorPlus(), Span(), *this,
            other);
        return *this;
    }
    const Expression& operator-=(const Expression& other)
    {
        *this = FunctionCallExpression::binary(OperatorMinus(), Span(), *this,
            other);
        return *this;
    }
    Expression operator!() const
    {
        return FunctionCallExpression::unary(OperatorNot(), Span(), *this);
    }
    Expression dot(const Identifier& identifier)
    {
        return new DotImplementation(*this, identifier);
    }

    Expression toString() const { return as<Expression>()->toString(); }

    class Implementation : public ParseTreeObject::Implementation
    {
    public:
        Implementation(const Span& span)
          : ParseTreeObject::Implementation(span) { }
        virtual Expression toString() const
        {
            return new FunctionCallExpression::Implementation(
                Expression(this).dot(Identifier("toString")),
                List<Expression>(), span());
        }
    };
protected:
    static Expression parseElement(CharacterSource* source)
    {
        Location start = source->location();
        Expression e = parseDoubleQuotedString(source);
        if (e.valid())
            return e;
        e = parseEmbeddedLiteral(source);
        if (e.valid())
            return e;
        e = parseInteger(source);
        if (e.valid())
            return e;
        Identifier i = Identifier::parse(source);
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
            e = parse(&s2);
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
                e = parse(&s2);
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
    static int parseHexadecimalCharacter(CharacterSource* source, Span* span)
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

    static Expression parseDoubleQuotedString(CharacterSource* source)
    {
        Span span;
        Span startSpan;
        if (!source->parse('"', &startSpan))
            return Expression();
        Span stringStartSpan = startSpan;
        Span stringEndSpan = startSpan;
        int startOffset = source->offset();
        int endOffset;
        String insert;
        int n;
        int nn;
        String string;
        Expression expression;
        Expression part;
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
                    return expression +
                        Expression(string, stringStartSpan + span);
                case '\\':
                    string += s.subString(startOffset, endOffset);
                    c = s.get(&stringEndSpan);
                    if (c < 0x20) {
                        if (c == 10)
                            source->location().throwError("End of line in "
                                "string");
                        if (c == -1)
                            source->location().throwError("End of file in "
                                "string");
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
                                    source->
                                        throwUnexpected("hexadecimal digit");
                                }
                                n = (n << 4) | nn;
                            }
                            nn = parseHexadecimalCharacter(source,
                                &stringEndSpan);
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
                    part = Identifier::parse(source);
                    if (!part.valid()) {
                        if (Space::parseCharacter(source, '(', &span)) {
                            part = Expression::parseOrFail(source);
                            source->assert(')', &span);
                        }
                        else
                            source->location().throwError("Expected "
                                "identifier or parenthesized expression");
                    }
                    string += s.subString(startOffset, endOffset);
                    startOffset = source->offset();
                    if (part.valid()) {
                        expression += Expression(string,
                            stringStartSpan + stringEndSpan);
                        string = "";
                        expression += part.toString();
                    }
                    break;
                default:
                    stringEndSpan = span;
            }
        } while (true);
    }

    static Expression parseEmbeddedLiteral(CharacterSource* source)
    {
        Span startSpan;
        Span endSpan;
        if (!source->parse('#', &startSpan))
            return Expression();
        if (!source->parse('#'))
            return Expression();
        if (!source->parse('#', &endSpan))
            return Expression();
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
        CharacterSource terminatorSource(terminator);
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
                return Expression(string, startSpan + endSpan);
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
                            string +=
                                s.subString(startOffset, source->offset());
                            *source = s2;
                            Space::parse(source);
                            return Expression(string, startSpan + endSpan);
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

    static Expression parseInteger(CharacterSource* source)
    {
        CharacterSource s = *source;
        int n = 0;
        Span span;
        int c = s.get(&span);
        if (c < '0' || c > '9')
            return Expression();
        do {
            n = n*10 + c - '0';
            *source = s;
            Span span2;
            c = s.get(&span2);
            if (c < '0' || c > '9') {
                Space::parse(source);
                return Expression(n, span);
            }
            span += span2;
        } while (true);
    }

    class TrueImplementation : public Implementation
    {
    public:
        TrueImplementation(const Span& span) : Implementation(span) { }
        Expression toString() const { return Expression("true", span()); }
    };
    class FalseImplementation : public Implementation
    {
    public:
        FalseImplementation(const Span& span) : Implementation(span) { }
        Expression toString() const { return Expression("false", span()); }
    };
    class NullImplementation : public Implementation
    {
    public:
        NullImplementation(const Span& span) : Implementation(span) { }
        Expression toString() const { return Expression("null", span()); }
    };
    class StringLiteralImplementation : public Implementation
    {
    public:
        StringLiteralImplementation(const String& string, const Span& span)
            : Implementation(span), _string(string) { }
        Expression toString() const { return Expression(this); }
    private:
        String _string;
        Span _span;
    };
    class IntegerLiteralImplementation : public Implementation
    {
    public:
        IntegerLiteralImplementation(int n, const Span& span)
            : Implementation(span), _n(n) { }
        Expression toString() const { return Expression(this); }
    private:
        int _n;
        Span _span;
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
    class DotImplementation : public Implementation
    {
    public:
        DotImplementation(const Expression& left, const Identifier& right)
          : Implementation(left.span() + right.span()), _left(left),
          _right(right) { }
    private:
        Expression _left;
        Identifier _right;
    };
};

template<class T> class IdentifierTemplate : public ExpressionTemplate<T>
{
public:
    IdentifierTemplate(const String& name)
      : ExpressionTemplate(new NameImplementation(name, Span())) { }

    static Identifier parse(CharacterSource* source)
    {
        CharacterSource s = *source;
        Location location = s.location();
        int start = s.offset();
        int c = s.get();
        if (c < 'a' || c > 'z')
            return Identifier();
        CharacterSource s2;
        do {
            s2 = s;
            c = s.get();
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') || c == '_')
                continue;
            break;
        } while (true);
        int end = s2.offset();
        Location endLocation = s2.location();
        Space::parse(&s2);
        String name = s2.subString(start, end);
        static String keywords[] = {
            "assembly",
            "break",
            "case",
            "catch",
            "continue",
            "default",
            "delete",
            "do",
            "done",
            "else",
            "elseIf",
            "elseUnless",
            "finally",
            "from",
            "for",
            "forever",
            "if",
            "in",
            "new",
            "nothing",
            "return",
            "switch",
            "this",
            "throw",
            "try",
            "unless",
            "until",
            "while"};
        for (int i = 0; i < sizeof(keywords)/sizeof(keywords[0]); ++i)
            if (name == keywords[i])
                return Identifier();
        Span span(location, endLocation);
        if (name != "operator") {
            *source = s2;
            return Identifier(new NameImplementation(name, span));
        }
        Span endSpan;
        Span span3;
        Operator o;
        if (Space::parseCharacter(&s2, '(', &endSpan)) {
            if (Space::parseCharacter(&s2, ')', &endSpan))
                o = OperatorFunctionCall();
            else
                s2.location().throwError("Expected )");
        }
        else if (Space::parseCharacter(&s2, '[', &endSpan)) {
            if (Space::parseCharacter(&s2, ']', &endSpan))
                o = OperatorIndex();
            else
                s2.location().throwError("Expected ]");
        }

        static const Operator ops[] = {
            OperatorEqualTo(), OperatorAssignment(), OperatorAddAssignment(),
            OperatorSubtractAssignment(), OperatorMultiplyAssignment(),
            OperatorDivideAssignment(), OperatorModuloAssignment(),
            OperatorShiftLeftAssignment(), OperatorShiftRightAssignment(),
            OperatorBitwiseAndAssignment(), OperatorBitwiseOrAssignment(),
            OperatorBitwiseXorAssignment(), OperatorPowerAssignment(),
            OperatorBitwiseOr(), OperatorTwiddle(), OperatorNot(),
            OperatorAmpersand(), OperatorNotEqualTo(),
            OperatorLessThanOrEqualTo(), OperatorShiftRight(), Operator()};

        for (const Operator* op = ops; op->valid(); ++op) {
            if (o.valid())
                break;
            o = op->parse(&s2, &endSpan);
        }
        if (!o.valid()) {
            CharacterSource s3 = s2;
            o = OperatorLessThan().parse(&s3, &endSpan);
            if (o.valid()) {
                // Only if we know it's not operator<<T>() can we try
                // operator<<()
                CharacterSource s4 = s3;
                TemplateArguments templateArguments =
                    TemplateArguments::parse(&s4);
                if (templateArguments.count() == 0) {
                    Operator o2 = OperatorShiftLeft().parse(&s2, &endSpan);
                    if (o2.valid())
                        o = o2;
                    else
                        s2 = s3;
                }
                else
                    s2 = s3;
            }
        }

        static const Operator ops2[] = {
            OperatorGreaterThanOrEqualTo(), OperatorGreaterThan(),
            OperatorPlus(), OperatorMinus(), OperatorDivide(), OperatorStar(),
            OperatorModulo(), OperatorPower(), Operator()};

        for (const Operator* op = ops2; op->valid(); ++op) {
            if (o.valid())
                break;
            o = op->parse(&s2, &endSpan);
        }
        if (!o.valid())
            s2.location().throwError("Expected an operator");
        *source = s2;
        return Identifier(o, span + endSpan);
    }

    IdentifierTemplate(const Operator& op, const Span& span)
      : Expression(new OperatorImplementation(op, span)) { }

    String name() const { return as<Identifier>()->name(); }

    class Implementation : public ExpressionTemplate<T>::Implementation
    {
    public:
        Implementation(const Span& span) : Expression::Implementation(span) { }
        virtual String name() const = 0;
    };
private:
    class NameImplementation : public Implementation
    {
    public:
        NameImplementation(const String& name, const Span& span)
          : Implementation(span), _name(name) { }
        String name() const { return _name; }
    private:
        String _name;
    };
    class OperatorImplementation : public Implementation
    {
    public:
        OperatorImplementation(const Operator& op, const Span& span)
          : Implementation(span), _op(op) { }
        String name() const { return "operator" + _op.name(); }
    private:
        Operator _op;
    };
    IdentifierTemplate() { }
    IdentifierTemplate(Implementation* implementation)
      : Expression(implementation) { }
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
        Expression expression = parse(source);
        if (!expression.valid())
            return expression;
        Span span;
        Operator o = OperatorPower().parse(source, &span);
        if (o.valid()) {
            Expression right = parsePower(source);
            if (!right.valid())
                throwError(source);
            expression = binary(o, span, expression, right);
        }
        return expression;
    }

    static Expression parseUnary(CharacterSource* source)
    {
        static const Operator ops[] = {
            OperatorTwiddle(), OperatorNot(), OperatorPlus(), OperatorMinus(),
            OperatorStar(), OperatorAmpersand(), Operator()};
        const Operator* op = ops;
        for (const Operator* op = ops; op->valid(); ++op) {
            Span span;
            Operator o = op->parse(source, &span);
            if (o.valid())
                return unary(o, span, parseUnary(source));
        }
        return parsePower(source);
    }

    static Expression parseHelper(CharacterSource* source, const Operator* ops,
        Expression (*parser)(CharacterSource* source))
    {
        Expression expression = parser(source);
        if (!expression.valid())
            return expression;
        const Operator* op = ops;
        do {
            Span span;
            Operator o = op->parse(source, &span);
            if (!o.valid()) {
                ++op;
                if (!op->valid())
                    break;
                return expression;
            }
            Expression right = parser(source);
            if (!right.valid())
                throwError(source);
            expression = binary(o, span, expression, right);
            op = ops;
        } while (true);
        // Not reachable
        return Expression();
    }

    static Expression parseMultiplicative(CharacterSource* source)
    {
        static const Operator ops[] = {
            OperatorStar(), OperatorDivide(), OperatorModulo(), Operator()};
        return parseHelper(source, ops, parseUnary);
    }

    static Expression parseAdditive(CharacterSource* source)
    {
        static const Operator ops[] = {
            OperatorPlus(), OperatorMinus(), Operator()};
        return parseHelper(source, ops, parseMultiplicative);
    }

    static Expression parseShift(CharacterSource* source)
    {
        static const Operator ops[] = {
            OperatorShiftLeft(), OperatorShiftRight(), Operator()};
        return parseHelper(source, ops, parseAdditive);
    }

    static Expression parseComparison(CharacterSource* source)
    {
        static const Operator ops[] = {
            OperatorLessThanOrEqualTo(), OperatorGreaterThanOrEqualTo(),
            OperatorLessThan(), OperatorGreaterThan(), Operator()};
        return parseHelper(source, ops, parseShift);
    }

    static Expression parseEquality(CharacterSource* source)
    {
        static const Operator ops[] = {
            OperatorEqualTo(), OperatorNotEqualTo(), Operator()};
        return parseHelper(source, ops, parseComparison);
    }

    static Expression parseBitwiseAnd(CharacterSource* source)
    {
        static const Operator ops[] = {
            OperatorAmpersand(), Operator()};
        return parseHelper(source, ops, parseEquality);
    }

    static Expression parseXor(CharacterSource* source)
    {
        static const Operator ops[] = {
            OperatorTwiddle(), Operator()};
        return parseHelper(source, ops, parseBitwiseAnd);
    }

    static Expression parseBitwiseOr(CharacterSource* source)
    {
        static const Operator ops[] = {
            OperatorBitwiseOr(), Operator()};
        return parseHelper(source, ops, parseXor);
    }

    static FunctionCallExpression unary(const Operator& op, const Span& span,
        const Expression& expression)
    {
        Identifier identifier(op, span);
        List<Expression> arguments;
        arguments.add(expression);
        return new Implementation(identifier, arguments,
            span + expression.span());
    }

    static FunctionCallExpression binary(const Operator& op, const Span& span,
        const Expression& left, const Expression& right)
    {
        Identifier identifier(op, span);
        List<Expression> arguments;
        arguments.add(left);
        arguments.add(right);
        return new Implementation(identifier, arguments,
            left.span() + right.span());
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
private:
    FunctionCallExpression(const Implementation* implementation)
      : Expression(implementation) { }

    template<class T> friend class ExpressionTemplate;
};

template<class T> class BinaryExpression : public Expression
{
protected:
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
                e = new Implementation(e, span, e2);
                continue;
            }
            return e;
        } while (true);
    }
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
            if (Space::parseOperator(source, "||", &span)) {
                Expression e2 = LogicalAndExpression::parse(source);
                if (!e2.valid())
                    throwError(source);
                e = new Implementation(e, span, e2);
                continue;
            }
            return e;
        } while (true);
    }
};
