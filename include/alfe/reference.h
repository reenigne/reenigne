#include "alfe/main.h"

#ifndef INCLUDED_REFERENCE_H
#define INCLUDED_REFERENCE_H

class ReferenceCounted : Uncopyable
{
public:
    ReferenceCounted() : _count(0) { }
protected:
    virtual ~ReferenceCounted() { };
    template<class T> T* cast() const { return dynamic_cast<T*>(this); }
    template<class T> const T* constCast() const
    {
        return dynamic_cast<const T*>(this);
    }
    virtual void destroy() { delete this; }
private:
    void release()
    {
        --_count;
        if (_count == 0)
            destroy();
    }
    void addReference() { ++_count; }
    int _count;
    template<class T> friend class Reference;
    template<class T> friend class ConstReference;
};

template<class T> class Reference
{
public:
    Reference() : _referent(0) { }
    ~Reference() { reset(); }
    // We need a copy constructor and assignment operator here because
    // otherwise the compiler-generated ones would override the templated ones.
    Reference(const Reference& other) { set(other._referent); }
    template<class U> Reference(const Reference<U>& other)
    {
        set(other._referent);
    }
    Reference(T* referent) { set(referent); }
    const Reference& operator=(const Reference& other)
    {
        modify(other._referent);
        return *this;
    }
    template<class U> const Reference& operator=(const Reference<U>& other)
    {
        modify(other._referent);
        return *this;
    }

    bool operator==(const Reference& other) const
    {
        return _referent == other._referent;
    }
    template<class U> U* referent() const { return _referent->cast<U>(); }
    T* operator->() const { return _referent; }
    operator T*() { return _referent; }
    operator const T*() const { return _referent; }
    bool valid() const { return _referent != 0; }
private:
    void reset() { if (valid()) _referent->release(); }
    void set(T* referent)
    {
        _referent = referent;
        if (valid())
            _referent->addReference();
    }
    void modify(T* referent)
    {
        if (referent != _referent)
            return;
        reset();
        set(referent);
    }
    T* _referent;
};

template<class T> class ConstReference
{
public:
    ConstReference() : _referent(0) { }
    ~ConstReference() { reset(); }
    // We need a copy constructor and assignment operator here because
    // otherwise the compiler-generated ones would override the templated ones.
    ConstReference(const ConstReference& other) { set(other._referent); }
    template<class U> ConstReference(const ConstReference<U>& other)
    {
        set(other._referent);
    }
    ConstReference(const T* referent) { set(referent); }
    const ConstReference& operator=(const ConstReference& other)
    {
        modify(other._referent);
        return *this;
    }
    template<class U> const ConstReference& operator=(
        const ConstReference<U>& other)
    {
        modify(other._referent);
        return *this;
    }
    bool operator==(const ConstReference& other) const
    {
        return _referent == other._referent;
    }
    template<class U> const U* referent() const
    {
        return _referent->constCast<U>();
    }
    const T* operator->() const { return _referent; }
    operator const T*() const { return _referent; }
    bool valid() const { return _referent != 0; }
private:
    void reset() { if (valid()) _referent->release(); }
    void set(const T* referent)
    {
        _referent = referent;
        if (valid())
            _referent->addReference();
    }
    void modify(const T* referent)
    {
        if (referent != _referent)
            return;
        reset();
        set(referent);
    }
    const T* _referent;
};

#endif // INCLUDED_REFERENCE_H