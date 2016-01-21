#include "alfe/main.h"

#ifndef INCLUDED_ANY_H
#define INCLUDED_ANY_H

class Any : private Handle
{
public:
    Any() { }
    template<class T, std::enable_if_t<!(std::is_base_of<ConstHandle, T>::value
        && sizeof(T) == sizeof(ConstHandle))>* = nullptr> Any(const T& t)
      : Handle(create<Body<T>>(t)) { }
    template<class T, std::enable_if_t<std::is_base_of<ConstHandle, T>::value
        && sizeof(T) == sizeof(ConstHandle)>* = nullptr> Any(const T& t)
    {
        set(reinterpret_cast<const ConstHandle*>(
            reinterpret_cast<const char*>(&t))->_body);
    }
    bool operator==(const Any& other) const
    {
        return Handle::operator==(other);
    }
    bool operator!=(const Any& other) const { return !(*this == other); }
    template<class T> class Body : public Handle::Body
    {
    public:
        Body(const T& t) : _t(t) { }
        T value() const { return _t; }
        bool equals(const Handle::Body* other) const
        {
            auto o = other->to<Body>();
            if (o == 0)
                return false;
            return _t == o->_t;
        }
    private:
        T _t;
    };
    template<class T, std::enable_if_t<!(std::is_base_of<ConstHandle, T>::value
        && sizeof(T) == sizeof(ConstHandle))>* = nullptr> T value() const
    {
        return as<Body<T>>()->value();
    }
    template<class T, std::enable_if_t<std::is_base_of<ConstHandle, T>::value
        && sizeof(T) == sizeof(ConstHandle)>* = nullptr> T value() const
    {
        ConstHandle h;
        h.set(_body);
        return *reinterpret_cast<T*>(reinterpret_cast<char*>(&h));
    }
};

#endif // INCLUDED_ANY_H
