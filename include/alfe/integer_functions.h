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

#endif // INCLUDED_INTEGER_FUNCTIONS_H
