#include "alfe/main.h"

#ifndef INCLUDED_IDENTIFIER_H
#define INCLUDED_IDENTIFIER_H

#include "alfe/operator.h"
#include "alfe/expression.h"
#include "alfe/type_specifier.h"

template<class My> class IdentifierOperator;

template<class T> class IdentifierTemplate : public ExpressionTemplate<T>
{
public:
    IdentifierTemplate(const String& name)
      : ExpressionTemplate<T>(new NameImplementation(name, Span())) { }
    IdentifierTemplate(const char* name)
      : ExpressionTemplate<T>(new NameImplementation(name, Span())) { }

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

    IdentifierTemplate(const Operator& op, const Span& span = Span())
      : Expression(
            new typename IdentifierOperator<T>::Implementation(op, span)) { }

    String name() const { return implementation()->name(); }
    bool isOperator() const { return implementation()->isOperator(); }
    int hash() const
    {
        return this->template as<IdentifierTemplate<T>>()->hash();
    }
    bool operator==(const Identifier& other) const
    {
        if (_implementation == other._implementation)
            return true;
        return implementation()->equals(other.implementation());
    }

    class Implementation : public ExpressionTemplate<T>::Implementation
    {
    public:
        Implementation(const Span& span) : Expression::Implementation(span) { }
        virtual String name() const = 0;
        TypedValueTemplate<T> evaluate(EvaluationContext* context) const
        {
            return context->valueOfIdentifier(this);
        }
        virtual bool isOperator() const = 0;
        virtual int hash() const = 0;
        virtual bool equals(const Implementation* other) const
        {
            return this == other;
        }
    };

    IdentifierTemplate(const Implementation* implementation)
      : Expression(implementation) { }

    IdentifierTemplate() { }
private:
    class NameImplementation : public Implementation
    {
    public:
        NameImplementation(const String& name, const Span& span)
          : Implementation(span), _name(name) { }
        String name() const { return _name; }
        bool isOperator() const { return false; }
        int hash() const { return _name.hash(); }
        bool equals(const Implementation* other) const
        {
            const NameImplementation* o =
                dynamic_cast<const NameImplementation*>(other);
            if (o == 0)
                return false;
            return _name == o->_name;
        }
    private:
        String _name;
    };

    const Implementation* implementation() const
    {
        return this->template as<IdentifierTemplate<T>>();
    }
};

template<class My> class IdentifierOperator : public Identifier
{
public:
    IdentifierOperator() { }

    class Implementation : public Identifier::Implementation
    {
    public:
        Implementation(const Operator& op = My.op(),
            const Span& span = Span())
          : Identifier::Implementation(span), _op(op) { }
        String name() const { return "operator" + _op.toString(); }
        bool isOperator() const { return true; }
        int hash() const { return _op.hash(); }
    private:
        Operator _op;
    };
    IdentifierOperator(const Implementation* implementation)
      : Identifier(implementation) { }
private:
    static IdentifierOperator _instance;
    static IdentifierOperator instance()
    {
        if (!_instance.valid())
            _instance = new Implementation();
        return _instance;
    }
};

class IdentifierEqualTo : public IdentifierOperator<IdentifierEqualTo>
{
public:
    static Operator op() { return OperatorEqualTo(); }
};

template<> IdentifierOperator<IdentifierEqualTo>
    IdentifierOperator<IdentifierEqualTo>::_instance;

class IdentifierAssignment : public IdentifierOperator<IdentifierAssignment>
{
public:
    static Operator op() { return OperatorAssignment(); }
};

template<> IdentifierOperator<IdentifierAssignment>
    IdentifierOperator<IdentifierAssignment>::_instance;

class IdentifierAddAssignment
  : public IdentifierOperator<IdentifierAddAssignment>
{
public:
    static Operator op() { return OperatorAddAssignment(); }
};

template<> IdentifierOperator<IdentifierAddAssignment>
    IdentifierOperator<IdentifierAddAssignment>::_instance;

class IdentifierSubtractAssignment
    : public IdentifierOperator<IdentifierSubtractAssignment>
{
public:
    static Operator op() { return OperatorSubtractAssignment(); }
};

template<> IdentifierOperator<IdentifierSubtractAssignment>
    IdentifierOperator<IdentifierSubtractAssignment>::_instance;

class IdentifierMultiplyAssignment
    : public IdentifierOperator<IdentifierMultiplyAssignment>
{
public:
    static Operator op() { return OperatorMultiplyAssignment(); }
};

