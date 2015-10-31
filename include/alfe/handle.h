#include "alfe/main.h"

#ifndef INCLUDED_HANDLE_H
#define INCLUDED_HANDLE_H

class ConstHandle
{
public:
    ConstHandle() : _body(0) { }
    ~ConstHandle() { reset(); }
    ConstHandle(const ConstHandle& other) { set(other._body); }
    const ConstHandle& operator=(const ConstHandle& other)
    {
        modify(other._body);
        return *this;
    }
    bool valid() const { return _body != 0; }
    //void swap(ConstHandle& other)
    //{
    //    const Body* t = _body;
    //    _body = other._body;
    //    other._body = t;
    //}
    UInt32 hash() const { return _body->hash(); }
    bool operator==(const ConstHandle& other) const
    {
        return _body->equals(other._body);
    }
    bool operator!=(const ConstHandle& other) const
    {
        return !(*this == other);
    }

protected:
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
        virtual void destroy() const { delete this; }  // for Array::Body
        virtual Hash hash() const { return typeid(*this); }
        virtual bool equals(const Body* other) const
        {
            return typeid(*this) == typeid(*other);
        }
    private:
        void release() const
        {
            --_count;
            if (_count == 0)
                destroy();
        }
        void acquire() const { ++_count; }
        mutable int _count;
        friend class ConstHandle;
    };

    template<class T> const T* as() const { return _body->as<T>(); }
    template<class T> bool is() const { return as<T::Body>() != 0; }
    const Body* body() const { return _body; }
    ConstHandle(const Body* body) { set(body); }
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

    friend class Handle;
};

class Handle : private ConstHandle
{
public:
    Handle() { }
    Handle(const Handle& other) : ConstHandle(other) { }
    const Handle& operator=(const Handle& other)
    {
        ConstHandle::operator=(other);
        return *this;
    }
    bool valid() const { return ConstHandle::valid(); }
    UInt32 hash() const { return ConstHandle::hash(); }
    bool operator==(const Handle& other) const
    {
        return ConstHandle::operator==(other);
    }
    bool operator!=(const ConstHandle& other) const
    {
        return ConstHandle::operator!=(other);
    }
protected:
    typedef ConstHandle::Body Body;
    Handle(Body* body) { set(body); }
    Body* body() { return const_cast<Body*>(_body); }
    const Body* body() const { return ConstHandle::body(); }
    template<class T> const T* as() const { return ConstHandle::as<T>(); }
    template<class T> T* as() { return body()->as<T>(); }
    template<class T> bool is() const { return ConstHandle::is<T>; }
};

#endif // INCLUDED_HANDLE_H
