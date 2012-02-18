#ifndef INCLUDED_RANDOM_H
#define INCLUDED_RANDOM_H

#include "alfe/integer_types.h"

UInt32 random32()
{
    static UInt32 r = 0;
    r = r*1103515245 + 12345;
    return r;
}

UInt32 random(UInt32 n) { return random32() % n; }

UInt64 random64(UInt64 n)
{
    return ((static_cast<UInt64>(random32()) << 32) + random32()) % n;
}

#endif // INCLUDED_RANDOM_H