template<> IdentifierOperator<IdentifierMultiplyAssignment>
    IdentifierOperator<IdentifierMultiplyAssignment>::_instance;

class IdentifierDivideAssignment
    : public IdentifierOperator<IdentifierDivideAssignment>
{
public:
    static Operator op() { return OperatorDivideAssignment(); }
};

template<> IdentifierOperator<IdentifierDivideAssignment>
    IdentifierOperator<IdentifierDivideAssignment>::_instance;

class IdentifierModuloAssignment
    : public IdentifierOperator<IdentifierModuloAssignment>
{
public:
    static Operator op() { return OperatorModuloAssignment(); }
};

template<> IdentifierOperator<IdentifierModuloAssignment>
    IdentifierOperator<IdentifierModuloAssignment>::_instance;

class IdentifierShiftLeftAssignment
    : public IdentifierOperator<IdentifierShiftLeftAssignment>
{
public:
    static Operator op() { return OperatorShiftLeftAssignment(); }
};

template<> IdentifierOperator<IdentifierShiftLeftAssignment>
    IdentifierOperator<IdentifierShiftLeftAssignment>::_instance;

class IdentifierShiftRightAssignment
    : public IdentifierOperator<IdentifierShiftRightAssignment>
{
public:
    static Operator op() { return OperatorShiftRightAssignment(); }
};

template<> IdentifierOperator<IdentifierShiftRightAssignment>
    IdentifierOperator<IdentifierShiftRightAssignment>::_instance;

class IdentifierBitwiseAndAssignment
    : public IdentifierOperator<IdentifierBitwiseAndAssignment>
{
public:
    static Operator op() { return OperatorBitwiseAndAssignment(); }
};

template<> IdentifierOperator<IdentifierBitwiseAndAssignment>
    IdentifierOperator<IdentifierBitwiseAndAssignment>::_instance;

class IdentifierBitwiseOrAssignment
    : public IdentifierOperator<IdentifierBitwiseOrAssignment>
{
public:
    static Operator op() { return OperatorBitwiseOrAssignment(); }
};

template<> IdentifierOperator<IdentifierBitwiseOrAssignment>
    IdentifierOperator<IdentifierBitwiseOrAssignment>::_instance;

class IdentifierBitwiseXorAssignment
    : public IdentifierOperator<IdentifierBitwiseXorAssignment>
{
public:
    static Operator op() { return OperatorBitwiseXorAssignment(); }
};

template<> IdentifierOperator<IdentifierBitwiseXorAssignment>
    IdentifierOperator<IdentifierBitwiseXorAssignment>::_instance;

class IdentifierPowerAssignment
    : public IdentifierOperator<IdentifierPowerAssignment>
{
public:
    static Operator op() { return OperatorPowerAssignment(); }
};

template<> IdentifierOperator<IdentifierPowerAssignment>
    IdentifierOperator<IdentifierPowerAssignment>::_instance;

class IdentifierAmpersand : public IdentifierOperator<IdentifierAmpersand>
{
public:
    static Operator op() { return OperatorAmpersand(); }
};

template<> IdentifierOperator<IdentifierAmpersand>
    IdentifierOperator<IdentifierAmpersand>::_instance;

class IdentifierBitwiseOr : public IdentifierOperator<IdentifierBitwiseOr>
{
public:
    static Operator op() { return OperatorBitwiseOr(); }
};

template<> IdentifierOperator<IdentifierBitwiseOr>
    IdentifierOperator<IdentifierBitwiseOr>::_instance;

class IdentifierTwiddle : public IdentifierOperator<IdentifierTwiddle>
{
public:
    static Operator op() { return OperatorTwiddle(); }
};

template<> IdentifierOperator<IdentifierTwiddle>
    IdentifierOperator<IdentifierTwiddle>::_instance;

class IdentifierNot : public IdentifierOperator<IdentifierNot>
{
public:
    static Operator op() { return OperatorNot(); }
};

template<> IdentifierOperator<IdentifierNot>
    IdentifierOperator<IdentifierNot>::_instance;

class IdentifierNotEqualTo
  : public IdentifierOperator<IdentifierNotEqualTo>
{
public:
    static Operator op() { return OperatorNotEqualTo(); }
};

template<> IdentifierOperator<IdentifierNotEqualTo>
    IdentifierOperator<IdentifierNotEqualTo>::_instance;

class IdentifierLessThan : public IdentifierOperator<IdentifierLessThan>
{
public:
    static Operator op() { return OperatorLessThan(); }
};

