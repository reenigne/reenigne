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
    UInt32 hash() const { return _body != 0 ? _body->hash() : 0; }
    bool operator==(const ConstHandle& other) const
    {
        if (_body == 0)
            return other._body == 0;
        if (other._body == 0)
            return false;
        return _body == other._body || _body->equals(other._body);
    }
    bool operator!=(const ConstHandle& other) const
    {
        return !(*this == other);
    }
    template<class B, typename... Args> static ConstHandle
        create(Args&&... args)
    {
        return ConstHandle(new B(std::forward<Args>(args)...), false);
    }

protected:
    class Body : Uncopyable
    {
    public:
        // A Body always start off with count 1, so that if handle() is called
        // from the child constructor, the Body won't be deleted when that
        // handle goes away. The owner during construction is the constructor
        // itself, since that will delete the Body if an exception is thrown.
        Body() : _count(1) { }
        template<class T> T* as() { return dynamic_cast<T*>(this); }
        template<class T> const T* as() const
        {
            return dynamic_cast<const T*>(this);
        }
    protected:
        virtual ~Body() { };
        virtual void preDestroy() const { }  // for HashTable::Body
        virtual void destroy() const { delete this; }  // for Array::Body
        virtual Hash hash() const { return typeid(*this); }
        virtual bool equals(const Body* other) const { return false; }
        template<class T> T handle() const
        {
            return T(ConstHandle(this, true));
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
private:
    void reset() { if (valid()) _body->release(); }
    void set(const Body* body, bool acquire = true)
    {
        _body = body;
        if (acquire && valid())
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
    ConstHandle(const Body* body, bool acquire) { set(body, acquire); }

    friend class Handle;
    template<class T> friend class Array;
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
    template<class B, typename... Args> static Handle create(Args&&... args)
    {
        return Handle(new B(std::forward<Args>(args)...), false);
    }
protected:
    class Body : public ConstHandle::Body
    {
    protected:
        template<class T> T handle() { return T(ConstHandle(this, true)); }
        virtual bool equals(const Body* other) const { return false; }
    private:
        bool equals(const ConstHandle::Body* other) const
        {
            return equals(static_cast<const Body*>(other));
        }
    };
    Body* body() { return const_cast<ConstHandle::Body*>(_body)->as<Body>(); }
    template<class T> const T* as() const { return ConstHandle::as<T>(); }
    template<class T> T* as() { return body()->as<T>(); }
private:
    Handle(Body* body, bool acquire) { set(body, acquire); }
    friend class ConstHandle::Body;
    template<class T> friend class Array;
};

#endif // INCLUDED_HANDLE_H
