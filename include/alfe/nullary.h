#include "alfe/main.h"

#ifndef INCLUDED_NULLARY_H
#define INCLUDED_NULLARY_H

#include "alfe/string.h"

// Nullary is a helper class used for implementing classes which carry no data
// (apart from their vtable pointer). It's used for some subclasses of Type and
// Kind.
template<class Base, class My = Base> class Nullary : public Base
{
public:
    Nullary() : Base(instance()) { }
protected:
    class Implementation : public Base::Implementation
    {
    public:
        Implementation() { }
        String toString() const { return My::name(); }
        bool equals(const typename Base::Implementation* other)
        {
            const Implementation* o =
                dynamic_cast<const Implementation*>(other);
            return o == this;
        }
    };

    Nullary(const Implementation* implementation)
      : Base(implementation) { }
private:
    static Nullary _instance;
    static Nullary instance()
    {
        if (!_instance.valid())
            _instance = new typename My::Implementation();
        return _instance;
    }
};

#endif // INCLUDED_NULLARY_H
