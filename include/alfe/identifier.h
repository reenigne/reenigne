#include "alfe/main.h"

#ifndef INCLUDED_IDENTIFIER_H
#define INCLUDED_IDENTIFIER_H

#include "alfe/operator.h"
#include "alfe/expression.h"
#include "alfe/type_specifier.h"

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
            "true"
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
      : Expression(new OperatorImplementation(op, span)) { }

    String name() const { return as<Identifier>()->name(); }

    class Implementation : public ExpressionTemplate<T>::Implementation
    {
    public:
        Implementation(const Span& span) : Expression::Implementation(span) { }
        virtual String name() const = 0;
        TypedValue evaluate(EvaluationContext* context) const
        {
            return context->valueOfIdentifier(this);
        }
    };

    IdentifierTemplate(Implementation* implementation)
        : Expression(implementation) { }
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
        String name() const { return "operator" + _op.toString(); }
    private:
        Operator _op;
    };
    IdentifierTemplate() { }
};

#endif // INCLUDED_IDENTIFIER_H
