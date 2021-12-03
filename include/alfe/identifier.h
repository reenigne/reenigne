#include "alfe/code.h"

#ifndef INCLUDED_IDENTIFIER_H
#define INCLUDED_IDENTIFIER_H

template<class T> class ResolutionPathT;
typedef ResolutionPathT<void> ResolutionPath;

template<class T> class IdentifierT : public ExpressionT<T>
{
    class Body : public ExpressionT<T>::Body
    {
    public:
        Body(const Span& span) : Expression::Body(span) { }
        Identifier identifier() { return this->handle<Handle>(); }
        ValueT<T> evaluate(Structure* context)
        {
            return _path.evaluate(context, identifier());
        }
        virtual bool isOperator() = 0;
        TypeT<T> type() { return _definition.type(); }
        bool mightHaveSideEffect() { return false; }
    private:
        VariableDefinition _definition;
        ResolutionPath _path;
    };
    class NameBody : public Body
    {
    public:
        NameBody(const String& name, const Span& span)
          : Body(span), _name(name) { }
        String toString() { return _name; }
        bool isOperator() { return false; }
        Hash hash() { return Body::hash().mixin(_name.hash()); }
        bool equals(HandleBase::Body* other)
        {
            auto o = other->to<NameBody>();
            return o != 0 && _name == o->_name;
        }
    private:
        String _name;
    };
    class OperatorBody : public Body
    {
    public:
        OperatorBody(const Operator& op, const Span& span)
          : Body(span), _op(op) { }
        String toString() { return "operator" + _op.toString(); }
        bool isOperator() { return true; }
        Hash hash() { return Body::hash().mixin(_op.hash()); }
        bool equals(HandleBase::Body* other)
        {
            auto o = other->to<OperatorBody>();
            return o != 0 && _op == o->_op;
        }
    private:
        Operator _op;
    };
    class InternalBody : public Body
    {
    public:
        InternalBody(const Span& span)
          : Body(span)
        {
            static int n = 0;
            _n = n;
            ++n;
        }
        String toString() { return "\\" + decimal(_n); }
        bool isOperator() { return false; }
        Hash hash() { return Body::hash().mixin(_n); }
        bool equals(HandleBase::Body* other)
        {
            auto o = other->to<InternalBody>();
            return o != 0 && _n == o->_n;
        }
    private:
        int _n;
    };

public:
    IdentifierT() { }
    IdentifierT(Handle other) : ExpressionT<T>(other) { }
    IdentifierT(const String& name)
      : ExpressionT<T>(IdentifierT::template create<NameBody>(name, Span()))
    { }
    IdentifierT(const char* name)
      : ExpressionT<T>(IdentifierT::template create<NameBody>(name, Span()))
    { }
    IdentifierT(int)
      : ExpressionT<T>(IdentifierT::template create<InternalBody>()) { }
    void setDefinition(VariableDefinition d) { body()->_definition = d; }
    void setResolutionPath(ResolutionPath p) { body()->_path = p; }

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
            "false",
            "finally",
            "from",
            "for",
            "forever",
            "if",
            "in",
            "include",
            "new",
            "nothing",
            "return",
            "switch",
            "this",
            "throw",
            "true",
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
            return IdentifierT::template create<NameBody>(name, span);
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

        static Operator ops[] = {
            OperatorEqualTo(), OperatorAssignment(), OperatorAddAssignment(),
            OperatorSubtractAssignment(), OperatorMultiplyAssignment(),
            OperatorDivideAssignment(), OperatorModuloAssignment(),
            OperatorShiftLeftAssignment(), OperatorShiftRightAssignment(),
            OperatorBitwiseAndAssignment(), OperatorBitwiseOrAssignment(),
            OperatorBitwiseXorAssignment(), OperatorPowerAssignment(),
            OperatorBitwiseOr(), OperatorTwiddle(), OperatorNot(),
            OperatorAmpersand(), OperatorNotEqualTo(),
            OperatorLessThanOrEqualTo(), OperatorShiftRight(), Operator()};

        for (Operator* op = ops; op->valid(); ++op) {
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

        static Operator ops2[] = {
            OperatorGreaterThanOrEqualTo(), OperatorGreaterThan(),
            OperatorPlus(), OperatorMinus(), OperatorDivide(), OperatorStar(),
            OperatorModulo(), OperatorPower(), Operator()};

        for (Operator* op = ops2; op->valid(); ++op) {
            if (o.valid())
                break;
            o = op->parse(&s2, &endSpan);
        }
        if (!o.valid())
            s2.location().throwError("Expected an operator");
        *source = s2;
        return Identifier(o, span + endSpan);
    }

    IdentifierT(const Operator& op, const Span& span = Span())
      : Expression(IdentifierT::template create<OperatorBody>(op, span))
    { }

    bool isOperator() { return body()->isOperator(); }

private:
    Body* body() { return this->template as<Body>(); }
};

#endif // INCLUDED_IDENTIFIER_H
