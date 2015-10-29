#include "alfe/main.h"

#ifndef INCLUDED_HANDLE_H
#define INCLUDED_HANDLE_H

class Handle
{
public:
    class Body : Uncopyable
    {
    public:
        Body() : _count(0) { }
        template<class T> T* as() { return dynamic_cast<T*>(this); }
        template<class T> const T* as() const
        {
            return dynamic_cast<const T*>(this);
        }
    protected:
        virtual ~Body() { };
        virtual void destroy() const { delete this; }  // for BodyWithArray
    private:
        void release() const
        {
            --_count;
            if (_count == 0)
                destroy();
        }
        void acquire() const { ++_count; }
        mutable int _count;
        friend class Handle;
        friend class ConstHandle;
    };

    Handle() : _body(0) { }
    ~Handle() { reset(); }
    Handle(const Handle& other) { set(other._body); }
    Handle(Body* body) { set(body); }
    const Handle& operator=(const Handle& other)
    {
        modify(other._body);
        return *this;
    }
    bool valid() const { return _body != 0; }
    void swap(Handle& other)
    {
        Body* t = _body;
        _body = other._body;
        other._body = t;
    }

    template<class T> T* as() { return _body->as<T>(); }
    template<class T> const T* as() const { return _body->as<T>(); }
    template<class T> bool is() const { return as<T::Body>() != 0; }
protected:
    Body* body() { return _body; }
    const Body* body() const { return _body; }
private:
    void reset() { if (valid()) _body->release(); }
    void set(Body* body)
    {
        _body = body;
        if (valid())
            _body->acquire();
    }
    void modify(Body* body)
    {
        if (body == _body)
            return;
        reset();
        set(body);
    }
    Body* _body;                                                               

    friend class ConstHandle;
};

class ConstHandle
{
public:
    typedef Handle::Body Body;

    ConstHandle() : _body(0) { }
    ~ConstHandle() { reset(); }
    // We need a copy constructor and assignment operator here because
    // otherwise the compiler-generated ones would override the templated ones.
    ConstHandle(const ConstHandle& other) { set(other._body); }
    ConstHandle(const Handle& other) { set(other._body); }
    ConstHandle(const Body* body) { set(body); }
    const ConstHandle& operator=(const ConstHandle& other)
    {
        modify(other._body);
        return *this;
    }
    const ConstHandle& operator=(const Handle& other)
    {
        modify(other._body);
        return *this;
    }
    bool valid() const { return _body != 0; }
    void swap(ConstHandle& other)
    {
        const Body* t = _body;
        _body = other._body;
        other._body = t;
    }

    template<class T> const T* as() const { return _body->as<T>(); }
    template<class T> bool is() const { return as<T::Body>() != 0; }
protected:
    const Body* body() const { return _body; }
private:
    void reset() { if (valid()) _body->release(); }
    void set(const Body* body)
    {
        _body = body;
        if (valid())
            _body->acquire();
    }
    void modify(const Body* body)
    {
        if (body == _body)
            return;
        reset();
        set(body);
    }
    const Body* _body;
};

#endif // INCLUDED_HANDLE_H
