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
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return TypedValue(l + i->value<int>());
        }
        Identifier identifier() const { return OperatorPlus(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(IntegerType(), IntegerType(), IntegerType()),
                this);
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
        TypedValue evaluate(List<TypedValue> arguments) const
        {
            auto i = arguments.begin();
            int l = i->value<int>();
            ++i;
            return TypedValue(l - i->value<int>());
        }
        Identifier identifier() const { return OperatorMinus(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(IntegerType(), IntegerType(), IntegerType()),
                this);
        }
    };
};

class MultiplyIntegerInteger : public Nullary<Function, MultiplyIntegerInteger>
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
            return TypedValue(l * i->value<int>());
        }
        Identifier identifier() const { return OperatorStar(); }
        TypedValue typedValue() const
        {
            return TypedValue(
                FunctionTyco(IntegerType(), IntegerType(), IntegerType()),
                this);
        }
    };
};

#endif // INCLUDED_INTEGER_FUNCTIONS_H
