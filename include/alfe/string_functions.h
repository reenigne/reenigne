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

#endif // INCLUDED_STRING_FUNCTIONS_H
