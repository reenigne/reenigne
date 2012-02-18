#ifndef INCLUDED_VALUE_H
#define INCLUDED_VALUE_H

#include "alfe/reference_counted.h"

template<class T> class Value
{
public:
    Value() : _implementation(new Implementation) { }
    T* operator->() const { return &_implementation->_t; }
    T& operator*() const { return _implementation->_t; }
private:
    class Implementation : public ReferenceCounted
    {
    public:
        T _t;
    };
    Reference<Implementation> _implementation;
};

#endif // INCLUDED_ANY_H
