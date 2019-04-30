#include "alfe/main.h"

#ifndef INCLUDED_RATIONAL_FUNCTIONS_H
#define INCLUDED_RATIONAL_FUNCTIONS_H

#include "alfe/function.h"
#include "alfe/rational.h"
#include "alfe/power.h"

class AddRationalRational : public Nullary<Function, AddRationalRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l + i->value<Rational>());
        }
        Identifier identifier() const { return OperatorPlus(); }
        FunctionType type() const
        {
            return
                FunctionType(RationalType(), RationalType(), RationalType());
        }
    };
};

class AddRationalInteger : public Nullary<Function, AddRationalInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l + i->value<int>());
        }
        Identifier identifier() const { return OperatorPlus(); }
        FunctionType type() const
        {
            return FunctionType(RationalType(), RationalType(), IntegerType());
        }
    };
};

class AddIntegerRational : public Nullary<Function, AddIntegerRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return Value(l + i->value<Rational>());
        }
        Identifier identifier() const { return OperatorPlus(); }
        FunctionType type() const
        {
            return FunctionType(RationalType(), IntegerType(), RationalType());
        }
    };
};

class SubtractRationalRational
  : public Nullary<Function, SubtractRationalRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l - i->value<Rational>());
        }
        Identifier identifier() const { return OperatorMinus(); }
        FunctionType type() const
        {
            return
                FunctionType(RationalType(), RationalType(), RationalType());
        }
    };
};

class SubtractRationalInteger
  : public Nullary<Function, SubtractRationalInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l - i->value<int>());
        }
        Identifier identifier() const { return OperatorMinus(); }
        FunctionType type() const
        {
            return FunctionType(RationalType(), RationalType(), IntegerType());
        }
    };
};

class SubtractIntegerRational
  : public Nullary<Function, SubtractIntegerRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return Value(l - i->value<Rational>());
        }
        Identifier identifier() const { return OperatorMinus(); }
        FunctionType type() const
        {
            return FunctionType(RationalType(), IntegerType(), RationalType());
        }
    };
};

class MultiplyRationalRational
  : public Nullary<Function, MultiplyRationalRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l * i->value<Rational>());
        }
        Identifier identifier() const { return OperatorStar(); }
        FunctionType type() const
        {
            return
                FunctionType(RationalType(), RationalType(), RationalType());
        }
    };
};

class MultiplyRationalInteger
    : public Nullary<Function, MultiplyRationalInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l * i->value<int>());
        }
        Identifier identifier() const { return OperatorStar(); }
        FunctionType type() const
        {
            return FunctionType(RationalType(), RationalType(), IntegerType());
        }
    };
};

class MultiplyIntegerRational
    : public Nullary<Function, MultiplyIntegerRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return Value(l * i->value<Rational>());
        }
        Identifier identifier() const { return OperatorStar(); }
        FunctionType type() const
        {
            return FunctionType(RationalType(), IntegerType(), RationalType());
        }
    };
};

class DivideRationalRational : public Nullary<Function, DivideRationalRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l / i->value<Rational>());
        }
        Identifier identifier() const { return OperatorDivide(); }
        FunctionType type() const
        {
            return
                FunctionType(RationalType(), RationalType(), RationalType());
        }
    };
};

class DivideRationalInteger : public Nullary<Function, DivideRationalInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return Value(l / i->value<int>());
        }
        Identifier identifier() const { return OperatorDivide(); }
        FunctionType type() const
        {
            return FunctionType(RationalType(), RationalType(), IntegerType());
        }
    };
};

class DivideIntegerRational : public Nullary<Function, DivideIntegerRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return Value(l / i->value<Rational>());
        }
        Identifier identifier() const { return OperatorDivide(); }
        FunctionType type() const
        {
            return FunctionType(RationalType(), IntegerType(), RationalType());
        }
    };
};

class DivideIntegerInteger : public Nullary<Function, DivideIntegerInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return Value(Rational(l, i->value<int>()));
        }
        Identifier identifier() const { return OperatorDivide(); }
        FunctionType type() const
        {
            return FunctionType(RationalType(), IntegerType(), IntegerType());
        }
    };
};

class ShiftLeftRationalInteger
  : public Nullary<Function, ShiftLeftRationalInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            auto l = i->value<Rational>();
            ++i;
            int r = i->value<int>();
            if (r < 0)
                return Rational(l.numerator, l.denominator << -r);
            return Rational(l.numerator << r, l.denominator);
        }
        Identifier identifier() const { return OperatorShiftLeft(); }
        FunctionType type() const
        {
            return FunctionType(RationalType(), RationalType(), IntegerType());
        }
    };
};

class ShiftRightRationalInteger
  : public Nullary<Function, ShiftRightRationalInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            auto l = i->value<Rational>();
            ++i;
            int r = i->value<int>();
            if (r < 0)
                return Rational(l.numerator << -r, l.denominator);
            return Rational(l.numerator, l.denominator << r);
        }
        Identifier identifier() const { return OperatorShiftRight(); }
        FunctionType type() const
        {
            return FunctionType(RationalType(), RationalType(), IntegerType());
        }
    };
};

class PowerIntegerInteger : public Nullary<Function, PowerIntegerInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            int r = i->value<int>();
            return power(Rational(l), r);
        }
        Identifier identifier() const { return OperatorPower(); }
        FunctionType type() const
        {
            return FunctionType(RationalType(), IntegerType(), IntegerType());
        }
    };
};

class PowerRationalInteger : public Nullary<Function, PowerRationalInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            int r = i->value<int>();
            return power(l, r);
        }
        Identifier identifier() const { return OperatorPower(); }
        FunctionType type() const
        {
            return FunctionType(RationalType(), RationalType(), IntegerType());
        }
    };
};

class NegativeRational : public Nullary<Function, NegativeRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            return Value( - arguments.begin()->value<Rational>());
        }
        Identifier identifier() const { return OperatorMinus(); }
        FunctionType type() const
        {
            return FunctionType(RationalType(), RationalType());
        }
    };
};

class FloorRational : public Nullary<Function, FloorRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            return Value(arguments.begin()->value<Rational>().floor());
        }
        Identifier identifier() const { return "floor"; }
        FunctionType type() const
        {
            return FunctionType(IntegerType(), RationalType());
        }
    };
};

#endif // INCLUDED_RATIONAL_FUNCTIONS_H
