#include "alfe/main.h"

#ifndef INCLUDED_VALUE_H
#define INCLUDED_VALUE_H

template<class T> class Value : private Handle
{
public:
    Value() : Handle(new Body) { }
    T* operator->() { return &body()->_t; }
    T& operator*() { return body()->_t; }
private:
    class Body : public Handle::Body
    {
    public:
        T _t;
    };
    Body* body() { return as<Body>(); }
};

#endif // INCLUDED_ANY_H
