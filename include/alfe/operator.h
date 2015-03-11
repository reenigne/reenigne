#include "alfe/main.h"

#ifndef INCLUDED_OPERATOR_H
#define INCLUDED_OPERATOR_H

#include "alfe/nullary.h"

class Operator
{
public:
    Operator() { }

    String toString() const { return _implementation->toString(); }
    Operator parse(CharacterSource* source, Span* span) const
    {
        if (Space::parseOperator(source, toString(), span))
            return *this;
        return Operator();
    }
    int hash() const
    {
        // All the implementations are nullary
        return
            reinterpret_cast<int>(_implementation.referent<Implementation>());
    }
    bool operator==(const Operator& other) const
    {
        // All the implementations are nullary
        return _implementation == other._implementation;
    }

    bool valid() const { return _implementation.valid(); }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual String toString() const = 0;
    };
public:
    Operator(const Implementation* implementation)
      : _implementation(implementation) { }
private:
    ConstReference<Implementation> _implementation;
};

class OperatorEqualTo : public Nullary<Operator, OperatorEqualTo>
{
public:
    static String name() { return "=="; }
};

template<> Nullary<Operator, OperatorEqualTo>
    Nullary<Operator, OperatorEqualTo>::_instance;

class OperatorAssignment : public Nullary<Operator, OperatorAssignment>
{
public:
    static String name() { return "="; }
};

template<> Nullary<Operator, OperatorAssignment>
    Nullary<Operator, OperatorAssignment>::_instance;

class OperatorAddAssignment : public Nullary<Operator, OperatorAddAssignment>
{
public:
    static String name() { return "+="; }
};

template<> Nullary<Operator, OperatorAddAssignment>
    Nullary<Operator, OperatorAddAssignment>::_instance;

class OperatorSubtractAssignment
  : public Nullary<Operator, OperatorSubtractAssignment>
{
public:
    static String name() { return "-="; }
};

template<> Nullary<Operator, OperatorSubtractAssignment>
    Nullary<Operator, OperatorSubtractAssignment>::_instance;

class OperatorMultiplyAssignment
  : public Nullary<Operator, OperatorMultiplyAssignment>
{
public:
    static String name() { return "*="; }
};

template<> Nullary<Operator, OperatorMultiplyAssignment>
    Nullary<Operator, OperatorMultiplyAssignment>::_instance;

class OperatorDivideAssignment
  : public Nullary<Operator, OperatorDivideAssignment>
{
public:
    static String name() { return "/="; }
};

template<> Nullary<Operator, OperatorDivideAssignment>
    Nullary<Operator, OperatorDivideAssignment>::_instance;

class OperatorModuloAssignment
  : public Nullary<Operator, OperatorModuloAssignment>
{
public:
    static String name() { return "%="; }
};

template<> Nullary<Operator, OperatorModuloAssignment>
    Nullary<Operator, OperatorModuloAssignment>::_instance;

class OperatorShiftLeftAssignment
  : public Nullary<Operator, OperatorShiftLeftAssignment>
{
public:
    static String name() { return "<<="; }
};

template<> Nullary<Operator, OperatorShiftLeftAssignment>
    Nullary<Operator, OperatorShiftLeftAssignment>::_instance;

class OperatorShiftRightAssignment
  : public Nullary<Operator, OperatorShiftRightAssignment>
{
public:
    static String name() { return ">>="; }
};

template<> Nullary<Operator, OperatorShiftRightAssignment>
    Nullary<Operator, OperatorShiftRightAssignment>::_instance;

class OperatorBitwiseAndAssignment
  : public Nullary<Operator, OperatorBitwiseAndAssignment>
{
public:
    static String name() { return "&="; }
};

template<> Nullary<Operator, OperatorBitwiseAndAssignment>
    Nullary<Operator, OperatorBitwiseAndAssignment>::_instance;

class OperatorBitwiseOrAssignment
  : public Nullary<Operator, OperatorBitwiseOrAssignment>
{
public:
    static String name() { return "|="; }
};

template<> Nullary<Operator, OperatorBitwiseOrAssignment>
    Nullary<Operator, OperatorBitwiseOrAssignment>::_instance;

class OperatorBitwiseXorAssignment
  : public Nullary<Operator, OperatorBitwiseXorAssignment>
{
public:
    static String name() { return "~="; }
};

template<> Nullary<Operator, OperatorBitwiseXorAssignment>
    Nullary<Operator, OperatorBitwiseXorAssignment>::_instance;

class OperatorPowerAssignment
  : public Nullary<Operator, OperatorPowerAssignment>
{
public:
    static String name() { return "^="; }
};

template<> Nullary<Operator, OperatorPowerAssignment>
    Nullary<Operator, OperatorPowerAssignment>::_instance;

class OperatorAmpersand : public Nullary<Operator, OperatorAmpersand>
{
public:
    static String name() { return "&"; }
};

template<> Nullary<Operator, OperatorAmpersand>
    Nullary<Operator, OperatorAmpersand>::_instance;

