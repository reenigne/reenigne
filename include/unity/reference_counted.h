#ifndef INCLUDED_REFERENCE_COUNTED_H
#define INCLUDED_REFERENCE_COUNTED_H

#include "unity/uncopyable.h"

class ReferenceCounted : Uncopyable
{
public:
    ReferenceCounted() { _count = 0; }

    template<class U> U* cast() { return dynamic_cast<U*>(this); }
protected:
    virtual ~ReferenceCounted() { };

private:
    void addReference() const { ++_count; }

    void release() const
    {
        --_count;
        if (_count == 0)
            delete this;
    }

    mutable int _count;

    template<class T> friend class Reference;
    template<class T> friend class ConstReference;
};

template<class T> class Reference
{
public:
    Reference() : _t(0) { }
    Reference(const Reference& other) { set(other._t); }
    template<class U> Reference(const Reference<U>& other) { set(dynamic_cast<T*>(other._t)); }
    Reference(T* t) { set(t); }
    ~Reference() { reset(); }
    const Reference& operator=(const Reference& other)
    {
        if (this != &other) {
            reset();
            set(other._t);
        }
        return *this;
    }
    template<class U> Reference& operator=(const Reference<U>& other)
    {
        reset();
        set(dynamic_cast<T*>(other._t));
        return *this;
    }
    const Reference& operator=(T* t) { reset(); set(t); return *this; }

    T** operator&() { return &_t; }
    T* operator->() const { return _t; }
    operator T*() { return _t; }
    operator const T*() const { return _t; }
    bool valid() const { return _t != 0; }
    bool operator==(const Reference& other) const { return _t == other._t; }
private:
    void reset() { if (valid()) _t->release(); }
    void set(T* t)
    {
        _t = t;
        if (valid())
            _t->addReference();
    }
    T* _t;

    template<class U> friend class Reference;
};

template<class T> class ConstReference
{
public:
    ConstReference() : _t(0) { }
    ConstReference(const ConstReference& other) { set(other._t); }
    template<class U> ConstReference(const ConstReference<U>& other) { set(dynamic_cast<T*>(other._t)); }
    ConstReference(const T* t) { set(t); }
    ~ConstReference() { reset(); }
    const ConstReference& operator=(const ConstReference& other)
    {
        if (this != &other) {
            reset();
            set(other._t);
        }
        return *this;
    }
    template<class U> ConstReference& operator=(const ConstReference<U>& other)
    {
        reset();
        set(dynamic_cast<T*>(other._t));
        return *this;
    }
    const ConstReference& operator=(const T* t) { reset(); set(t); return *this; }

    const T* operator->() const { return _t; }
    operator const T*() const { return _t; }
    bool valid() const { return _t != 0; }
    bool operator==(const ConstReference& other) const { return _t == other._t; }

private:
    void reset() { if (valid()) _t->release(); }
    void set(const T* t)
    {
        _t = t;
        if (valid())
            _t->addReference();
    }
    const T* _t;

    template<class U> friend class Reference;
};

#endif // INCLUDED_REFERENCE_COUNTED_H
