#include "alfe/main.h"

#ifndef INCLUDED_RATIONAL_FUNCTIONS_H
#define INCLUDED_RATIONAL_FUNCTIONS_H

#include "alfe/function.h"
#include "alfe/rational.h"

class AddRationalRational : public Nullary<Function, AddRationalRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return TypedValue(l + i->value<Rational>());
        }
        Identifier identifier() const { return OperatorPlus(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(RationalType(), RationalType(), RationalType()),
                this);
        }
    };
};

class AddRationalInteger : public Nullary<Function, AddRationalInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return TypedValue(l + i->value<int>());
        }
        Identifier identifier() const { return OperatorPlus(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(RationalType(), RationalType(), IntegerType()),
                this);
        }
    };
};

class AddIntegerRational : public Nullary<Function, AddIntegerRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return TypedValue(l + i->value<Rational>());
        }
        Identifier identifier() const { return OperatorPlus(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(RationalType(), IntegerType(), RationalType()),
                this);
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
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return TypedValue(l - i->value<Rational>());
        }
        Identifier identifier() const { return OperatorMinus(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(RationalType(), RationalType(), RationalType()),
                this);
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
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return TypedValue(l - i->value<int>());
        }
        Identifier identifier() const { return OperatorMinus(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(RationalType(), RationalType(), IntegerType()),
                this);
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
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return TypedValue(l - i->value<Rational>());
        }
        Identifier identifier() const { return OperatorMinus(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(RationalType(), IntegerType(), RationalType()),
                this);
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
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return TypedValue(l * i->value<Rational>());
        }
        Identifier identifier() const { return OperatorStar(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(RationalType(), RationalType(), RationalType()),
                this);
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
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return TypedValue(l * i->value<int>());
        }
        Identifier identifier() const { return OperatorStar(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(RationalType(), RationalType(), IntegerType()),
                this);
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
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return TypedValue(l * i->value<Rational>());
        }
        Identifier identifier() const { return OperatorStar(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(RationalType(), IntegerType(), RationalType()),
                this);
        }
    };
};

class DivideRationalRational : public Nullary<Function, DivideRationalRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return TypedValue(l / i->value<Rational>());
        }
        Identifier identifier() const { return OperatorDivide(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(RationalType(), RationalType(), RationalType()),
                this);
        }
    };
};

class DivideRationalInteger : public Nullary<Function, DivideRationalInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            Rational l = i->value<Rational>();
            ++i;
            return TypedValue(l / i->value<int>());
        }
        Identifier identifier() const { return OperatorDivide(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(RationalType(), RationalType(), IntegerType()),
                this);
        }
    };
};

class DivideIntegerRational : public Nullary<Function, DivideIntegerRational>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return TypedValue(l / i->value<Rational>());
        }
        Identifier identifier() const { return OperatorDivide(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(RationalType(), IntegerType(), RationalType()),
                this);
        }
    };
};

class DivideIntegerInteger : public Nullary<Function, DivideIntegerInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return TypedValue(Rational(l, i->value<int>()));
        }
        Identifier identifier() const { return OperatorDivide(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(RationalType(), IntegerType(), IntegerType()),
                this);
        }
    };
};

#endif // INCLUDED_RATIONAL_FUNCTIONS_H