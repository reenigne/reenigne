#include "alfe/main.h"

#ifndef INCLUDED_BITWISE_H
#define INCLUDED_BITWISE_H

#ifdef _MSC_VER
#include <intrin.h>
#endif

int roundUpToPowerOf2(int n)
{
#ifdef _MSC_VER
    unsigned long k;
    _BitScanReverse(&k, n);
    if ((n & (n - 1)) != 0)
        ++k;
    return 1 << k;
#elif defined __GNUC__
    int k = (sizeof(int)*8 - 1) - __builtin_clz(n);
    if ((n & (n - 1)) != 0)
        ++k;
    return 1 << k;
#else
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
#endif
}

#endif // INCLUDED_BITWISE_H
