#include "alfe/main.h"

#ifndef INCLUDED_OPERATOR_H
#define INCLUDED_OPERATOR_H

#include "alfe/space.h"

class Operator
{
public:
    Operator() { }

    String name() const { return _implementation->name(); }
    Operator parse(CharacterSource* source, Span* span) const
    {
        if (Space::parseOperator(source, name(), span))
            return *this;
        return Operator();
    }

    bool valid() const { return _implementation.valid(); }
protected:
    class Implementation : public ReferenceCounted
    {
    public:
        virtual String name() const = 0;
    };
public:
    Operator(const Implementation* implementation)
      : _implementation(implementation) { }
private:
    ConstReference<Implementation> _implementation;
};

template<class T> class OperatorBase : public Operator
{
public:
    OperatorBase() : Operator(op()) { }
private:
    class Implementation : public Operator::Implementation
    {
    public:
        String name() const { return T::name(); }
    };
    static Operator _operator;
    static Operator op()
    {
        if (!_operator.valid())
            _operator = new Implementation();
        return _operator;
    }
};

class OperatorEqualTo : public OperatorBase<OperatorEqualTo>
{
public:
    static String name() { return "=="; }
};

Operator OperatorBase<OperatorEqualTo>::_operator;

class OperatorAssignment : public OperatorBase<OperatorAssignment>
{
public:
    static String name() { return "="; }
};

Operator OperatorBase<OperatorAssignment>::_operator;

class OperatorAddAssignment : public OperatorBase<OperatorAddAssignment>
{
public:
    static String name() { return "+="; }
};

Operator OperatorBase<OperatorAddAssignment>::_operator;

class OperatorSubtractAssignment
  : public OperatorBase<OperatorSubtractAssignment>
{
public:
    static String name() { return "-="; }
};

Operator OperatorBase<OperatorSubtractAssignment>::_operator;

class OperatorMultiplyAssignment
  : public OperatorBase<OperatorMultiplyAssignment>
{
public:
    static String name() { return "*="; }
};

Operator OperatorBase<OperatorMultiplyAssignment>::_operator;

class OperatorDivideAssignment : public OperatorBase<OperatorDivideAssignment>
{
public:
    static String name() { return "/="; }
};

Operator OperatorBase<OperatorDivideAssignment>::_operator;

class OperatorModuloAssignment : public OperatorBase<OperatorModuloAssignment>
{
public:
    static String name() { return "%="; }
};

Operator OperatorBase<OperatorModuloAssignment>::_operator;

class OperatorShiftLeftAssignment
  : public OperatorBase<OperatorShiftLeftAssignment>
{
public:
    static String name() { return "<<="; }
};

Operator OperatorBase<OperatorShiftLeftAssignment>::_operator;

class OperatorShiftRightAssignment
  : public OperatorBase<OperatorShiftRightAssignment>
{
public:
    static String name() { return ">>="; }
};

Operator OperatorBase<OperatorShiftRightAssignment>::_operator;

class OperatorBitwiseAndAssignment
  : public OperatorBase<OperatorBitwiseAndAssignment>
{
public:
    static String name() { return "&="; }
};

Operator OperatorBase<OperatorBitwiseAndAssignment>::_operator;

class OperatorBitwiseOrAssignment
  : public OperatorBase<OperatorBitwiseOrAssignment>
{
public:
    static String name() { return "|="; }
};

Operator OperatorBase<OperatorBitwiseOrAssignment>::_operator;

class OperatorBitwiseXorAssignment
  : public OperatorBase<OperatorBitwiseXorAssignment>
{
public:
    static String name() { return "~="; }
};

Operator OperatorBase<OperatorBitwiseXorAssignment>::_operator;

class OperatorPowerAssignment : public OperatorBase<OperatorPowerAssignment>
{
public:
    static String name() { return "^="; }
};

Operator OperatorBase<OperatorPowerAssignment>::_operator;

class OperatorAmpersand : public OperatorBase<OperatorAmpersand>
{
public:
    static String name() { return "&"; }
};

