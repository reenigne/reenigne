#include "alfe/main.h"

#ifndef INCLUDED_OPERATOR_H
#define INCLUDED_OPERATOR_H

#include "alfe/nullary.h"

class Operator : public ConstHandle
{
public:
    Operator() { }
    Operator(const ConstHandle& other) : ConstHandle(other) { }

    String toString() const { return body()->toString(); }
    Operator parse(CharacterSource* source, Span* span) const
    {
        if (Space::parseOperator(source, toString(), span))
            return *this;
        return Operator();
    }
    bool operator==(const Operator& other) const
    {
        // All the bodies are nullary
        return body() == other.body();
    }
protected:
    class Body : public ConstHandle::Body
    {
    public:
        virtual String toString() const = 0;
    };
    const Body* body() const { return as<Body>(); }
};

class OperatorEqualTo : public NamedNullary<Operator, OperatorEqualTo>
{
public:
    static String name() { return "=="; }
};

class OperatorAssignment : public NamedNullary<Operator, OperatorAssignment>
{
public:
    static String name() { return "="; }
};

class OperatorAddAssignment
  : public NamedNullary<Operator, OperatorAddAssignment>
{
public:
    static String name() { return "+="; }
};

class OperatorSubtractAssignment
  : public NamedNullary<Operator, OperatorSubtractAssignment>
{
public:
    static String name() { return "-="; }
};

class OperatorMultiplyAssignment
  : public NamedNullary<Operator, OperatorMultiplyAssignment>
{
public:
    static String name() { return "*="; }
};

class OperatorDivideAssignment
  : public NamedNullary<Operator, OperatorDivideAssignment>
{
public:
    static String name() { return "/="; }
};

class OperatorModuloAssignment
  : public NamedNullary<Operator, OperatorModuloAssignment>
{
public:
    static String name() { return "%="; }
};

class OperatorShiftLeftAssignment
  : public NamedNullary<Operator, OperatorShiftLeftAssignment>
{
public:
    static String name() { return "<<="; }
};

class OperatorShiftRightAssignment
  : public NamedNullary<Operator, OperatorShiftRightAssignment>
{
public:
    static String name() { return ">>="; }
};

class OperatorBitwiseAndAssignment
  : public NamedNullary<Operator, OperatorBitwiseAndAssignment>
{
public:
    static String name() { return "&="; }
};

class OperatorBitwiseOrAssignment
  : public NamedNullary<Operator, OperatorBitwiseOrAssignment>
{
public:
    static String name() { return "|="; }
};

class OperatorBitwiseXorAssignment
  : public NamedNullary<Operator, OperatorBitwiseXorAssignment>
{
public:
    static String name() { return "~="; }
};

class OperatorPowerAssignment
  : public NamedNullary<Operator, OperatorPowerAssignment>
{
public:
    static String name() { return "^="; }
};

class OperatorAmpersand : public NamedNullary<Operator, OperatorAmpersand>
{
public:
    static String name() { return "&"; }
};

class OperatorBitwiseOr : public NamedNullary<Operator, OperatorBitwiseOr>
{
public:
    static String name() { return "|"; }
};

class OperatorTwiddle : public NamedNullary<Operator, OperatorTwiddle>
{
public:
    static String name() { return "~"; }
};

class OperatorNot : public NamedNullary<Operator, OperatorNot>
{
public:
    static String name() { return "!"; }
};

class OperatorNotEqualTo : public NamedNullary<Operator, OperatorNotEqualTo>
{
public:
    static String name() { return "!="; }
};

class OperatorLessThan : public NamedNullary<Operator, OperatorLessThan>
{
public:
    static String name() { return "<"; }
};

class OperatorGreaterThan : public NamedNullary<Operator, OperatorGreaterThan>
{
public:
    static String name() { return ">"; }
};

class OperatorLessThanOrEqualTo
  : public NamedNullary<Operator, OperatorLessThanOrEqualTo>
{
public:
    static String name() { return "<="; }
};

class OperatorGreaterThanOrEqualTo
  : public NamedNullary<Operator, OperatorGreaterThanOrEqualTo>
{
public:
    static String name() { return ">="; }
};

class OperatorShiftLeft : public NamedNullary<Operator, OperatorShiftLeft>
{
public:
    static String name() { return "<<"; }
};

class OperatorShiftRight : public NamedNullary<Operator, OperatorShiftRight>
{
public:
    static String name() { return ">>"; }
};

class OperatorPlus : public NamedNullary<Operator, OperatorPlus>
{
public:
    static String name() { return "+"; }
};

class OperatorMinus : public NamedNullary<Operator, OperatorMinus>
{
public:
    static String name() { return "-"; }
};

class OperatorStar : public NamedNullary<Operator, OperatorStar>
{
public:
    static String name() { return "*"; }
};

class OperatorDivide : public NamedNullary<Operator, OperatorDivide>
{
public:
    static String name() { return "/"; }
};

class OperatorModulo : public NamedNullary<Operator, OperatorModulo>
{
public:
    static String name() { return "%"; }
};

class OperatorPower : public NamedNullary<Operator, OperatorPower>
{
public:
    static String name() { return "^"; }
};

class OperatorFunctionCall
  : public NamedNullary<Operator, OperatorFunctionCall>
{
public:
    static String name() { return "()"; }
};

class OperatorIndex : public NamedNullary<Operator, OperatorIndex>
{
public:
    static String name() { return "[]"; }
};

class OperatorIncrement : public NamedNullary<Operator, OperatorIncrement>
{
public:
    static String name() { return "++"; }
};

class OperatorDecrement : public NamedNullary<Operator, OperatorDecrement>
{
public:
    static String name() { return "--"; }
};

#endif // INCLUDED_OPERATOR_H
