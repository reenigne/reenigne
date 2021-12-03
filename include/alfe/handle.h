#include "alfe/main.h"

#ifndef INCLUDED_HANDLE_H
#define INCLUDED_HANDLE_H

class HandleBase
{
public:
    HandleBase() : _body(0) { }
    HandleBase(const HandleBase& other) { _body = other._body; }
    bool valid() const { return _body != 0; }
    UInt32 hash() const { return _body != 0 ? _body->hash() : 0; }
    bool operator==(const HandleBase& other) const
    {
        if (_body == 0)
            return other._body == 0;
        if (other._body == 0)
            return false;
        return _body == other._body || _body->equals(other._body);
    }
    bool operator!=(const HandleBase& other) const
    {
        return !(*this == other);
    }
    template<class B, typename... Args> static HandleBase
        create(Args&&... args)
    {
        return HandleBase(new B(std::forward<Args>(args)...));
    }
    void destroy() const { _body->destroy(); }
protected:
    class Body : public Uncopyable
    {
    public:
        template<class T> T* as() { return static_cast<T*>(this); }
        template<class T> T* to() { return dynamic_cast<T*>(this); }
        template<class T> const T* asConst() const
        {
            return static_cast<const T*>(this);
        }
        template<class T> const T* toConst() const
        {
            return dynamic_cast<const T*>(this);
        }
    protected:
        virtual ~Body() { };
        virtual void preDestroy() { }  // for HashTable::Body
        virtual void destroy() { delete this; }
        virtual Hash hash() { return typeid(*this); }
        virtual bool equals(Body* other) { return false; }
        template<class T> T handle() { return T(HandleBase(this)); }

        friend class HandleBase;
    };

    template<class B> static HandleBase to(const HandleBase& other)
    {
        HandleBase r;
        r._body = other.to<B>();
        return r;
    }
    template<class T> T* as() const { return _body->as<T>(); }
    template<class T> T* to() const { return _body->to<T>(); }
private:
    Body* _body;

    HandleBase(Body* body) : _body(body) { }

    // Array needs to be able to construct from a Body*.
    template<class T> friend class Array;
};

class ConstHandleBase
{
public:
    ConstHandleBase() : _body(0) { }
    ConstHandleBase(const ConstHandleBase& other) { _body = other._body; }
    bool valid() const { return _body != 0; }
    UInt32 hash() const { return _body != 0 ? _body->hash() : 0; }
    bool operator==(const ConstHandleBase& other) const
    {
        if (_body == 0)
            return other._body == 0;
        if (other._body == 0)
            return false;
        return _body == other._body || _body->equals(other._body);
    }
    bool operator!=(const ConstHandleBase& other) const
    {
        return !(*this == other);
    }
    template<class B, typename... Args> static ConstHandleBase
        create(Args&&... args)
    {
        return ConstHandleBase(new B(std::forward<Args>(args)...));
    }
    void destroy() const { _body->destroy(); }
protected:
    class Body : Uncopyable
    {
    public:
        template<class T> const T* as() const
        {
            return static_cast<const T*>(this);
        }
        template<class T> const T* to() const
        {
            return dynamic_cast<const T*>(this);
        }
    protected:
        virtual ~Body() { };
        virtual void destroy() const { delete this; }
        virtual Hash hash() const { return typeid(*this); }
        virtual bool equals(const Body* other) const { return false; }
        template<class T> T handle() const { return T(ConstHandleBase(this)); }

        friend class ConstHandleBase;
    };

    template<class B> static ConstHandleBase to(const ConstHandleBase& other)
    {
        ConstHandleBase r;
        r._body = other.to<B>();
        return r;
    }
    template<class T> const T* as() const { return _body->as<T>(); }
    template<class T> const T* to() const { return _body->to<T>(); }
private:
    const Body* _body;

    ConstHandleBase(const Body* body) { _body = body; }
};

// Base classes for reference-counted objects. Unlike std::shared_ptr, these
// are not thread-safe, so you must do your own locking if you want to copy
// and destruct handles from separate threads.

template<class Base = HandleBase> class ReferenceCounted : public Base
{
public:
    ReferenceCounted() { }
    ~ReferenceCounted() { reset(); }
    ReferenceCounted(const Base& other) { set(other); }
    ReferenceCounted(const ReferenceCounted& other) { set(other); }
    const ReferenceCounted& operator=(const ReferenceCounted& other)
    {
        modify(other);
        return *this;
    }
    template<class B, typename... Args> static ReferenceCounted
        create(Args&&... args)
    {
        return ReferenceCounted(Base::template create<B, Args...>(
            std::forward<Args>(args)...), false);
    }
    void destroy() { }
    template<class B> static ReferenceCounted to(const ReferenceCounted& other)
    {
        return ReferenceCounted(Base::template to<B>(other));
    }
    template<class T> auto /*const T**/ to() const
    {
        return Base::template to<T>();
    }
    template<class T> auto /*const T**/ as() const
    {
        return Base::template as<T>();
    }
    bool valid() const { return Base::valid(); }

protected:
    class Body : public Base::Body
    {
    public:
        // A Body always start off with count 1, so that if handle() is called
        // from the child constructor, the Body won't be deleted when that
        // handle goes away. The owner during construction is the constructor
        // itself, since that will delete the Body if an exception is thrown.
        Body() : _count(1) { }
    protected:
        template<class T> T handle()
        {
            return T(ReferenceCounted(Base::Body::template handle<T>(), true));
        }
        template<class T> T constHandle() const
        {
            return T(ReferenceCounted(Base::Body::template handle<T>(), true));
        }
    private:
        void release() const
        {
            --_count;
            if (_count == 0)
                const_cast<Body*>(this)->destroy();
        }
        void acquire() const { ++_count; }
        mutable int _count;
        friend class ReferenceCounted;
    };
private:
    auto body() const { return as<Body>(); }
    void reset() { if (valid()) body()->release(); }
    // acquire should only be false when creating
    void set(Base base, bool acquire = true)
    {
        Base::operator=(base);
        if (acquire && valid())
            body()->acquire();
    }
    void modify(const ReferenceCounted& other)
    {
        if (body() == other.body())
            return;
        reset();
        set(other);
    }
    ReferenceCounted(Base base, bool acquire) { set(base, acquire); }

    // Array needs its own create<> which needs to call the 2-parameter
    // constructor.
    template<class T> friend class Array;
};

typedef ReferenceCounted<ConstHandleBase> ConstHandle;
typedef ReferenceCounted<HandleBase> Handle;

#endif // INCLUDED_HANDLE_H
