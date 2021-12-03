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
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return Value(l + i->value<int>());
        }
        Identifier identifier() { return OperatorPlus(); }
        FunctionType type()
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
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return Value(l - i->value<int>());
        }
        Identifier identifier() { return OperatorMinus(); }
        FunctionType type()
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
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return Value(l * i->value<int>());
        }
        Identifier identifier() { return OperatorStar(); }
        FunctionType type()
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
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            int r = i->value<int>();
            if (r < 0)
                return Rational(l, 1 << -r);
            return l << r;
        }
        Identifier identifier() { return OperatorShiftLeft(); }
        FunctionType type()
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
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            int r = i->value<int>();
            if (r < 0)
                return l << -r;
            return Rational(l, 1 << r);
        }
        Identifier identifier() { return OperatorShiftRight(); }
        FunctionType type()
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
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return Value(l < i->value<int>());
        }
        Identifier identifier() { return OperatorLessThan(); }
        FunctionType type()
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
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return Value(l > i->value<int>());
        }
        Identifier identifier() { return OperatorGreaterThan(); }
        FunctionType type()
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
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return Value(l <= i->value<int>());
        }
        Identifier identifier() { return OperatorLessThanOrEqualTo(); }
        FunctionType type()
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
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return Value(l >= i->value<int>());
        }
        Identifier identifier() { return OperatorGreaterThanOrEqualTo(); }
        FunctionType type()
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
        Value evaluate(List<Value> arguments, Span span)
        {
            return Value( - arguments.begin()->value<int>());
        }
        Identifier identifier() { return OperatorMinus(); }
        FunctionType type()
        {
            return FunctionType(IntegerType(), IntegerType());
        }
    };
};

#endif // INCLUDED_INTEGER_FUNCTIONS_H