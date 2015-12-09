#include "alfe/main.h"

#ifndef INCLUDED_TUPLE_H
#define INCLUDED_TUPLE_H

template<class T1, class T2> class Tuple
{
public:
    Tuple() { }
    Tuple(const T1& t1, const T2& t2) : _t1(t1), _t2(t2) { }
    const T1& first() const { return _t1; }
    const T2& second() const { return _t2; }
    T1& first() { return _t1; }
    T2& second() { return _t2; }
private:
    T1 _t1;
    T2 _t2;
};

#endif // INCLUDED_TUPLE_H
