#include "alfe/main.h"

#ifndef INCLUDED_EXPRESSION_H
#define INCLUDED_EXPRESSION_H

#include "alfe/parse_tree_object.h"
#include "alfe/operator.h"
#include "alfe/identifier.h"
#include "alfe/type_specifier.h"

template<class T> class ExpressionTemplate;
typedef ExpressionTemplate<void> Expression;

template<class T> class IdentifierTemplate;
typedef IdentifierTemplate<void> Identifier;

template<class T> class FunctionCallExpressionTemplate;
typedef FunctionCallExpressionTemplate<void> FunctionCallExpression;

template<class T> class TypeTemplate;
typedef TypeTemplate<void> Type;

template<class T> class ValueTemplate;
typedef ValueTemplate<void> Value;

template<class T> class TycoTemplate;
typedef TycoTemplate<void> Tyco;

template<class T> class ValueTemplate;
typedef ValueTemplate<void> Value;

template<class T> class TycoIdentifierTemplate;
typedef TycoIdentifierTemplate<void> TycoIdentifier;

template<class T> class LogicalOrExpressionTemplate;
typedef LogicalOrExpressionTemplate<void> LogicalOrExpression;

template<class T> class NumericLiteralTemplate;
typedef NumericLiteralTemplate<void> NumericLiteral;

template<class T> class StructuredTypeTemplate;
typedef StructuredTypeTemplate<void> StructuredType;

template<class T> class LValueTypeTemplate;
typedef LValueTypeTemplate<void> LValueType;

template<class T> class LValueTemplate;
typedef LValueTemplate<void> LValue;

template<class T> class StructureTemplate;
typedef StructureTemplate<void> Structure;

template<class T> class FunctionTycoTemplate;
typedef FunctionTycoTemplate<void> FunctionTyco;

class BooleanType;

class EvaluationContext
{
public:
    virtual Value valueOfIdentifier(Identifier i) = 0;
    virtual Tyco resolveTycoIdentifier(TycoIdentifier i) = 0;
};

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

template<class T> class ExpressionTemplate : public ParseTreeObject
{
public:
    ExpressionTemplate() { }

    class Body : public ParseTreeObject::Body
    {
    public:
        Body(const Span& span) : ParseTreeObject::Body(span) { }
        virtual Expression toString() const
        {
            return new typename
                FunctionCallExpressionTemplate<T>::FunctionCallBody(
                Expression(this).dot(Identifier("toString")),
                List<Expression>(), span());
        }
        virtual Value evaluate(EvaluationContext* context) const = 0;
    };

    ExpressionTemplate(const Body* body) : ParseTreeObject(body) { }

    ExpressionTemplate(const String& string, const Span& span)
      : ParseTreeObject(new StringLiteralBody(string, span)) { }

    static Expression parse(CharacterSource* source)
    {
        return LogicalOrExpressionTemplate<T>::parse(source);
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
        *this = FunctionCallExpressionTemplate<T>::binary(OperatorPlus(),
            Span(), *this, other);
        return *this;
    }
    const Expression& operator-=(const Expression& other)
    {
        *this = FunctionCallExpressionTemplate<T>::binary(OperatorMinus(),
            Span(), *this, other);
        return *this;
    }
    Expression operator!() const
    {
        return FunctionCallExpressionTemplate<T>::unary(OperatorNot(),
            Span(), *this);
    }
    Expression dot(const Identifier& identifier)
    {
        return new DotBody(*this, identifier);
    }

    Expression toString() const { return body()->toString(); }

    static Expression parseDot(CharacterSource* source)
    {
        Expression e = FunctionCallExpressionTemplate<T>::parse(source);
        if (!e.valid())
            return e;
        do {
            Span span;
            if (Space::parseCharacter(source, '.', &span)) {
                IdentifierTemplate<T> i = IdentifierTemplate<T>::parse(source);
                if (!i.valid())
                    source->location().throwError("Identifier expected");
                e = e.dot(i);
                continue;
            }
            return e;
        } while (true);
    }

    ValueTemplate<T> evaluate(EvaluationContext* context) const
    {
        return body()->evaluate(context).simplify();
    }

protected:
    const Body* body() const { return as<Body>(); }

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
        e = IdentifierTemplate<T>::parse(source);
        if (e.valid())
            return e;
        e = FunctionCallExpression::parseConstructorCall(source);
        if (e.valid())
            return e;
        Span span;
        if (Space::parseKeyword(source, "true", &span))
            return new TrueBody(span);
        if (Space::parseKeyword(source, "false", &span))
            return new FalseBody(span);
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
                    return new ArrayLiteralBody(expressions,
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
                    part = IdentifierTemplate<T>::parse(source);
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
            return NumericLiteral(n, span);
        return Expression();
    }

    class TrueBody : public Body
    {
    public:
        TrueBody(const Span& span) : Body(span) { }
        Expression toString() const
        {
            return Expression("true", this->span());
        }
        ValueTemplate<T> evaluate(EvaluationContext* context) const
        {
            return true;
        }
    };
    class FalseBody : public Body
    {
    public:
        FalseBody(const Span& span) : Body(span) { }
        Expression toString() const
        {
            return Expression("false", this->span());
        }
        ValueTemplate<T> evaluate(EvaluationContext* context) const
        {
            return false;
        }
    };
    class StringLiteralBody : public Body
    {
    public:
        StringLiteralBody(const String& string, const Span& span)
          : Body(span), _string(string) { }
        Expression toString() const { return Expression(this); }
        ValueTemplate<T> evaluate(EvaluationContext* context) const
        {
            return _string;
        }
    private:
        String _string;
    };
    class ArrayLiteralBody : public Body
    {
    public:
        ArrayLiteralBody(const List<Expression>& expressions, const Span& span)
          : Body(span), _expressions(expressions) { }
        ValueTemplate<T> evaluate(EvaluationContext* context) const
        {
            return Value(StructuredTypeTemplate<T>(), _expressions);
        }
    private:
        List<Expression> _expressions;
    };
    class DotBody : public Body
    {
    public:
        DotBody(const Expression& left, const IdentifierTemplate<T>& right)
          : Body(left.span() + right.span()), _left(left), _right(right) { }
        ValueTemplate<T> evaluate(EvaluationContext* context) const
        {
            ValueTemplate<T> e = _left.evaluate(context);

            LValueTypeTemplate<T> lValueType(e.type());
            if (!lValueType.valid()) {
                if (!e.type().member(_right).valid()) {
                    this->span().throwError("Expression has no member named " +
                        _right.name());
                }
                auto m = e.value<HashTable<IdentifierTemplate<T>,
                    ValueTemplate<T>>>();
                e = m[_right];
                e = Value(e.type(), e.value(), this->span());
            }
            else {
                Type t = lValueType.inner().member(_right);
                if (!t.valid())
                    this->span().throwError("Expression has no member named " +
                        _right.name());
                StructureTemplate<T>* p = e.
                    template value<LValueTemplate<T>>().
                    rValue().template value<StructureTemplate<T>*>();
                e = Value(LValueTypeTemplate<T>::wrap(t), LValue(p, _right),
                    this->span());
            }
            return e;
        }
    private:
        ExpressionTemplate<T> _left;
        IdentifierTemplate<T> _right;
    };
};

