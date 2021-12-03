#include "alfe/main.h"

#ifndef INCLUDED_ARRAY_FUNCTIONS_H
#define INCLUDED_ARRAY_FUNCTIONS_H

#include "alfe/function.h"

class IndexArray : public Nullary<Funco, IndexArray>
{
public:
    class Body : public Nullary::Body
    {
    public:
        Value evaluate(List<Value> arguments, Span span)
        {
            auto i = arguments.begin();
            auto l = i->value<List<Value>>();
            ++i;
            Value r = *i;
            if (r.type() != IntegerType()) {
                span.throwError("Evaluating arrays with a non-integer indexer "
                    "has not been implemented.");
            }
            int rr = r.value<int>();
            if (rr < 0 || rr >= l.count()) {
                span.throwError("Array access out of bounds: " + decimal(rr) +
                    " is not in the range 0 to " + l.count() + ".");
            }
            auto x = l.begin();
            while (rr > 0) {
                ++x;
                --rr;
            }
            return *x;
        }
        Identifier identifier() { return OperatorIndex(); }
        bool argumentsMatch(List<Type> argumentTypes)
        {
            if (argumentTypes.count() != 2)
                return false;
            auto i = argumentTypes.begin();
            ArrayType l(*i);
            if (!l.valid())
                return false;
            ++i;
            Type indexType(*i);
            return indexType.canConvertTo(l.indexer());
        }
        List<Tyco> parameterTycos()
        {
            List<Tyco> r;
            r.add(ArrayTemplate());
            r.add(Type());
            return r;
        }
    };
};

#endif // INCLUDED_ARRAY_FUNCTIONS_H
