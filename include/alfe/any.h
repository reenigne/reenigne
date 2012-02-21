#include "alfe/main.h"

#ifndef INCLUDED_ANY_H
#define INCLUDED_ANY_H

class Any
{
public:
    Any() { }
    template<class T> Any(const T& t)
      : _implementation(new Implementation<T>(t)) { }
    template<class T> T value() const
    {
        return Reference<Implementation<T> >(_implementation)->value();
    }
    bool valid() const { return _implementation.valid(); }
private:
    class ImplementationBase : public ReferenceCounted
    {
    };
    template<class T> class Implementation : public ImplementationBase
    {
    public:
        Implementation(const T& t) : _t(t) { }
        T value() const { return _t; }
    private:
        T _t;
    };
    Reference<ImplementationBase> _implementation;
};

#endif // INCLUDED_ANY_H
