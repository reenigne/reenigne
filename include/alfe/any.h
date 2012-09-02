#include "alfe/main.h"

#ifndef INCLUDED_ANY_H
#define INCLUDED_ANY_H

class Any
{
public:
    Any() { }
    template<class T> Any(const T& t)
      : _implementation(new SpecificImplementation<T>(t)) { }
    template<class T> T value() const
    {
        return _implementation.referent<SpecificImplementation<T> >()
            ->value();
    }
    bool valid() const { return _implementation.valid(); }
private:
    class Implementation : public ReferenceCounted
    {
    };
    template<class T> class SpecificImplementation : public Implementation
    {
    public:
        SpecificImplementation(const T& t) : _t(t) { }
        T value() const { return _t; }
    private:
        T _t;
    };
    ConstReference<Implementation> _implementation;
};

#endif // INCLUDED_ANY_H