template<> IdentifierOperator<IdentifierLessThan>
    IdentifierOperator<IdentifierLessThan>::_instance;

class IdentifierGreaterThan : public IdentifierOperator<IdentifierGreaterThan>
{
public:
    static Operator op() { return OperatorGreaterThan(); }
};

template<> IdentifierOperator<IdentifierGreaterThan>
    IdentifierOperator<IdentifierGreaterThan>::_instance;

class IdentifierLessThanOrEqualTo
  : public IdentifierOperator<IdentifierLessThanOrEqualTo>
{
public:
    static Operator op() { return OperatorLessThanOrEqualTo(); }
};

template<> IdentifierOperator<IdentifierLessThanOrEqualTo>
    IdentifierOperator<IdentifierLessThanOrEqualTo>::_instance;

class IdentifierGreaterThanOrEqualTo
  : public IdentifierOperator<IdentifierGreaterThanOrEqualTo>
{
public:
    static Operator op() { return OperatorGreaterThanOrEqualTo(); }
};

template<> IdentifierOperator<IdentifierGreaterThanOrEqualTo>
    IdentifierOperator<IdentifierGreaterThanOrEqualTo>::_instance;

class IdentifierShiftLeft : public IdentifierOperator<IdentifierShiftLeft>
{
public:
    static Operator op() { return OperatorShiftLeft(); }
};

template<> IdentifierOperator<IdentifierShiftLeft>
    IdentifierOperator<IdentifierShiftLeft>::_instance;

class IdentifierShiftRight : public IdentifierOperator<IdentifierShiftRight>
{
public:
    static Operator op() { return OperatorShiftRight(); }
};

template<> IdentifierOperator<IdentifierShiftRight>
    IdentifierOperator<IdentifierShiftRight>::_instance;

class IdentifierPlus : public IdentifierOperator<IdentifierPlus>
{
public:
    static Operator op() { return OperatorPlus(); }
};

template<> IdentifierOperator<IdentifierPlus>
    IdentifierOperator<IdentifierPlus>::_instance;

class IdentifierMinus : public IdentifierOperator<IdentifierMinus>
{
public:
    static Operator op() { return OperatorMinus(); }
};

template<> IdentifierOperator<IdentifierMinus>
    IdentifierOperator<IdentifierMinus>::_instance;

class IdentifierStar : public IdentifierOperator<IdentifierStar>
{
public:
    static Operator op() { return OperatorStar(); }
};

template<> IdentifierOperator<IdentifierStar>
    IdentifierOperator<IdentifierStar>::_instance;

class IdentifierDivide : public IdentifierOperator<IdentifierDivide>
{
public:
    static Operator op() { return OperatorDivide(); }
};

template<> IdentifierOperator<IdentifierDivide>
    IdentifierOperator<IdentifierDivide>::_instance;

class IdentifierModulo : public IdentifierOperator<IdentifierModulo>
{
public:
    static Operator op() { return OperatorModulo(); }
};

template<> IdentifierOperator<IdentifierModulo>
    IdentifierOperator<IdentifierModulo>::_instance;

class IdentifierPower : public IdentifierOperator<IdentifierPower>
{
public:
    static Operator op() { return OperatorPower(); }
};

template<> IdentifierOperator<IdentifierPower>
    IdentifierOperator<IdentifierPower>::_instance;

class IdentifierFunctionCall
  : public IdentifierOperator<IdentifierFunctionCall>
{
public:
    static Operator op() { return OperatorFunctionCall(); }
};

template<> IdentifierOperator<IdentifierFunctionCall>
    IdentifierOperator<IdentifierFunctionCall>::_instance;

class IdentifierIndex : public IdentifierOperator<IdentifierIndex>
{
public:
    static Operator op() { return OperatorIndex(); }
};

template<> IdentifierOperator<IdentifierIndex>
    IdentifierOperator<IdentifierIndex>::_instance;

class IdentifierIncrement : public IdentifierOperator<IdentifierIncrement>
{
public:
    static Operator op() { return OperatorIncrement(); }
};

template<> IdentifierOperator<IdentifierIncrement>
    IdentifierOperator<IdentifierIncrement>::_instance;

class IdentifierDecrement : public IdentifierOperator<IdentifierDecrement>
{
public:
    static Operator op() { return OperatorDecrement(); }
};

template<> IdentifierOperator<IdentifierDecrement>
    IdentifierOperator<IdentifierDecrement>::_instance;

#endif // INCLUDED_IDENTIFIER_H
