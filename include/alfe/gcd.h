#include "alfe/main.h"

#ifndef INCLUDED_GCD_H
#define INCLUDED_GCD_H

template<class T> T gcd(T x, T y)
{
    while (y != 0) {
        T t = y;
        y = x % y;
        x = t;
    }
    return x;
}

template<class T> T lcm(T x, T y)
{
    return (x/gcd(x,y))*y;
}

#endif // INCLUDED_GCD_H
