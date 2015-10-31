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
    Nullary(typename Base::Body* body) : Base(body) { }
    Nullary(const typename Base::Body* body) : Base(body) { }
    static Nullary instance()
    {
        if (!_instance.valid())
            _instance = new typename My::Body();
        return _instance;
    }
private:
    static Nullary _instance;
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
        Body() { }
        String toString() const { return My::name(); }
        bool equals(const typename Base::Body* other) const
        {
            return dynamic_cast<const Body*>(other) == this;
        }
    };

    friend class Nullary<Base, My>;
    NamedNullary(const Body* body) : Nullary(body) { }
};

#endif // INCLUDED_NULLARY_H
