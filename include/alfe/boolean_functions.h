#include "alfe/main.h"

#ifndef INCLUDED_BOOLEAN_FUNCTIONS_H
#define INCLUDED_BOOLEAN_FUNCTIONS_H

#include "alfe/function.h"

class LogicalNotBoolean : public Nullary<Function, LogicalNotBoolean>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span) const
        {
            return Value(!arguments.begin()->value<bool>());
        }
        Identifier identifier() const { return OperatorNot(); }
        FunctionType type() const
        {
            return FunctionType(BooleanType(), BooleanType());
        }
    };
};

#endif // INCLUDED_BOOLEAN_FUNCTIONS_H
