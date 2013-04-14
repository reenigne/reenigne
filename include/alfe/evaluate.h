#include "alfe/main.h"

#ifndef INCLUDED_EVALUATE_H
#define INCLUDED_EVALUATE_H

#include "alfe/expression.h"
#include "alfe/type.h"

TypedValue evaluate(Expression e)
{
    if (e.is<IntegerLiteral>())
        return TypedValue(Type::integer, Any(IntegerLiteral(e).value()));
    throw Exception("Don't know how to evaluate this expression");
}

template<class T> T evaluate(String s)
{
    CharacterSource source(s);
    Expression e = Expression::parse(source);
    TypedValue v = evaluate<int>(Expression e);
    Type t = Type::fromCompileTimeType<T>();
    TypedValue v2 = Expression::convert(v, t);
    return v2.value<T>();
}

#endif // INCLUDED_EVALUATE_H

