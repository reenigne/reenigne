#include "alfe/main.h"

#ifndef INCLUDED_RELATIONAL_FUNCTIONS_H
#define INCLUDED_RELATIONAL_FUNCTIONS_H

#include "alfe/function.h"

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

#endif // INCLUDED_RELATIONAL_FUNCTIONS_H