Operator OperatorBase<OperatorAmpersand>::_operator;

class OperatorBitwiseOr : public OperatorBase<OperatorBitwiseOr>
{
public:
    static String name() { return "|"; }
};

Operator OperatorBase<OperatorBitwiseOr>::_operator;

class OperatorTwiddle : public OperatorBase<OperatorTwiddle>
{
public:
    static String name() { return "~"; }
};

Operator OperatorBase<OperatorTwiddle>::_operator;

class OperatorNot : public OperatorBase<OperatorNot>
{
public:
    static String name() { return "!"; }
};

Operator OperatorBase<OperatorNot>::_operator;

class OperatorNotEqualTo : public OperatorBase<OperatorNotEqualTo>
{
public:
    static String name() { return "!="; }
};

Operator OperatorBase<OperatorNotEqualTo>::_operator;

class OperatorLessThan : public OperatorBase<OperatorLessThan>
{
public:
    static String name() { return "<"; }
};

Operator OperatorBase<OperatorLessThan>::_operator;

class OperatorGreaterThan : public OperatorBase<OperatorGreaterThan>
{
public:
    static String name() { return ">"; }
};

Operator OperatorBase<OperatorGreaterThan>::_operator;

class OperatorLessThanOrEqualTo : public OperatorBase<OperatorLessThanOrEqualTo>
{
public:
    static String name() { return "<="; }
};

Operator OperatorBase<OperatorLessThanOrEqualTo>::_operator;

class OperatorGreaterThanOrEqualTo
    : public OperatorBase<OperatorGreaterThanOrEqualTo>
{
public:
    static String name() { return ">="; }
};

Operator OperatorBase<OperatorGreaterThanOrEqualTo>::_operator;

class OperatorShiftLeft : public OperatorBase<OperatorShiftLeft>
{
public:
    static String name() { return "<<"; }
};

Operator OperatorBase<OperatorShiftLeft>::_operator;

class OperatorShiftRight : public OperatorBase<OperatorShiftRight>
{
public:
    static String name() { return ">>"; }
};

Operator OperatorBase<OperatorShiftRight>::_operator;

class OperatorPlus : public OperatorBase<OperatorPlus>
{
public:
    static String name() { return "+"; }
};

Operator OperatorBase<OperatorPlus>::_operator;

class OperatorMinus : public OperatorBase<OperatorMinus>
{
public:
    static String name() { return "-"; }
};

Operator OperatorBase<OperatorMinus>::_operator;

class OperatorStar : public OperatorBase<OperatorStar>
{
public:
    static String name() { return "*"; }
};

Operator OperatorBase<OperatorStar>::_operator;

class OperatorDivide : public OperatorBase<OperatorDivide>
{
public:
    static String name() { return "/"; }
};

Operator OperatorBase<OperatorDivide>::_operator;

class OperatorModulo : public OperatorBase<OperatorModulo>
{
public:
    static String name() { return "%"; }
};

Operator OperatorBase<OperatorModulo>::_operator;

class OperatorPower : public OperatorBase<OperatorPower>
{
public:
    static String name() { return "^"; }
};

Operator OperatorBase<OperatorPower>::_operator;

class OperatorFunctionCall : public OperatorBase<OperatorFunctionCall>
{
public:
    static String name() { return "()"; }
};

Operator OperatorBase<OperatorFunctionCall>::_operator;

class OperatorIndex : public OperatorBase<OperatorIndex>
{
public:
    static String name() { return "[]"; }
};

Operator OperatorBase<OperatorIndex>::_operator;

class OperatorIncrement : public OperatorBase<OperatorIncrement>
{
public:
    static String name() { return "++"; }
};

Operator OperatorBase<OperatorIncrement>::_operator;

class OperatorDecrement : public OperatorBase<OperatorDecrement>
{
public:
    static String name() { return "--"; }
};

Operator OperatorBase<OperatorDecrement>::_operator;

#endif // INCLUDED_OPERATOR_H
