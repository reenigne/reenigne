#include "alfe/main.h"

#ifndef INCLUDED_REFERENCE_H
#define INCLUDED_REFERENCE_H

template<class T> class Reference : private Handle
{
public:
    template<typename... Args> Reference(Args&&... args)
      : Handle(new Body(std::forward<Args>(args)...)) { }
    T* operator->() { return &body()->_t; }
    T& operator*() { return body()->_t; }
private:
    class Body : public Handle::Body
    {
    public:
        template<typename... Args> Body(Args&&... args)
          : _t(std::forward<Args>(args)...) { }
        T _t;
    };
    Body* body() { return as<Body>(); }
};

#endif // INCLUDED_REFERENCE_H
