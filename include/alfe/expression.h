#include "alfe/code.h"

#ifndef INCLUDED_EXPRESSION_H
#define INCLUDED_EXPRESSION_H

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

// Expression::parse()
// ConditionalExpression::parse()
// LogicalOrExpression::parse()
// LogicalAndExpression::parse()
// FunctionCallExpression::parseBitwiseOr() - should be CallExpression::parseBitwiseOr()
// CallExpression::parseXor()
// CallExpression::parseBitwiseAnd()
// CallExpression::parseEquality()
// CallExpression::parseComparison()
// CallExpression::parseShift()
// CallExpression::parseAdditive()
// CallExpression::parseMultiplicative()
// CallExpression::parseUnary()
// CallExpression::parsePower()
// Expression::parseDot()
// FunctionCallExpression::parse() - should be CallExpression::parse()
// Expression::parseElement() and CallExpression::parseRemainder()
//   Expression::parseDoubleQuotedString()
//   Expression::parseEmbeddedLiteral()
//   Expression::parseNumber()
//   Identifier::parse()
//   FunctionCallExpression::parseConstructorCall()
//   VariableDefinition::parse()

template<class T> class ExpressionT : public ParseTreeObject
{
public:
    ExpressionT() { }
    ExpressionT(Handle other) : ParseTreeObject(to<Body>(other)) { }

    class Body : public ParseTreeObject::Body
    {
    public:
        Body(const Span& span) : ParseTreeObject::Body(span) { }
        virtual Expression stringify()
        {
            return create<
                typename FunctionCallExpressionT<T>::FunctionCallBody>(
                Expression(expression()).dot(IdentifierT<T>("toString")),
                List<Expression>(), span());
        }
        virtual String toString() = 0;
        virtual Value evaluate(Structure* context) = 0;
        Expression expression() { return handle<Expression>(); }
        virtual TypeT<T> type() = 0;
        virtual bool mightHaveSideEffect() = 0;
    };

    ExpressionT(const String& string, const Span& span)
      : ExpressionT(StringLiteralExpression(string, span)) { }

    static Expression parse(CharacterSource* source)
    {
        return ConditionalExpressionT<T>::parse(source);
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
        *this = FunctionCallExpressionT<T>::binary(OperatorPlus(),
            Span(), *this, other);
        return *this;
    }
    const Expression& operator-=(const Expression& other)
    {
        *this = FunctionCallExpressionT<T>::binary(OperatorMinus(),
            Span(), *this, other);
        return *this;
    }
    Expression operator!() const
    {
        return FunctionCallExpressionT<T>::unary(OperatorNot(),
            Span(), *this);
    }
    Expression dot(const Identifier& identifier)
    {
        return DotExpression(*this, identifier);
    }

    // toString() creates a compile-time String which is a pretty-printed
    // representation of the Expression.
    // stringify() create a run-time Expression which converts the value of
    // the expression into a String.
    Expression stringify() { return body()->stringify(); }
    String toString() { return body()->toString(); }

    static Expression parseDot(CharacterSource* source)
    {
        Expression e = FunctionCallExpressionT<T>::parse(source);
        if (!e.valid())
            return e;
        do {
            Span span;
            if (Space::parseCharacter(source, '.', &span)) {
                IdentifierT<T> i = IdentifierT<T>::parse(source);
                if (!i.valid())
                    source->location().throwError("Identifier expected");
                e = e.dot(i);
                continue;
            }
            return e;
        } while (true);
    }

    ValueT<T> evaluate(Structure* context)
    {
        return body()->evaluate(context);
    }
    TypeT<T> type() { return body()->type(); }
    bool mightHaveSideEffect() { return body()->mightHaveSideEffect(); }

    static Expression from(Rational r) { return NumericLiteral(r); }
    static Expression from(String s)
    {
        return StringLiteralExpression(s);
    }
    static Expression from(bool b)
    {
        if (b)
            return create<TrueBody>();
       return create<FalseBody>();
    }
protected:
    const Body* body() const { return as<Body>(); }
    Body* body() { return as<Body>(); }

    static Expression parseElement(CharacterSource* source)
    {
        Location start = source->location();
        Expression e = parseDoubleQuotedString(source);
        if (e.valid())
            return e;
        e = parseEmbeddedLiteral(source);
        if (e.valid())
            return e;
        e = parseNumber(source);
        if (e.valid())
            return e;
        e = IdentifierT<T>::parse(source);
        if (e.valid())
            return e;
        e = FunctionCallExpressionT<T>::parseConstructorCall(source);
        if (e.valid())
            return e;
        e = VariableDefinitionT<T>::parse(source);
        if (e.valid())
            return e;
        Span span;
        if (Space::parseKeyword(source, "true", &span))
            return create<TrueBody>(span);
        if (Space::parseKeyword(source, "false", &span))
            return create<FalseBody>(span);
        CharacterSource s2 = *source;
        if (Space::parseCharacter(&s2, '(', &span)) {
            List<Expression> expressions;
            bool foundExpression = false;
            do {
                Span span2;
                if (Space::parseCharacter(&s2, ')', &span2)) {
                    *source = s2;
                    if (expressions.count() == 0)
                        return create<UnitBody>(span + span2);
                    if (expressions.count() == 1)
                        return *expressions.begin();
                    return create<TupleBody>(expressions, span + span2);
                }
                if (foundExpression &&
                    !Space::parseCharacter(&s2, ',', &span2)) {
                    return Expression();
                }
                e = parse(&s2);
                if (!e.valid())
                    return e;
                expressions.add(e);
                foundExpression = true;
            } while (true);
        }
        if (Space::parseCharacter(&s2, '{', &span)) {
            List<Expression> expressions;
            bool foundExpression = false;
            do {
                Span span2;
                if (Space::parseCharacter(&s2, '}', &span2)) {
                    *source = s2;
                    return create<ArrayLiteralBody>(expressions, span + span2);
                }
                if (foundExpression &&
                    !Space::parseCharacter(&s2, ',', &span2)) {
                    return Expression();
                }
                e = parse(&s2);
                if (!e.valid())
                    return e;
                expressions.add(e);
                foundExpression = true;
            } while (true);
        }
        return e;
    }

    static void throwError(CharacterSource* source)
    {
        source->location().throwError("Expected expression");
    }
private:
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
                    {
                        string += s.subString(startOffset, endOffset);
                        Space::parse(source);
                        Expression r(string, stringStartSpan + span);
                        if (expression.valid())
                            return expression + r;
                        return r;
                    }
                case '\\':
                    string += s.subString(startOffset, endOffset);
                    c = s.get(&stringEndSpan);
                    if (c < 0x20) {
                        if (c == 10)
                            source->location().throwError(
                                "End of line in string");
                        if (c == -1)
                            source->location().throwError(
                                "End of file in string");
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
                        case '\\':
                            insert = "\\";
                            break;
                        case 'U':
                            source->assert('+', &stringEndSpan);
                            n = 0;
                            for (int i = 0; i < 4; ++i) {
                                nn = parseHexadecimalCharacter(source,
                                    &stringEndSpan);
                                if (nn == -1) {
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
                    part = IdentifierT<T>::parse(source);
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
                        expression += part.stringify();
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
        if (!source->parseString("###", &startSpan))
            return Expression();
        int startOffset = source->offset();
        Location location = source->location();
        CharacterSource s = *source;

        bool eof;
        String terminator = s.delimitString(String(String::CodePoint(10)), &eof);
        if (eof)
            source->location().throwError("End of file in string");

        String string = s.delimitString(terminator + "###", &eof);
        if (eof)
            s.location().throwError("End of file in string");

        return Expression(string, Span(startSpan.start(), s.location()));
    }

    static Expression parseNumber(CharacterSource* source)
    {
        Rational n;
        Span span;
        if (Space::parseNumber(source, &n, &span))
            return NumericLiteralT<T>(n, span);
        return Expression();
    }

    class UnitBody : public Body
    {
    public:
        UnitBody(const Span& span) : Body(span) { }
        Expression stringify() const { return Expression("()", this->span()); }
        String toString() { return "()"; }
        ValueT<T> evaluate(Structure* context) { return ValueT<T>(); }
        TypeT<T> type() { return VoidTypeT<T>(); }
        bool mightHaveSideEffect() { return false; }
    };
    class TupleBody : public Body
    {
    public:
        TupleBody(List<Expression> expressions, const Span& span)
          : Body(span), _expressions(expressions) { }
        Expression stringify()
        {
            Expression r("(", Span());
            bool first = true;
            for (auto e : _expressions) {
                if (!first)
                    r += Expression(", ", Span());
                first = false;
                r += e.stringify();
            }
            r += Expression(")", Span());
            r.setSpan(this->span());
            return r;
        }
        String toString()
        {
            String r = "(";
            bool first = true;
            for (auto e : _expressions) {
                if (!first)
                    r += ", ";
                first = false;
                r += e.toString();
            }
            return r + ")";
        }
        ValueT<T> evaluate(Structure* context)
        {
            List<ValueT<T>> v;
            for (auto e : _expressions)
                v.add(e.evaluate(context));
            return ValueT<T>(type(), v);
        }
        TypeT<T> type()
        {
            TupleTycoT<T> t;
            for (auto e : _expressions)
                t = t.instantiate(e.type());
            return t;
        }
        bool mightHaveSideEffect()
        {
            for (auto e : _expressions) {
                if (e.mightHaveSideEffect())
                    return true;
            }
            return false;
        }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(parseTreeObject());
            if (r == CodeWalker::Result::recurse) {
                for (auto e : _expressions) {
                    if (e.walk(walker) == CodeWalker::Result::abort)
                        return CodeWalker::Result::abort;
                }
            }
            return r;
        }
    private:
        List<Expression> _expressions;
    };
    class BooleanBody : public Body
    {
    public:
        BooleanBody(const Span& span) : Body(span) { }
        Expression stringify()
        {
            return Expression(this->toString(), this->span());
        }
        TypeT<T> type() { return BooleanTypeT<T>(); }
        bool mightHaveSideEffect() { return false; }
    };
    class TrueBody : public BooleanBody
    {
    public:
        TrueBody(const Span& span) : BooleanBody(span) { }
        String toString() { return "true"; }
        ValueT<T> evaluate(Structure* context) { return true; }
    };
    class FalseBody : public BooleanBody
    {
    public:
        FalseBody(const Span& span) : BooleanBody(span) { }
        String toString() { return "false"; }
        ValueT<T> evaluate(Structure* context) { return false; }
    };
    class ArrayLiteralBody : public Body
    {
    public:
        ArrayLiteralBody(const List<Expression>& expressions, const Span& span)
          : Body(span), _expressions(expressions) { }
        ValueT<T> evaluate(Structure* context)
        {
            HashTable<Identifier, Value> values;
            List<typename StructuredTypeT<T>::Member> members;
            int i = 0;
            for (auto e : _expressions) {
                ValueT<T> v = e.evaluate(context);
                Type t = v.type();
                String n = decimal(i);
                values.add(n, v);
                members.add(typename StructuredTypeT<T>::Member("", t));
                ++i;
            }
            return Value(StructuredType("", members), values, this->span());
        }
        TypeT<T> type()
        {
            TypeT<T> type = VoidTypeT<T>();
            int i = 0;
            for (auto e : _expressions) {
                if (i == 0)
                    type = e.type();
                else {
                    if (type != e.type()) {
                        e.span().throwError("Array literal has inconsistent "
                            "types. Element 0 has type " + type.toString() +
                            ", element" + decimal(i) + " has type " +
                            e.type().toString() + ".");
                    }
                }
                ++i;
            }
            return ArrayType(type);
        }
        Expression stringify()
        {
            Expression r("{", Span());
            bool first = true;
            for (auto e : _expressions) {
                if (!first)
                    r += Expression(", ", Span());
                first = false;
                r += e.stringify();
            }
            r += Expression("}", Span());
            r.setSpan(this->span());
            return r;
        }
        String toString()
        {
            String r = "{";
            bool first = true;
            for (auto e : _expressions) {
                if (!first)
                    r += ", ";
                first = false;
                r += e.toString();
            }
            return r + "}";
        }
        bool mightHaveSideEffect()
        {
            for (auto e : _expressions) {
                if (e.mightHaveSideEffect())
                    return true;
            }
            return false;
        }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(parseTreeObject());
            if (r == CodeWalker::Result::recurse) {
                for (auto e : _expressions) {
                    if (e.walk(walker) == CodeWalker::Result::abort)
                        return CodeWalker::Result::abort;
                }
            }
            return r;
        }
    private:
        List<Expression> _expressions;
    };
};

