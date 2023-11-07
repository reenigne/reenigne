#include "alfe/main.h"

#ifndef INCLUDED_DOUBLE_FUNCTIONS_H
#define INCLUDED_DOUBLE_FUNCTIONS_H

#include "alfe/function.h"
#include "alfe/rational.h"

class AddDoubleDouble : public Nullary<Function, AddDoubleDouble>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            double l = i->value<double>();
            ++i;
            return Value(l + i->value<double>());
        }
        Identifier identifier() { return OperatorPlus(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), DoubleType(), DoubleType());
        }
    };
};

class AddDoubleRational : public Nullary<Function, AddDoubleRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            double l = i->value<double>();
            ++i;
            return Value(l + i->value<Rational>().value<double>());
        }
        Identifier identifier() { return OperatorPlus(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), DoubleType(), RationalType());
        }
    };
};

class AddRationalDouble : public Nullary<Function, AddRationalDouble>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l.value<double>() + i->value<double>());
        }
        Identifier identifier() { return OperatorPlus(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), RationalType(), DoubleType());
        }
    };
};

class SubtractDoubleDouble : public Nullary<Function, SubtractDoubleDouble>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            double l = i->value<double>();
            ++i;
            return Value(l - i->value<double>());
        }
        Identifier identifier() { return OperatorMinus(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), DoubleType(), DoubleType());
        }
    };
};

class SubtractDoubleRational : public Nullary<Function, SubtractDoubleRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            double l = i->value<double>();
            ++i;
            return Value(l - i->value<Rational>().value<double>());
        }
        Identifier identifier() { return OperatorMinus(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), DoubleType(), RationalType());
        }
    };
};

class SubtractRationalDouble : public Nullary<Function, SubtractRationalDouble>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l.value<double>() - i->value<double>());
        }
        Identifier identifier() { return OperatorMinus(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), RationalType(), DoubleType());
        }
    };
};

class MultiplyDoubleDouble : public Nullary<Function, MultiplyDoubleDouble>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            double l = i->value<double>();
            ++i;
            return Value(l * i->value<double>());
        }
        Identifier identifier() { return OperatorStar(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), DoubleType(), DoubleType());
        }
    };
};

class MultiplyDoubleRational : public Nullary<Function, MultiplyDoubleRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            double l = i->value<double>();
            ++i;
            return Value(l * i->value<Rational>().value<double>());
        }
        Identifier identifier() { return OperatorStar(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), DoubleType(), RationalType());
        }
    };
};

class MultiplyRationalDouble : public Nullary<Function, MultiplyRationalDouble>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l.value<double>() * i->value<double>());
        }
        Identifier identifier() { return OperatorStar(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), RationalType(), DoubleType());
        }
    };
};

class DivideDoubleDouble : public Nullary<Function, DivideDoubleDouble>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            double l = i->value<double>();
            ++i;
            return Value(l / i->value<double>());
        }
        Identifier identifier() { return OperatorDivide(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), DoubleType(), DoubleType());
        }
    };
};

class DivideDoubleRational : public Nullary<Function, DivideDoubleRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            double l = i->value<double>();
            ++i;
            return Value(l / i->value<Rational>().value<double>());
        }
        Identifier identifier() { return OperatorDivide(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), DoubleType(), RationalType());
        }
    };
};

class DivideRationalDouble : public Nullary<Function, DivideRationalDouble>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l.value<double>() / i->value<double>());
        }
        Identifier identifier() { return OperatorDivide(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), RationalType(), DoubleType());
        }
    };
};

class ShiftLeftDoubleInteger : public Nullary<Function, ShiftLeftDoubleInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            auto l = i->value<double>();
            ++i;
            return l*pow(2.0, i->value<int>());
        }
        Identifier identifier() { return OperatorShiftLeft(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), DoubleType(), IntegerType());
        }
    };
};

class ShiftRightDoubleInteger
  : public Nullary<Function, ShiftRightDoubleInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            auto l = i->value<double>();
            ++i;
            return l/pow(2.0, i->value<int>());
        }
        Identifier identifier() { return OperatorShiftRight(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), DoubleType(), IntegerType());
        }
    };
};

class PowerDoubleDouble : public Nullary<Function, PowerDoubleDouble>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            double l = i->value<double>();
            ++i;
            return pow(l, i->value<double>());
        }
        Identifier identifier() { return OperatorPower(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), DoubleType(), DoubleType());
        }
    };
};

class PowerDoubleRational : public Nullary<Function, PowerDoubleRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            double l = i->value<double>();
            ++i;
            return pow(l, i->value<Rational>().value<double>());
        }
        Identifier identifier() { return OperatorPower(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), DoubleType(), RationalType());
        }
    };
};

class PowerRationalDouble : public Nullary<Function, PowerRationalDouble>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return pow(l.value<double>(), i->value<double>());
        }
        Identifier identifier() { return OperatorPower(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), RationalType(), DoubleType());
        }
    };
};

class NegativeDouble : public Nullary<Function, NegativeDouble>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            return Value( - arguments.begin()->value<double>());
        }
        Identifier identifier() { return OperatorMinus(); }
        FuncoTyco type()
        {
            return FunctionType(DoubleType(), DoubleType());
        }
    };
};

#endif // INCLUDED_DOUBLE_FUNCTIONS_H
