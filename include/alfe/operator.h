#include "alfe/main.h"

#ifndef INCLUDED_OPERATOR_H
#define INCLUDED_OPERATOR_H

#include "alfe/nullary.h"

class Operator : public ConstHandle
{
public:
    Operator() { }

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
public:
    Operator(const Body* body) : ConstHandle(body) { }
};

class OperatorEqualTo : public NamedNullary<Operator, OperatorEqualTo>
{
public:
    static String name() { return "=="; }
};

template<> Nullary<Operator, OperatorEqualTo>
    Nullary<Operator, OperatorEqualTo>::_instance;

class OperatorAssignment : public NamedNullary<Operator, OperatorAssignment>
{
public:
    static String name() { return "="; }
};

template<> Nullary<Operator, OperatorAssignment>
    Nullary<Operator, OperatorAssignment>::_instance;

class OperatorAddAssignment : public NamedNullary<Operator, OperatorAddAssignment>
{
public:
    static String name() { return "+="; }
};

template<> Nullary<Operator, OperatorAddAssignment>
    Nullary<Operator, OperatorAddAssignment>::_instance;

class OperatorSubtractAssignment
  : public NamedNullary<Operator, OperatorSubtractAssignment>
{
public:
    static String name() { return "-="; }
};

template<> Nullary<Operator, OperatorSubtractAssignment>
    Nullary<Operator, OperatorSubtractAssignment>::_instance;

class OperatorMultiplyAssignment
  : public NamedNullary<Operator, OperatorMultiplyAssignment>
{
public:
    static String name() { return "*="; }
};

template<> Nullary<Operator, OperatorMultiplyAssignment>
    Nullary<Operator, OperatorMultiplyAssignment>::_instance;

class OperatorDivideAssignment
  : public NamedNullary<Operator, OperatorDivideAssignment>
{
public:
    static String name() { return "/="; }
};

template<> Nullary<Operator, OperatorDivideAssignment>
    Nullary<Operator, OperatorDivideAssignment>::_instance;

class OperatorModuloAssignment
  : public NamedNullary<Operator, OperatorModuloAssignment>
{
public:
    static String name() { return "%="; }
};

template<> Nullary<Operator, OperatorModuloAssignment>
    Nullary<Operator, OperatorModuloAssignment>::_instance;

class OperatorShiftLeftAssignment
  : public NamedNullary<Operator, OperatorShiftLeftAssignment>
{
public:
    static String name() { return "<<="; }
};

template<> Nullary<Operator, OperatorShiftLeftAssignment>
    Nullary<Operator, OperatorShiftLeftAssignment>::_instance;

class OperatorShiftRightAssignment
  : public NamedNullary<Operator, OperatorShiftRightAssignment>
{
public:
    static String name() { return ">>="; }
};

template<> Nullary<Operator, OperatorShiftRightAssignment>
    Nullary<Operator, OperatorShiftRightAssignment>::_instance;

class OperatorBitwiseAndAssignment
  : public NamedNullary<Operator, OperatorBitwiseAndAssignment>
{
public:
    static String name() { return "&="; }
};

template<> Nullary<Operator, OperatorBitwiseAndAssignment>
    Nullary<Operator, OperatorBitwiseAndAssignment>::_instance;

class OperatorBitwiseOrAssignment
  : public NamedNullary<Operator, OperatorBitwiseOrAssignment>
{
public:
    static String name() { return "|="; }
};

template<> Nullary<Operator, OperatorBitwiseOrAssignment>
    Nullary<Operator, OperatorBitwiseOrAssignment>::_instance;

class OperatorBitwiseXorAssignment
  : public NamedNullary<Operator, OperatorBitwiseXorAssignment>
{
public:
    static String name() { return "~="; }
};

template<> Nullary<Operator, OperatorBitwiseXorAssignment>
    Nullary<Operator, OperatorBitwiseXorAssignment>::_instance;

class OperatorPowerAssignment
  : public NamedNullary<Operator, OperatorPowerAssignment>
{
public:
    static String name() { return "^="; }
};

template<> Nullary<Operator, OperatorPowerAssignment>
    Nullary<Operator, OperatorPowerAssignment>::_instance;

class OperatorAmpersand : public NamedNullary<Operator, OperatorAmpersand>
{
public:
    static String name() { return "&"; }
};

template<> Nullary<Operator, OperatorAmpersand>
    Nullary<Operator, OperatorAmpersand>::_instance;

class OperatorBitwiseOr : public NamedNullary<Operator, OperatorBitwiseOr>
{
public:
    static String name() { return "|"; }
};

template<> Nullary<Operator, OperatorBitwiseOr>
    Nullary<Operator, OperatorBitwiseOr>::_instance;

class OperatorTwiddle : public NamedNullary<Operator, OperatorTwiddle>
{
public:
    static String name() { return "~"; }
};