class OperatorBitwiseOr : public Nullary<Operator, OperatorBitwiseOr>
{
public:
    static String name() { return "|"; }
};

template<> Nullary<Operator, OperatorBitwiseOr>
    Nullary<Operator, OperatorBitwiseOr>::_instance;

class OperatorTwiddle : public Nullary<Operator, OperatorTwiddle>
{
public:
    static String name() { return "~"; }
};

template<> Nullary<Operator, OperatorTwiddle>
    Nullary<Operator, OperatorTwiddle>::_instance;

class OperatorNot : public Nullary<Operator, OperatorNot>
{
public:
    static String name() { return "!"; }
};

template<> Nullary<Operator, OperatorNot>
    Nullary<Operator, OperatorNot>::_instance;

class OperatorNotEqualTo : public Nullary<Operator, OperatorNotEqualTo>
{
public:
    static String name() { return "!="; }
};

template<> Nullary<Operator, OperatorNotEqualTo>
    Nullary<Operator, OperatorNotEqualTo>::_instance;

class OperatorLessThan : public Nullary<Operator, OperatorLessThan>
{
public:
    static String name() { return "<"; }
};

template<> Nullary<Operator, OperatorLessThan>
    Nullary<Operator, OperatorLessThan>::_instance;

class OperatorGreaterThan : public Nullary<Operator, OperatorGreaterThan>
{
public:
    static String name() { return ">"; }
};

template<> Nullary<Operator, OperatorGreaterThan>
    Nullary<Operator, OperatorGreaterThan>::_instance;

class OperatorLessThanOrEqualTo
  : public Nullary<Operator, OperatorLessThanOrEqualTo>
{
public:
    static String name() { return "<="; }
};

template<> Nullary<Operator, OperatorLessThanOrEqualTo>
    Nullary<Operator, OperatorLessThanOrEqualTo>::_instance;

class OperatorGreaterThanOrEqualTo
  : public Nullary<Operator, OperatorGreaterThanOrEqualTo>
{
public:
    static String name() { return ">="; }
};

template<> Nullary<Operator, OperatorGreaterThanOrEqualTo>
    Nullary<Operator, OperatorGreaterThanOrEqualTo>::_instance;

class OperatorShiftLeft : public Nullary<Operator, OperatorShiftLeft>
{
public:
    static String name() { return "<<"; }
};

template<> Nullary<Operator, OperatorShiftLeft>
    Nullary<Operator, OperatorShiftLeft>::_instance;

class OperatorShiftRight : public Nullary<Operator, OperatorShiftRight>
{
public:
    static String name() { return ">>"; }
};

template<> Nullary<Operator, OperatorShiftRight>
    Nullary<Operator, OperatorShiftRight>::_instance;

class OperatorPlus : public Nullary<Operator, OperatorPlus>
{
public:
    static String name() { return "+"; }
};

template<> Nullary<Operator, OperatorPlus>
    Nullary<Operator, OperatorPlus>::_instance;

class OperatorMinus : public Nullary<Operator, OperatorMinus>
{
public:
    static String name() { return "-"; }
};

template<> Nullary<Operator, OperatorMinus>
    Nullary<Operator, OperatorMinus>::_instance;

class OperatorStar : public Nullary<Operator, OperatorStar>
{
public:
    static String name() { return "*"; }
};

template<> Nullary<Operator, OperatorStar>
    Nullary<Operator, OperatorStar>::_instance;

class OperatorDivide : public Nullary<Operator, OperatorDivide>
{
public:
    static String name() { return "/"; }
};

template<> Nullary<Operator, OperatorDivide>
    Nullary<Operator, OperatorDivide>::_instance;

class OperatorModulo : public Nullary<Operator, OperatorModulo>
{
public:
    static String name() { return "%"; }
};

template<> Nullary<Operator, OperatorModulo>
    Nullary<Operator, OperatorModulo>::_instance;

class OperatorPower : public Nullary<Operator, OperatorPower>
{
public:
    static String name() { return "^"; }
};

template<> Nullary<Operator, OperatorPower>
    Nullary<Operator, OperatorPower>::_instance;

class OperatorFunctionCall : public Nullary<Operator, OperatorFunctionCall>
{
public:
    static String name() { return "()"; }
};

template<> Nullary<Operator, OperatorFunctionCall>
    Nullary<Operator, OperatorFunctionCall>::_instance;

class OperatorIndex : public Nullary<Operator, OperatorIndex>
{
public:
    static String name() { return "[]"; }
};

template<> Nullary<Operator, OperatorIndex>
    Nullary<Operator, OperatorIndex>::_instance;

class OperatorIncrement : public Nullary<Operator, OperatorIncrement>
{
public:
    static String name() { return "++"; }
};

template<> Nullary<Operator, OperatorIncrement>
    Nullary<Operator, OperatorIncrement>::_instance;

class OperatorDecrement : public Nullary<Operator, OperatorDecrement>
{
public:
    static String name() { return "--"; }
};

template<> Nullary<Operator, OperatorDecrement>
    Nullary<Operator, OperatorDecrement>::_instance;

#endif // INCLUDED_OPERATOR_H
