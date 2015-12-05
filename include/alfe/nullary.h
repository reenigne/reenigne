#include "alfe/main.h"

#ifndef INCLUDED_NULLARY_H
#define INCLUDED_NULLARY_H

#include "alfe/string.h"

// Nullary is a helper class used for implementing classes which carry no data
// (apart from their vtable pointer).
template<class Base, class My = Base> class Nullary : public Base
{
public:
    Nullary() : Base(instance()) { }
protected:
    Nullary(const Base& other) : Base(other) { }
    static Nullary instance()
    {
        static Nullary instance(Base::create<My::Body>());
        return instance;
    }
private:
    template<class Base, class My> friend class NamedNullary;
};

// NamedNullary is used for Operator and some subclasses of Type and Kind.
template<class Base, class My = Base> class NamedNullary
  : public Nullary<Base, My>
{
public:
    NamedNullary() : Nullary(instance()) { }
protected:
    NamedNullary(const Base& other) : Nullary(other) { }
    class Body : public Base::Body
    {
    public:
        String toString() const { return My::name(); }
    };

    friend class Nullary<Base, My>;
};

#endif // INCLUDED_NULLARY_H
