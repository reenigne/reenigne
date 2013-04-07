#include "alfe/main.h"

#ifndef INCLUDED_EVALUATE_H
#define INCLUDED_EVALUATE_H

template<class T> T evaluate(String s)
{
    CharacterSource source(s);
    Expression e = Expression::parse(source);
    TypedValue v = e.evaluate();
    Type t = Type::fromCompileTimeType<T>();
    TypedValue v2 = Expression::convert(v, t);
    return v2.value<T>();
}

#endif // INCLUDED_EVALUATE_H
