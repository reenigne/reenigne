#ifndef INCLUDED_REFERENCE_COUNTED_H
#define INCLUDED_REFERENCE_COUNTED_H

#include "uncopyable.h"

class ReferenceCounted : Uncopyable
{
public:
    ReferenceCounted() { _count = 0; }

private:
    ~ReferenceCounted() { }  // reference counted objects are heap-only.

    void addReference() { ++_count; }
    void release()
    {
        --_count;
        if (_count == 0)
            delete this;
    }

    int _count;

    template<class T> friend class Reference;
};

template<class T> class Reference
{
public:
    Reference() : _t(0) { }
    Reference(const Reference& other) { set(other._t); }
    template<class U> Reference(const Reference<U>& other)
      : _t(0)
    {
        T* t = dynamic_cast<T*>(other._t);
        if (t != 0)
            set(t);
    }
    Reference(T* t) { set(t); }
    ~Reference() { reset(); }
    const Reference& operator=(const Reference& other)
    {
        if (this != &other) {
            reset();
            set(other._t);
        }
    }
    const Reference& operator=(T* t) { reset(); set(t); }

    T** operator&() { return &_t; }
    T* operator->() { return _t; }
    operator T*() { return _t; }
    operator const T*() const { return _t; }
    bool valid() { return _t != 0; }
    bool operator==(const Reference& other) const { return _t == other->_t; }

private:
    void reset() { if (valid()) _t->release(); }
    void set(T* t) { _t = t; _t->addReference(); }
    T* _t;
};

#endif // INCLUDED_REFERENCE_COUNTED_H
