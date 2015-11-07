#include "alfe/main.h"

#ifndef INCLUDED_STRING_FUNCTIONS_H
#define INCLUDED_STRING_FUNCTIONS_H

#include "alfe/function.h"

class AddStringString : public Nullary<Function, AddStringString>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            String l = i->value<String>();
            ++i;
            return Value(l + i->value<String>());
        }
        Identifier identifier() const { return OperatorPlus(); }
        FunctionTyco tyco() const
        {
            return FunctionTyco(StringType(), StringType(), StringType());
        }
    };
};

template<> Nullary<Function, AddStringString>
    Nullary<Function, AddStringString>::_instance;

class MultiplyIntegerString : public Nullary<Function, MultiplyIntegerString>
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
            return Value(l*i->value<String>());
        }
        Identifier identifier() const { return OperatorStar(); }
        FunctionTyco tyco() const
        {
            return FunctionTyco(StringType(), IntegerType(), StringType());
        }
    };
};

template<> Nullary<Function, MultiplyIntegerString>
    Nullary<Function, MultiplyIntegerString>::_instance;

class MultiplyStringInteger : public Nullary<Function, MultiplyStringInteger>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            auto i = arguments.begin();
            String l = i->value<String>();
            ++i;
            return Value(l*i->value<int>());
        }
        Identifier identifier() const { return OperatorStar(); }
        FunctionTyco tyco() const
        {
            return FunctionTyco(StringType(), StringType(), IntegerType());
        }
    };
};

template<> Nullary<Function, MultiplyStringInteger>
    Nullary<Function, MultiplyStringInteger>::_instance;

#endif // INCLUDED_STRING_FUNCTIONS_H