template<> Nullary<Operator, OperatorTwiddle>
    Nullary<Operator, OperatorTwiddle>::_instance;

class OperatorNot : public NamedNullary<Operator, OperatorNot>
{
public:
    static String name() { return "!"; }
};

template<> Nullary<Operator, OperatorNot>
    Nullary<Operator, OperatorNot>::_instance;

class OperatorNotEqualTo : public NamedNullary<Operator, OperatorNotEqualTo>
{
public:
    static String name() { return "!="; }
};

template<> Nullary<Operator, OperatorNotEqualTo>
    Nullary<Operator, OperatorNotEqualTo>::_instance;

class OperatorLessThan : public NamedNullary<Operator, OperatorLessThan>
{
public:
    static String name() { return "<"; }
};

template<> Nullary<Operator, OperatorLessThan>
    Nullary<Operator, OperatorLessThan>::_instance;

class OperatorGreaterThan : public NamedNullary<Operator, OperatorGreaterThan>
{
public:
    static String name() { return ">"; }
};

template<> Nullary<Operator, OperatorGreaterThan>
    Nullary<Operator, OperatorGreaterThan>::_instance;

class OperatorLessThanOrEqualTo
  : public NamedNullary<Operator, OperatorLessThanOrEqualTo>
{
public:
    static String name() { return "<="; }
};

template<> Nullary<Operator, OperatorLessThanOrEqualTo>
    Nullary<Operator, OperatorLessThanOrEqualTo>::_instance;

class OperatorGreaterThanOrEqualTo
  : public NamedNullary<Operator, OperatorGreaterThanOrEqualTo>
{
public:
    static String name() { return ">="; }
};

template<> Nullary<Operator, OperatorGreaterThanOrEqualTo>
    Nullary<Operator, OperatorGreaterThanOrEqualTo>::_instance;

class OperatorShiftLeft : public NamedNullary<Operator, OperatorShiftLeft>
{
public:
    static String name() { return "<<"; }
};

template<> Nullary<Operator, OperatorShiftLeft>
    Nullary<Operator, OperatorShiftLeft>::_instance;

class OperatorShiftRight : public NamedNullary<Operator, OperatorShiftRight>
{
public:
    static String name() { return ">>"; }
};

template<> Nullary<Operator, OperatorShiftRight>
    Nullary<Operator, OperatorShiftRight>::_instance;

class OperatorPlus : public NamedNullary<Operator, OperatorPlus>
{
public:
    static String name() { return "+"; }
};

template<> Nullary<Operator, OperatorPlus>
    Nullary<Operator, OperatorPlus>::_instance;

class OperatorMinus : public NamedNullary<Operator, OperatorMinus>
{
public:
    static String name() { return "-"; }
};

template<> Nullary<Operator, OperatorMinus>
    Nullary<Operator, OperatorMinus>::_instance;

class OperatorStar : public NamedNullary<Operator, OperatorStar>
{
public:
    static String name() { return "*"; }
};

template<> Nullary<Operator, OperatorStar>
    Nullary<Operator, OperatorStar>::_instance;

class OperatorDivide : public NamedNullary<Operator, OperatorDivide>
{
public:
    static String name() { return "/"; }
};

template<> Nullary<Operator, OperatorDivide>
    Nullary<Operator, OperatorDivide>::_instance;

class OperatorModulo : public NamedNullary<Operator, OperatorModulo>
{
public:
    static String name() { return "%"; }
};

template<> Nullary<Operator, OperatorModulo>
    Nullary<Operator, OperatorModulo>::_instance;

class OperatorPower : public NamedNullary<Operator, OperatorPower>
{
public:
    static String name() { return "^"; }
};

template<> Nullary<Operator, OperatorPower>
    Nullary<Operator, OperatorPower>::_instance;

class OperatorFunctionCall : public NamedNullary<Operator, OperatorFunctionCall>
{
public:
    static String name() { return "()"; }
};

template<> Nullary<Operator, OperatorFunctionCall>
    Nullary<Operator, OperatorFunctionCall>::_instance;

class OperatorIndex : public NamedNullary<Operator, OperatorIndex>
{
public:
    static String name() { return "[]"; }
};

template<> Nullary<Operator, OperatorIndex>
    Nullary<Operator, OperatorIndex>::_instance;

class OperatorIncrement : public NamedNullary<Operator, OperatorIncrement>
{
public:
    static String name() { return "++"; }
};

template<> Nullary<Operator, OperatorIncrement>
    Nullary<Operator, OperatorIncrement>::_instance;

class OperatorDecrement : public NamedNullary<Operator, OperatorDecrement>
{
public:
    static String name() { return "--"; }
};

template<> Nullary<Operator, OperatorDecrement>
    Nullary<Operator, OperatorDecrement>::_instance;

#endif // INCLUDED_OPERATOR_H
