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
        Value evaluate(List<Value> arguments, Span span)
        {
            return Value(!arguments.begin()->value<bool>());
        }
        Identifier identifier() { return OperatorNot(); }
        FuncoTyco type()
        {
            return FunctionType(BooleanType(), BooleanType());
        }
    };
};

#endif // INCLUDED_BOOLEAN_FUNCTIONS_H