template<class T> class NumericLiteralTemplate : public Expression
{
public:
    NumericLiteralTemplate(Rational n, Span span = Span())
      : Expression(new Body(n, span)) { }
    NumericLiteralTemplate(const Expression& e) : Expression(e) { }
    int value() const { return as<NumericLiteral>()->value(); }

    class Body : public Expression::Body
    {
    public:
        Body(Rational n, const Span& span) : Expression::Body(span), _n(n) { }
        Expression toString() const { return Expression(this); }
        ValueTemplate<T> evaluate(EvaluationContext* context) const
        {
            return _n;
        }
        Rational value() const { return _n; }
    private:
        Rational _n;
    };
};

template<class T> class FunctionCallExpressionTemplate : public Expression
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
        Expression e = FunctionCallExpression(
            new ConstructorCallBody(t, arguments, t.span() + span));
        return parseRemainder(e, source);
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
        static const Operator ops[] = { OperatorAmpersand(), Operator()};
        return parseHelper(source, ops, parseEquality);
    }

    static Expression parseXor(CharacterSource* source)
    {
        static const Operator ops[] = { OperatorTwiddle(), Operator()};
        return parseHelper(source, ops, parseBitwiseAnd);
    }

    static Expression parseBitwiseOr(CharacterSource* source)
    {
        static const Operator ops[] = { OperatorBitwiseOr(), Operator()};
        return parseHelper(source, ops, parseXor);
    }

    static FunctionCallExpression unary(const Operator& op, const Span& span,
        const Expression& expression)
    {
        Identifier identifier(op, span);
        List<Expression> arguments;
        arguments.add(expression);
        return new FunctionCallBody(identifier, arguments,
            span + expression.span());
    }

    static FunctionCallExpression binary(const Operator& op, const Span& span,
        const Expression& left, const Expression& right)
    {
        Identifier identifier(op, span);
        List<Expression> arguments;
        arguments.add(left);
        arguments.add(right);
        return new FunctionCallBody(identifier, arguments,
            left.span() + right.span());
    }

    class Body : public Expression::Body
    {
    protected:
        Body(const Span& span, const List<Expression>& arguments)
          : Expression::Body(span), _arguments(arguments) { }
        List<Expression> _arguments;
    };

    class FunctionCallBody : public Body
    {
    public:
        FunctionCallBody(const Expression& function,
            const List<Expression>& arguments, const Span& span)
          : Body(span, arguments), _function(function) { }
        ValueTemplate<T> evaluate(EvaluationContext* context) const
        {
            ValueTemplate<T> l = _function.evaluate(context).rValue();
            List<Value> arguments;
            for (auto p : _arguments)
                arguments.add(p.evaluate(context).rValue());
            TypeTemplate<T> lType = l.type();
            if (lType == OverloadedFunctionSet::Type())
                return l.value<OverloadedFunctionSet>().evaluate(arguments,
                    span());
            // What we have on the left isn't a function, try to call its
            // operator() method instead.
            IdentifierTemplate<T> i = Identifier(OperatorFunctionCall());
            if (!lType.member(i).valid())
                span().throwError("Expression is not a function.");
            LValueTypeTemplate<T> lValueType(lType);
            if (!lValueType.valid()) {
                auto m = l.template value<HashTable<Identifier, Value>>();
                l = m[i];
                l = Value(l.type(), l.value(), this->span());
            }
            else {
                StructureTemplate<T>* p = l.template
                    value<LValue>().rValue().template value<Structure*>();
                l = Value(LValueTypeTemplate<T>::wrap(p->getValue(i).type()),
                    LValue(p, i), this->span());
            }
            return l.template value<Function>().evaluate(arguments, span());
        }
    private:
        Expression _function;
    };

    class ConstructorCallBody : public Body
    {
    public:
        ConstructorCallBody(const TycoSpecifier& type,
            const List<Expression>& arguments, const Span& span)
          : Body(span, arguments), _type(type) { }
        ValueTemplate<T> evaluate(EvaluationContext* context) const
        {
            TycoIdentifier ti = _type;
            if (!ti.valid())
                _type.span().throwError(
                    "Don't know how to evaluate complex types yet.");
            Tyco t = context->resolveTycoIdentifier(ti);
            if (!t.valid())
                _type.span().throwError("Unknown type " + ti.name());
            List<Value> arguments;
            for (auto p : _arguments)
                arguments.add(p.evaluate(context));

            StructuredType type = t;
            if (!type.valid())
                ti.span().throwError(
                    "Only structure types can be constructed at the moment.");
            const Array<StructuredType::Member> members = type.members();
            List<Any> values;
            Span span = _type.span();
            auto ai = arguments.begin();
            for (int i = 0; i < members.count(); ++i) {
                Value value = ai->convertTo(members[i].type());
                values.add(value.value());
                span += value.span();
                ++ai;
            }
            return Value(type, values, span);
        }
    private:
        TycoSpecifier _type;
    };
