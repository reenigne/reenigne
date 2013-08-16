#include "alfe/main.h"

#ifndef INCLUDED_ANY_H
#define INCLUDED_ANY_H

template<class T> class AnySpecificImplementation;

class Any
{
public:
    Any() { }
    template<class T> Any(const T& t)
      : _implementation(new AnySpecificImplementation<T>(t)) { }
    template<class T> T value() const
    {
        return _implementation.referent<AnySpecificImplementation<T> >()
            ->value();
    }
    bool valid() const { return _implementation.valid(); }
    class Implementation : public ReferenceCounted
    {
    };
private:
    ConstReference<Implementation> _implementation;
};

template<class T> class AnySpecificImplementation : public Any::Implementation
{
public:
    AnySpecificImplementation(const T& t) : _t(t) { }
    T value() const { return _t; }
private:
    T _t;
};
template<> class AnySpecificImplementation<void> : public Any::Implementation
{
public:
    void value() const { }
};

#endif // INCLUDED_ANY_H
