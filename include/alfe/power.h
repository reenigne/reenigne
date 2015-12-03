#include "alfe/main.h"

#ifndef INCLUDED_POWER_H
#define INCLUDED_POWER_H

// Computes a^b
template<class T> T power(T a, int b)
{
    T r = 1;
    if (b < 0) {
        a = 1/a;
        b = -b;
    }
    while (b > 0) {
        if ((b & 1) != 0)
            r = r*a;
        a *= a;
        b >>= 1;
    }
    return r;
}

#endif // INCLUDED_POWER_H