private:
    static Expression parseRemainder(Expression e, CharacterSource* source)
    {
        do {
            Span span;
            if (!Space::parseCharacter(source, '(', &span))
                break;
            List<Expression> arguments = parseList(source);
            Space::assertCharacter(source, ')', &span);
            e = FunctionCallExpression(
                new FunctionCallBody(e, arguments, e.span() + span));
        } while (true);
        return e;
    }

    FunctionCallExpressionTemplate(const Body* body) : Expression(body) { }

    template<class U> friend class ExpressionTemplate;
};

template<class T> class BinaryExpression : public Expression
{
protected:
    class Body : public Expression::Body
    {
    public:
        Body(const Expression& left, const Span& operatorSpan,
            const Expression& right)
          : Expression::Body(left.span() + right.span()),
            _left(left), _right(right), _operatorSpan(operatorSpan) { }
        Expression left() const { return _left; }
        Expression right() const { return _right; }
    private:
        Expression _left;
        Expression _right;
        Span _operatorSpan;
    };
};

template<class T> class LogicalAndExpressionTemplate;
typedef LogicalAndExpressionTemplate<void> LogicalAndExpression;

template<class T> class LogicalAndExpressionTemplate
  : public BinaryExpression<LogicalAndExpression>
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
                e = new Body(e, span, e2);
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
        ValueTemplate<T> evaluate(EvaluationContext* context) const
        {
            ValueTemplate<T> v = left().evaluate(context);
            if (v.type() != BooleanType()) {
                left().span().throwError("Logical operator requires operand "
                    "of type Boolean.");
            }
            if (!v.template value<bool>())
                return false;
            v = right().evaluate(context);
            if (v.type() != BooleanType()) {
                right().span().throwError("Logical operator requires operand "
                    "of type Boolean.");
            }
            return v.template value<bool>();
        }
    };
};

template<class T> class LogicalOrExpressionTemplate
  : public BinaryExpression<LogicalOrExpression>
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
                e = new Body(e, span, e2);
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
        ValueTemplate<T> evaluate(EvaluationContext* context) const
        {
            ValueTemplate<T> v = left().evaluate(context);
            if (v.type() != BooleanType()) {
                left().span().throwError("Logical operator requires operand "
                    "of type Boolean.");
            }
            if (!v.template value<bool>())
                return false;
            v = right().evaluate(context);
            if (v.type() != BooleanType()) {
                right().span().throwError("Logical operator requires operand "
                    "of type Boolean.");
            }
            return v.template value<bool>();
        }
    };
};

#endif // INCLUDED_EXPRESSION_H
