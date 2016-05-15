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
        FunctionType type() const
        {
            return FunctionType(IntegerType(), IntegerType(), IntegerType());
        }
    };
};

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
        FunctionType type() const
        {
            return FunctionType(IntegerType(), IntegerType(), IntegerType());
        }
    };
};

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
        FunctionType type() const
        {
            return FunctionType(IntegerType(), IntegerType(), IntegerType());
        }
    };
};

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
        FunctionType type() const
        {
            return FunctionType(RationalType(), IntegerType(), IntegerType());
        }
    };
};

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
        FunctionType type() const
        {
            return FunctionType(RationalType(), IntegerType(), IntegerType());
        }
    };
};

class LessThanIntegerInteger : public Nullary<Function, LessThanIntegerInteger>
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
            return Value(l < i->value<int>());
        }
        Identifier identifier() const { return OperatorLessThan(); }
        FunctionType type() const
        {
            return FunctionType(BooleanType(), IntegerType(), IntegerType());
        }
    };
};

class GreaterThanIntegerInteger
    : public Nullary<Function, GreaterThanIntegerInteger>
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
            return Value(l > i->value<int>());
        }
        Identifier identifier() const { return OperatorGreaterThan(); }
        FunctionType type() const
        {
            return FunctionType(BooleanType(), IntegerType(), IntegerType());
        }
    };
};

class LessThanOrEqualToIntegerInteger
    : public Nullary<Function, LessThanOrEqualToIntegerInteger>
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
            return Value(l <= i->value<int>());
        }
        Identifier identifier() const { return OperatorLessThanOrEqualTo(); }
        FunctionType type() const
        {
            return FunctionType(BooleanType(), IntegerType(), IntegerType());
        }
    };
};

class GreaterThanOrEqualToIntegerInteger
    : public Nullary<Function, GreaterThanOrEqualToIntegerInteger>
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
            return Value(l >= i->value<int>());
        }
        Identifier identifier() const
        {
            return OperatorGreaterThanOrEqualTo();
        }
        FunctionType type() const
        {
            return FunctionType(BooleanType(), IntegerType(), IntegerType());
        }
    };
};

class NegativeInteger : public Nullary<Function, NegativeInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            return Value( - arguments.begin()->value<int>());
        }
        Identifier identifier() const { return OperatorMinus(); }
        FunctionType type() const
        {
            return FunctionType(IntegerType(), IntegerType());
        }
    };
};

#endif // INCLUDED_INTEGER_FUNCTIONS_H