template<class T> class DotExpressionT : public Expression
{
public:
    DotExpressionT(ExpressionT<T> left, IdentifierT<T> right)
      : Expression(create<Body>(left, right)) { }
    DotExpressionT(const Handle& other) : Expression(to<Body>(other)) { }
    ExpressionT<T> left() { return body()->_left; }
    IdentifierT<T> right() { return body()->_right; }

private:
    class Body : public Expression::Body
    {
    public:
        Body(const Expression& left, const IdentifierT<T>& right)
          : Body(left.span() + right.span()), _left(left), _right(right) { }
        TypeT<T> type()
        {
            TypeT<T> lType = _left.type();
            StructuredType s = lType;
            if (!s.valid())
                _left.span().throwError("Expression has no members");
            return s.member(_right);
        }
        String toString()
        {
            return _left.toString() + "." + _right.toString();
        }
        bool mightHaveSideEffect()
        {
            return _left.mightHaveSideEffect();
        }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(parseTreeObject());
            if (r == CodeWalker::Result::recurse) {
                if (_left.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_right.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        ExpressionT<T> _left;
        IdentifierT<T> _right;
    };
    Body* body() { return as<Body>(); }
};

template<class T> class NumericLiteralT : public Expression
{
public:
    NumericLiteralT(Rational n, Span span = Span())
      : Expression(create<Body>(n, span)) { }
    NumericLiteralT(const Expression& e) : Expression(e) { }
    Rational value() const { return as<Body>()->value(); }

    class Body : public Expression::Body
    {
    public:
        Body(Rational n, const Span& span) : Expression::Body(span), _n(n) { }
        String toString()
        {
            return decimal(_n.numerator) + "/" + decimal(_n.denominator);
        }
        Rational value() { return _n; }
        TypeT<T> type()
        {
            if (_n.denominator == 1)
                return IntegerTypeT<T>();
            return RationalTypeT<T>();
        }
        bool mightHaveSideEffect() { return false; }
    private:
        Rational _n;
    };
};

template<class T> class FuncoT;
typedef FuncoT<void> Funco;

template<class T> class CallExpressionT : public Expression
{
public:
    CallExpressionT(Handle other) : Expression(other) { }

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

    static Expression parseConstructorCall(CharacterSource* source)
    {
        TycoSpecifier t = TycoSpecifier::parse(source);
        if (!t.valid())
            return Expression();
        Span span;
        if (!Space::parseCharacter(source, '(', &span))
            return Expression();
        List<Expression> arguments = parseList(source);
        Space::assertCharacter(source, ')', &span);
        return parseRemainder(
            create<ConstructorCallBody>(t, arguments, t.span() + span),
            source);
    }

    static Expression parse(CharacterSource* source)
    {
        Expression e = parseElement(source);
        if (!e.valid())
            return e;
        return parseRemainder(e, source);
    }

    static Expression parsePower(CharacterSource* source)
    {
        Expression expression = Expression::parseDot(source);
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
        static Operator ops[] = {
            OperatorTwiddle(), OperatorNot(), OperatorPlus(), OperatorMinus(),
            OperatorStar(), OperatorAmpersand(), Operator()};
        for (Operator* op = ops; op->valid(); ++op) {
            Span span;
            Operator o = op->parse(source, &span);
            if (o.valid()) {
                Expression inner = parseUnary(source);
                if (!inner.valid())
                    return inner;
                return unary(o, span, inner);
            }
        }
        return parsePower(source);
    }

    static Expression parseHelper(CharacterSource* source, Operator* ops,
        Expression (*parser)(CharacterSource* source))
    {
        Expression expression = parser(source);
        if (!expression.valid())
            return expression;
        Operator* op = ops;
        do {
            Span span;
            Operator o;
            do {
                o = op->parse(source, &span);
                if (o.valid())
                    break;
                ++op;
                if (!op->valid())
                    return expression;
            } while (true);
            Expression right = parser(source);
            if (!right.valid())
                throwError(source);
            expression = binary(o, span, expression, right);
            op = ops;
        } while (true);
        assert(false);
        return Expression();
    }

    static Expression parseMultiplicative(CharacterSource* source)
    {
        static Operator ops[] = {
            OperatorStar(), OperatorDivide(), OperatorModulo(), Operator()};
        return parseHelper(source, ops, parseUnary);
    }

    static Expression parseAdditive(CharacterSource* source)
    {
        static Operator ops[] = {
            OperatorPlus(), OperatorMinus(), Operator()};
        return parseHelper(source, ops, parseMultiplicative);
    }

    static Expression parseShift(CharacterSource* source)
    {
        static Operator ops[] = {
            OperatorShiftLeft(), OperatorShiftRight(), Operator()};
        return parseHelper(source, ops, parseAdditive);
    }

    static Expression parseComparison(CharacterSource* source)
    {
        static Operator ops[] = {
            OperatorLessThanOrEqualTo(), OperatorGreaterThanOrEqualTo(),
            OperatorLessThan(), OperatorGreaterThan(), Operator()};
        return parseHelper(source, ops, parseShift);
    }

    static Expression parseEquality(CharacterSource* source)
    {
        static Operator ops[] = {
            OperatorEqualTo(), OperatorNotEqualTo(), Operator()};
        return parseHelper(source, ops, parseComparison);
    }

    static Expression parseBitwiseAnd(CharacterSource* source)
    {
        static Operator ops[] = { OperatorAmpersand(), Operator()};
        return parseHelper(source, ops, parseEquality);
    }

    static Expression parseXor(CharacterSource* source)
    {
        static Operator ops[] = { OperatorTwiddle(), Operator()};
        return parseHelper(source, ops, parseBitwiseAnd);
    }

    static Expression parseBitwiseOr(CharacterSource* source)
    {
        static Operator ops[] = { OperatorBitwiseOr(), Operator()};
        return parseHelper(source, ops, parseXor);
    }

    static FunctionCallExpressionT<T> unary(const Operator& op,
        const Span& span, const Expression& expression)
    {
        Identifier identifier(op, span);
        List<Expression> arguments;
        arguments.add(expression);
        return create<FunctionCallBody>(identifier, arguments,
            span + expression.span());
    }

    static FunctionCallExpressionT<T> binary(const Operator& op,
        const Span& span, const Expression& left, const Expression& right)
    {
        Identifier identifier(op, span);
        List<Expression> arguments;
        arguments.add(left);
        arguments.add(right);
        return create<FunctionCallBody>(identifier, arguments,
            left.span() + right.span());
    }

    class Body : public Expression::Body
    {
    public:
        Body(const Span& span, const List<Expression>& arguments)
          : Expression::Body(span), _arguments(arguments) { }
        String toString()
        {
            String r = "(";
            bool first = true;
            for (auto e : _arguments) {
                if (!first)
                    r += ", ";
                first = false;
                r += e.toString();
            }
            return r + ")";
        }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            for (auto a : _arguments) {
                if (a.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return CodeWalker::Result::advance;
        }
        List<Expression> arguments() { return _arguments; }
    private:
        List<Expression> _arguments;
    };
    List<Expression> arguments() { return body()->arguments(); }
private:
    static Expression parseRemainder(Expression e, CharacterSource* source)
    {
        do {
            Span span;
            if (Space::parseCharacter(source, '(', &span)) {
                List<Expression> arguments = parseList(source);
                Space::assertCharacter(source, ')', &span);
                e = FunctionCallExpression(
                    create<FunctionCallBody>(e, arguments, e.span() + span));
            }
            else
                if (Space::parseCharacter(source, '[', &span)) {
                    Span span2;
                    Expression index = Expression::parse(source);
                    Space::assertCharacter(source, ']', &span2);
                    e = binary(OperatorIndex(), span + span2, e, index);
                }
                else
                    break;

        } while (true);
        return e;
    }

    template<class U> friend class ExpressionT;
};

template<class T> class FunctionCallExpressionT : public CallExpression
{
public:
    FunctionCallExpressionT(const Expression& function,
        const List<Expression>& arguments, const Span& span)
      : CallExpression(create<Body>(function, arguents, span)) { }
    FunctionCallExpressionT(const Handle& handle)
      : CallExpression(to<Body>(handle)) { }
    Expression function() { return body()->_function; }
    void setResolvedFunco(Funco f) { body()->_resolvedFunco = f; }
    FuncoT<T> resolvedFunco() { return body()->_resolvedFunco; }
protected:
    class Body : public CallExpressionT<T>::Body
    {
    public:
        Body(const Expression& function, const List<Expression>& arguments,
            const Span& span)
          : Expression::Body(span, arguments), _function(function) { }
        ValueT<T> evaluate(Structure* context)
        {
            ValueT<T> l;
            if (!_resolvedFunco.valid())
                l = _function.evaluate(context).rValue();
            List<Value> arguments;
            for (auto p : this->_arguments)
                arguments.add(p.evaluate(context).rValue());
            if (_resolvedFunco.valid()) {
                // Convert arguments. TODO: share this logic with
                // OverloadedFunctionSet::Body::evaluate()?

                List<Value> convertedArguments;
                if (Function(_resolvedFunco).valid()) {
                    List<Tyco> parameterTycos =
                        _resolvedFunco.parameterTycos();
                    auto ii = parameterTycos.begin();
                    for (auto a : arguments) {
                        Type type = *ii;
                        if (!type.valid()) {
                            a.span().throwError("Function parameter's type "
                                "constructor is not a type.");
                        }
                        convertedArguments.add(a.convertTo(type));
                        ++ii;
                    }
                }
                else {
                    // Funcos that are not functions don't get their arguments
                    // converted.
                    convertedArguments = arguments;
                }

                return _resolvedFunco.evaluate(convertedArguments,
                    this->span());
            }
            TypeT<T> lType = l.type();
            if (lType == FuncoTypeT<T>()) {
                return l.template value<OverloadedFunctionSet>().evaluate(
                    arguments, this->span());
            }
            // What we have on the left isn't a function, try to call its
            // operator() method instead.
            IdentifierT<T> i = IdentifierT<T>(OperatorFunctionCall());
            if (!lType.member(i).valid())
                this->span().throwError("Expression is not a function.");
            if (!LValueTypeT<T>(lType).valid()) {
                auto m = l.template value<HashTable<Identifier, Value>>();
                l = m[i];
                l = Value(l.type(), l.value(), this->span());
            }
            else {
                StructureT<T>* p = l.template
                    value<LValue>().rValue().template value<Structure*>();
                l = Value(LValueTypeT<T>::wrap(p->getValue(i).type()),
                    LValue(p, i), this->span());
            }
            List<Value> convertedArguments;
            Function f = l.template value<Function>();
            List<Tyco> parameterTycos = f.parameterTycos();
            auto ii = parameterTycos.begin();
            for (auto a : arguments) {
                Type type = *ii;
                if (!type.valid()) {
                    a.span().throwError("Function parameter's type "
                        "constructor is not a type.");
                }
                convertedArguments.add(a.convertTo(type));
                ++ii;
            }
            return f.evaluate(convertedArguments, this->span());
        }
        TypeT<T> type() { return _resolvedFunco.type().returnType(); }
        String toString()
        {
            return _function.toString() + Body::toString();
        }
        // TODO: check if it's a pure function
        bool mightHaveSideEffect() { return true; }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(parseTreeObject());
            if (r == CodeWalker::Result::recurse) {
                if (_function.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (Body::walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_resolvedFunco.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        Expression _function;
        Funco _resolvedFunco;
    };
};

template<class T> class ConstructorCallExpressionT : public CallExpression
{
public:
    ConstructorCallExpressionT() { }
    TycoSpecifierT<T> tycoSpecifier() { return body()->_tycoSpecifier; }
protected:
    class Body : public CallExpression::Body
    {
    public:
        Body(const TycoSpecifier& tycoSpecifier,
            const List<Expression>& arguments, const Span& span)
          : CallExpression::Body(span, arguments),
            _tycoSpecifier(tycoSpecifier) { }
        ValueT<T> evaluate(Structure* context)
        {
            List<ValueT<T>> arguments;
            for (auto p : this->_arguments)
                arguments.add(p.evaluate(context));

            StructuredTypeT<T> type = _type;
            if (!type.valid()) {
                _tycoSpecifier.span().throwError(
                    "Only structure types can be constructed at the moment.");
            }
            auto members = type.members();
            List<Any> values;
            Span span = _tycoSpecifier.span();
            auto ai = arguments.begin();
            for (int i = 0; i < members.count(); ++i) {
                ValueT<T> value = ai->convertTo(members[i].type());
                values.add(value.value());
                span += value.span();
                ++ai;
            }
            return type.constructValue(Value(type, values, span));
        }
        TypeT<T> type() { return _tycoSpecifier.tyco(); }
        String toString() { return _type.toString() + Body::toString(); }
        bool mightHaveSideEffect() { return true; }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(parseTreeObject());
            if (r == CodeWalker::Result::recurse) {
                if (_tycoSpecifier.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (Body::walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_type.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        TycoSpecifier _tycoSpecifier;
    };

};

template<class T> class VariableDefinitionT : public Expression
{
public:
    VariableDefinitionT(const Handle& other) : Expression(to<Body>(other)) { }
    VariableDefinitionT(const Identifier& identifier)
      : Expression(create<Body>(LabelType(), identifier, Expression(), Span()))
    { }
    static VariableDefinitionT parse(CharacterSource* source)
    {
        CharacterSource s = *source;
        TycoSpecifier t = TycoSpecifier::parse(&s);
        if (!t.valid())
            return VariableDefinitionT();
        Identifier i = Identifier::parse(&s);
        if (!i.valid())
            return VariableDefinitionT();
        *source = s;
        Expression e;
        Span span = t.span() + i.span();
        if (Space::parseCharacter(&s, '=', &span)) {
            e = Expression::parse(&s);
            if (e.valid()) {
                *source = s;
                span += e.span();
            }
        }
        return create<Body>(t, i, e, span);
    }
    VariableDefinitionT() { }
    VariableDefinitionT(TycoSpecifier tycoSpecifier, Identifier identifier)
      : Expression(create<Body>(tycoSpecifier, identifier, Expression(),
          Span()))
    { }
    VariableDefinitionT(TypeT<T> type, Identifier identifier)
      : Expression(create<Body>(type, identifier, Expression(), Span()))
    { }
    VariableDefinitionT(TypeT<T> type, Identifier identifier,
        Expression expression)
      : Expression(create<Body>(type, identifier, expression, Span()))
    { }
    TycoSpecifierT<T> tycoSpecifier() { return body()->tycoSpecifier(); }
    IdentifierT<T> identifier() const { return body()->identifier(); }
    Expression initializer() { return body()->initializer(); }
private:

    class Body : public Expression::Body
    {
    public:
        Body(TycoSpecifier tycoSpecifier, Identifier identifier,
            Expression initializer, Span span)
          : Expression::Body(span), _tycoSpecifier(tycoSpecifier),
            _identifier(identifier), _initializer(initializer)
        { }
        Body(TypeT<T> type, Identifier identifier, Expression initializer,
            Span span)
          : Expression::Body(span), _tycoSpecifier(type),
            _identifier(identifier), _initializer(initializer)
        { }
        String toString()
        {
            String r = _tycoSpecifier.toString() + " " +
                _identifier.toString();
            if (_initializer.valid())
                r += " = " + _initializer.toString();
            return r;
        }
        ValueT<T> evaluate(Structure* context)
        {
            ValueT<T> e = _initializer.evaluate(context);
            return e.convertTo(type());
        }
        TycoSpecifierT<T> tycoSpecifier() { return _tycoSpecifier; }
        TypeT<T> type() { return _tycoSpecifier.tyco(); }
        bool mightHaveSideEffect() { return true; }
        IdentifierT<T> identifier() { return _identifier; }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(parseTreeObject());
            if (r == CodeWalker::Result::recurse) {
                if (_tycoSpecifier.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_identifier.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_initializer.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
   private:
        TycoSpecifier _tycoSpecifier;
        Identifier _identifier;
        Expression _initializer;
    };

    Body* body() { return as<Body>(); }
};

template<class T> class BinaryExpressionT : public Expression
{
public:
    BinaryExpressionT(ParseTreeObject o) : Expression(to<Body>(o)) { }
    Expression left() { return body()->_left; }
    Expression right() { return body()->_right; }
protected:
    class Body : public Expression::Body
    {
    public:
        Body(const Expression& left, const Span& operatorSpan,
            const Expression& right)
          : Expression::Body(left.span() + right.span()),
            _left(left), _right(right), _operatorSpan(operatorSpan) { }
        Expression left() { return _left; }
        Expression right() { return _right; }
        TypeT<T> type() { return BooleanTypeT<T>(); }
        bool mightHaveSideEffect()
        {
            return _left.mightHaveSideEffect() || _right.mightHaveSideEffect();
        }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(parseTreeObject());
            if (r == CodeWalker::Result::recurse) {
                if (_left.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_right.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        Expression _left;
        Expression _right;
        Span _operatorSpan;

        template<class U> friend class BinaryExpressionT;
    };
    Body* body() { return as<Body>(); }
};

typedef BinaryExpressionT<void> BinaryExpression;

template<class T> class LogicalAndExpressionT;
typedef LogicalAndExpressionT<void> LogicalAndExpression;

template<class T> class LogicalAndExpressionT : public BinaryExpression
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
                e = create<Body>(e, span, e2);
                continue;
            }
            return e;
        } while (true);
    }
private:
    class Body : public BinaryExpression::Body
    {
    public:
        Body(const Expression& left, const Span& operatorSpan,
            const Expression& right)
          : BinaryExpression::Body(left, operatorSpan, right) { }
        ValueT<T> evaluate(Structure* context)
        {
            ValueT<T> v = left().evaluate(context);
            if (v.type() != BooleanTypeT<T>()) {
                left().span().throwError("Logical operator requires operand "
                    "of type Boolean.");
            }
            if (!v.template value<bool>())
                return false;
            v = right().evaluate(context);
            if (v.type() != BooleanTypeT<T>()) {
                right().span().throwError("Logical operator requires operand "
                    "of type Boolean.");
            }
            return v.template value<bool>();
        }
        String toString()
        {
            return "(" + left().toString() + " && " + right().toString() + ")";
        }
        Expression stringify()
        {
            return ConditionalExpressionT<T>(left(), right().stringify(),
                Expression("false", Span()));
        }
    };
};

template<class T> class LogicalOrExpressionT : public BinaryExpression
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
                e = create<Body>(e, span, e2);
                continue;
            }
            return e;
        } while (true);
    }
private:
    class Body : public BinaryExpression::Body
    {
    public:
        Body(const Expression& left, const Span& operatorSpan,
            const Expression& right)
          : BinaryExpression::Body(left, operatorSpan, right) { }
        ValueT<T> evaluate(Structure* context)
        {
            ValueT<T> v = left().evaluate(context);
            if (v.type() != BooleanTypeT<T>()) {
                left().span().throwError("Logical operator requires operand "
                    "of type Boolean.");
            }
            if (!v.template value<bool>())
                return false;
            v = right().evaluate(context);
            if (v.type() != BooleanTypeT<T>()) {
                right().span().throwError("Logical operator requires operand "
                    "of type Boolean.");
            }
            return v.template value<bool>();
        }
        String toString()
        {
            return "(" + left().toString() + " && " + right().toString() + ")";
        }
        Expression stringify()
        {
            return ConditionalExpressionT<T>(left(),
                Expression("true", Span()), right().stringify());
        }
    };
};

template<class T> class ConditionalExpressionT : public Expression
{
public:
    static Expression parse(CharacterSource* source)
    {
        Expression e = LogicalOrExpression::parse(source);
        if (!e.valid())
            return e;
        Span span1;
        if (Space::parseOperator(source, "?", &span1)) {
            Expression trueExpression = parse(source);
            if (!trueExpression.valid())
                source->throwUnexpected("expression");
            Span span2;
            if (!Space::parseOperator(source, ":", &span2))
                source->throwUnexpected(":");
            Expression falseExpression = parse(source);
            if (!falseExpression.valid())
                source->throwUnexpected("expression");
            e = create<Body>(e, span1, trueExpression, span2, falseExpression);
        }
        return e;
    }
    ConditionalExpressionT(Expression condition, Expression trueExpression,
        Expression falseExpression)
      : Expression(create<Body>(condition, Span(), trueExpression, Span(),
          falseExpression))
    { }
    Expression condition() { return body()->_condition; }
    Expression trueExpression() { return body()->_trueExpression; }
    Expression falseExpression() { return body()->_falseExpression; }
private:
    class Body : public Expression::Body
    {
    public:
        Body(const Expression& condition, const Span& s1,
            const Expression& trueExpression, const Span& s2,
            const Expression& falseExpression)
          : Expression::Body(condition.span() + falseExpression.span()),
            _condition(condition), _s1(s1), _trueExpression(trueExpression),
            _s2(s2), _falseExpression(falseExpression)
        { }
        ValueT<T> evaluate(Structure* context)
        {
            ValueT<T> v = _condition.evaluate(context).rValue();
            if (v.type() != BooleanTypeT<T>()) {
                _condition.span().throwError("Conditional operator requires "
                    "operand of type Boolean.");
            }
            if (v.template value<bool>())
                return _trueExpression.evaluate(context);
            return _falseExpression.evaluate(context);
        }
        TypeT<T> type()
        {
            TypeT<T> tt = _trueExpression.type();
            TypeT<T> ft = _falseExpression.type();
            if (tt != ft) {
                span().throwError("True expression has type " + tt.toString() +
                    " and false expression has type " + ft.toString() + ".");
            }
            return _trueExpression.type();
        }
        bool mightHaveSideEffect()
        {
            return _condition.mightHaveSideEffect() ||
                _trueExpression.mightHaveSideEffect() ||
                _falseExpression.mightHaveSideEffect();
        }
        String toString()
        {
            return "(" + _condition.toString() + " ? " +
                _trueExpression.toString() + " : " +
                _falseExpression.toString() + ")";
        }
        CodeWalker::Result walk(CodeWalker* walker)
        {
            auto r = walker->visit(parseTreeObject());
            if (r == CodeWalker::Result::recurse) {
                if (_condition.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_trueExpression.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
                if (_falseExpression.walk(walker) == CodeWalker::Result::abort)
                    return CodeWalker::Result::abort;
            }
            return r;
        }
    private:
        Expression _condition;
        Span _s1;
        Expression _trueExpression;
        Span _s2;
        Expression _falseExpression;

        template<class U> friend class ConditionalExpressionT;
    };

    Body* bodu() { return to<Body>(); }
};

template <class T> class StringLiteralExpressionT : public Expression
{
public:
    StringLiteralExpressionT(const String& string, const Span& span)
      : Expression(create<Body>(string, span)) { }
    StringLiteralExpressionT(const Expression& other)
      : Expression(to<Body>(other)) { }
    String string() const { return body()->string(); }
private:
    class Body : public Expression::Body
    {
    public:
        Body(const String& string, const Span& span)
          : Expression::Body(span), _string(string) { }
        Expression stringify() { return this->expression(); }
        String toString()
        {
            CharacterSource s(_string);
            String r = "\"";
            do {
                int c = s.get();
                if (c == -1)
                    break;
                switch (c) {
                    case '\"':
                        r += "\\\"";
                        break;
                    case '$':
                        r += "\\$";
                        break;
                    default:
                        r += codePoint(c);
                        break;
                }
            } while (true);
            return r + "\"";
        }
        ValueT<T> evaluate(Structure* context) { return _string; }
        TypeT<T> type() { return StringType(); }
        bool mightHaveSideEffect() { return false; }
        String string() { return _string; }
    private:
        String _string;
    };
    Body* body() const { return to<Body>(); }
};

#endif // INCLUDED_EXPRESSION_H
