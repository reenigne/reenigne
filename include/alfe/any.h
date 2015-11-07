#include "alfe/main.h"

#ifndef INCLUDED_ANY_H
#define INCLUDED_ANY_H

class Any : private Handle
{
public:
    Any() { }
    template<class T> Any(const T& t) : Handle(new Body<T>(t)) { }
    bool valid() const { return Handle::valid(); }
    template<class T> class Body : public Handle::Body
    {
    public:
        Body(const T& t) : _t(t) { }
        T value() const { return _t; }
    private:
        T _t;
    };
    template<> class Body<void> : public Handle::Body
    {
    public:
        void value() const { }
    };
    template<class T> T value() const { return as<Body<T>>()->value(); }
};

#endif // INCLUDED_ANY_H
