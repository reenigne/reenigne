#include "alfe/main.h"

#ifndef INCLUDED_INTEGER_FUNCTIONS_H
#define INCLUDED_INTEGER_FUNCTIONS_H

#include "alfe/function.h"

class AddIntegerInteger : public Nullary<Function, AddIntegerInteger>
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
            return Value(l + i->value<int>());
        }
        Identifier identifier() const { return OperatorPlus(); }
        FunctionTyco tyco() const
        {
            return FunctionTyco(IntegerType(), IntegerType(), IntegerType());
        }
    };
};

template<> Nullary<Function, AddIntegerInteger>
    Nullary<Function, AddIntegerInteger>::_instance;

class SubtractIntegerInteger : public Nullary<Function, SubtractIntegerInteger>
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
            return Value(l - i->value<int>());
        }
        Identifier identifier() const { return OperatorMinus(); }
        FunctionTyco tyco() const
        {
            return FunctionTyco(IntegerType(), IntegerType(), IntegerType());
        }
    };
};

template<> Nullary<Function, SubtractIntegerInteger>
    Nullary<Function, SubtractIntegerInteger>::_instance;

class MultiplyIntegerInteger : public Nullary<Function, MultiplyIntegerInteger>
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
            return Value(l * i->value<int>());
        }
        Identifier identifier() const { return OperatorStar(); }
        FunctionTyco tyco() const
        {
            return FunctionTyco(IntegerType(), IntegerType(), IntegerType());
        }
    };
};

template<> Nullary<Function, MultiplyIntegerInteger>
    Nullary<Function, MultiplyIntegerInteger>::_instance;

class ShiftLeftIntegerInteger
  : public Nullary<Function, ShiftLeftIntegerInteger>
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
            if (r < 0)
                return Rational(l, 1 << -r);
            return l << r;
        }
        Identifier identifier() const { return OperatorShiftLeft(); }
        FunctionTyco tyco() const
        {
            return FunctionTyco(RationalType(), IntegerType(), IntegerType());
        }
    };
};

template<> Nullary<Function, ShiftLeftIntegerInteger>
    Nullary<Function, ShiftLeftIntegerInteger>::_instance;

class ShiftRightIntegerInteger
  : public Nullary<Function, ShiftRightIntegerInteger>
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
            if (r < 0)
                return l << -r;
            return Rational(l, 1 << r);
        }
        Identifier identifier() const { return OperatorShiftRight(); }
        FunctionTyco tyco() const
        {
            return FunctionTyco(RationalType(), IntegerType(), IntegerType());
        }
    };
};

template<> Nullary<Function, ShiftRightIntegerInteger>
    Nullary<Function, ShiftRightIntegerInteger>::_instance;

#endif // INCLUDED_INTEGER_FUNCTIONS_H
