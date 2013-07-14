#include "alfe/main.h"

#ifndef INCLUDED_EVALUATE_H
#define INCLUDED_EVALUATE_H

#include "alfe/space.h"

template<class T> T evaluate(CharacterSource s)
{
    throw Exception("Don't know how to evaluate this expression");
}

template<> int evaluate<int>(CharacterSource s)
{
    int c = s.get();
    if (c < '0' || c > '9')
        return 0;
    int n = 0;
    do {
        n = n*10 + c - '0';
        c = s.get();
        if (c < '0' || c > '9') {
            Space::parse(&s);
            return n;
        }
    } while (true);
}

template<class T> T evaluate(String s)
{
    CharacterSource source(s);
    return evaluate(source);
}

#endif // INCLUDED_EVALUATE_H

