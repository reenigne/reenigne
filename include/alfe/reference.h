#include "alfe/main.h"

#ifndef INCLUDED_REFERENCE_H
#define INCLUDED_REFERENCE_H

class Handle
{
public:
    Handle() : _body(0) { }
    ~Handle() { reset(); }
    // We need a copy constructor and assignment operator here because
    // otherwise the compiler-generated ones would override the templated ones.
    Handle(const Handle& other) { set(other._body); }
    template<class U> Handle(const Handle<U>& other)
    {
        set(other._body);
    }
    Handle(const T* body) { set(body); }
    const Handle& operator=(const Handle& other)
    {
        modify(other._body);
        return *this;
    }
    template<class U> const Handle& operator=(
        const Handle<U>& other)
    {
        modify(other._body);
        return *this;
    }
    bool operator==(const Handle& other) const
    {
        return _body == other._body;
    }
    template<class U> const U* body() const
    {
        return _body->template constCast<U>();
    }
    const T* operator->() const { return _body; }
    operator const T*() const { return _body; }
    bool valid() const { return _body != 0; }
    template<class U> bool is() const { return body<U>() != 0; }

    class Body : Uncopyable
    {
    public:
        Body() : _count(0) { }

        template<class T> T* cast() const { return dynamic_cast<T*>(this); }
        template<class T> const T* constCast() const
        {
            return dynamic_cast<const T*>(this);
        }
    protected:
        virtual ~Body() { };
        virtual void destroy() const { delete this; }
    private:
        void release() const
        {
            --_count;
            if (_count == 0)
                destroy();
        }
        void addReference() const { ++_count; }
        mutable int _count;
        template<class T> friend class Reference;
        template<class T> friend class Handle;
    };
private:
    void reset() { if (valid()) _body->release(); }
    void set(const T* body)
    {
        _body = body;
        if (valid())
            _body->addReference();
    }
    void modify(const T* body)
    {
        if (body == _body)
            return;
        reset();
        set(body);
    }
    const T* _body;

    template<class U> friend class Handle;
};

#endif // INCLUDED_REFERENCE_H
