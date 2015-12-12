#include "alfe/main.h"

#ifndef INCLUDED_CONSTRUCTOR_H
#define INCLUDED_CONSTRUCTOR_H

// Constructor is a base class of both Tyco (type constructor) and Funco
// (function constructor). It handles instantiations.
class Constructor : public Handle
{
public:
    String toString() const { return body()->toString(); }
    Tyco instantiate(const Tyco& argument) const
    {
        return body()->instantiate(argument);
    }
protected:
    class Body : public Handle::Body
    {
    public:
        virtual String toString() const = 0;
        virtual bool canInstantiate(const Tyco& argument) const = 0;
        virtual Constructor instantiate(const Tyco& argument) const
        {
            if (_instantiations.hasKey(argument))
                return _instantiations[argument];

        }
    private:
        mutable HashTable<Tyco, Constructor> _instantiations;
    };
private:
    const Body* body() const { return as<Body>(); }
};

#endif // INCLUDED_CONSTRUCTOR_H
