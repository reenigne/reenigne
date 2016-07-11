#include "alfe/main.h"

#ifndef INCLUDED_WRAP_H
#define INCLUDED_WRAP_H

// Works like the % operator but doesn't refect around 0
template<class T> T wrap(const T& a, const T& b)
{
    return a < 0 ? b - (-a)%b : a%b;
}

#endif // INCLUDED_WRAP_H
