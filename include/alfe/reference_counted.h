#ifndef INCLUDED_REFERENCE_COUNTED_H
#define INCLUDED_REFERENCE_COUNTED_H

#include "alfe/uncopyable.h"

class ReferenceCounted : Uncopyable
{
public:
    ReferenceCounted() { _count = 0; }

    void addReference() const { ++_count; }

    template<class U> U* cast() { return dynamic_cast<U*>(this); }
protected:
    virtual ~ReferenceCounted() { };

private:
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
    ~Reference() { reset(); }
    // We need a copy constructor and assignment operator here because
    // otherwise the compiler-generated ones would override the templated ones.
    Reference(const Reference& other) { set(other._t); }
    template<class U> Reference(const Reference<U>& other) { set(other._t); }
    template<class U> Reference(U* t) { set(t); }
    const Reference& operator=(const Reference& other)
    {
        if (this != &other) {
            reset();
            set(other._t);
        }
        return *this;
    }
    template<class U> const Reference& operator=(const Reference<U>& other)
    {
        if (this != &other) {
            reset();
            set(other._t);
        }
        return *this;
    }
    template<class U> const Reference& operator=(U* t)
    {
        reset();
        set(t);
        return *this;
    }

    T** operator&() { return &_t; }
    T* operator->() const { return _t; }
    operator T*() { return _t; }
    operator const T*() const { return _t; }
    bool valid() const { return _t != 0; }
    bool operator==(const Reference& other) const { return _t == other._t; }
private:
    void reset() { if (valid()) _t->release(); }
    template<class U> void set(U* t) { set(dynamic_cast<T*>(t)); }
    template<> void set(T* t)
    {
        _t = t;
        if (valid())
            _t->addReference();
    }
    T* _t;

    template<class U> friend class Reference;
    template<class U> friend class ConstReference;
};

template<class T> class ConstReference
{
public:
    ConstReference() : _t(0) { }
    ~ConstReference() { reset(); }
    // We need a copy constructor and assignment operator here because
    // otherwise the compiler-generated ones would override the templated ones.
    ConstReference(const ConstReference& other) { set(other._t); }
    template<class U> ConstReference(const ConstReference<U>& other)
    {
        set(other._t);
    }
    template<class U> ConstReference(const Reference<U>& other)
    {
        set(other._t);
    }
    template<class U> ConstReference(const U* t) { set(t); }
    const ConstReference& operator=(const ConstReference& other)
    {
        if (this != &other) {
            reset();
            set(other._t);
        }
        return *this;
    }
    template<class U> const ConstReference& operator=(
        const ConstReference<U>& other)
    {
        if (this != &other) {
            reset();
            set(other._t);
        }
        return *this;
    }
    template<class U> const ConstReference& operator=(
        const Reference<U>& other)
    {
        reset();
        set(other._t);
        return *this;
    }
    template<class U> const ConstReference& operator=(const U* t)
    {
        reset();
        set(t);
        return *this;
    }

    const T* operator->() const { return _t; }
    operator const T*() const { return _t; }
    bool valid() const { return _t != 0; }
    bool operator==(const ConstReference& other) const
    {
        return _t == other._t;
    }
private:
    void reset() { if (valid()) _t->release(); }
    template<class U> void set(const U* t) { set(dynamic_cast<const T*>(t)); }
    template<> void set(const T* t)
    {
        _t = t;
        if (valid())
            _t->addReference();
    }
    const T* _t;

    template<class U> friend class ConstReference;
};

#endif // INCLUDED_REFERENCE_COUNTED_H
