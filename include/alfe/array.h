#include "alfe/main.h"

#ifndef INCLUDED_ARRAY_H
#define INCLUDED_ARRAY_H

#include "alfe/handle.h"
#include "alfe/body_with_array.h"
#ifdef _MSC_VER
#include <intrin.h>
#endif

template<class T> class List : private Handle
{
public:
    List() { }
    void add(const T& t)
    {
        if (!valid())
            Handle::operator=(Handle(new Body(t)));
        else
            body()->add(t);
    }
    int count() const
    {
        if (valid())
            return body()->count();
        return 0;
    }
private:
    class Body : public Handle::Body
    {
        class Node
        {
        public:
            Node(const T& t) : _value(t), _next(0) { }
            Node* next() const { return _next; }
            void setNext(Node* next) { _next = next; }
            const T& value() const { return _value; }
        private:
            T _value;
            Node* _next;
        };
    public:
        Body(const T& t) : _first(t), _last(&_first), _count(1) { }
        ~Body()
        {
            Node* n = _first.next();
            while (n != 0) {
                Node* nn = n->next();
                delete n;
                n = nn;
            }
        }
        void add(const T& t)
        {
            _last->setNext(new Node(t));
            _last = _last->next();
            ++_count;
        }
        int count() const { return _count; }
        const Node* start() const { return &_first; }
    private:
        Node _first;
        Node* _last;
        int _count;

        friend class List::Iterator;
    };
    Body* body() { return as<Body>(); }
    const Body* body() const { return as<Body>(); }
public:
    class Iterator
    {
    public:
        const T& operator*() const { return _node->value(); }
        const T* operator->() const { return &_node->value(); }
        const Iterator& operator++() { _node = _node->next(); return *this; }
        bool operator==(const Iterator& other) { return _node == other._node; }
        bool operator!=(const Iterator& other) { return !operator==(other); }
        bool end() const { return _node == 0; }
    private:
        const typename Body::Node* _node;

        Iterator(const typename Body::Node* node) : _node(node) { }

        friend class List;
    };
    Iterator begin() const
    {
        if (valid())
            return Iterator(body()->start());
        return end();
    }
    Iterator end() const { return Iterator(0); }
};

template<class T> class AppendableArray;

template<class T> class Array : private Handle
{
    typedef BodyWithArray<Handle::Body, T> Body;
public:
    Array() { }
    Array(const List<T>& list)
    {
        int n = list.count();
        if (n != 0) {
            Handle::operator=(Handle(new Body(n, 0)));
            for (auto p = list.begin(); p != list.end(); ++p)
                body()->constructT(*p);
        }
    }
    explicit Array(int n)
    {
        if (n != 0)
            Handle::operator=(Handle(Body::create(n, n)));
    }
    void allocate(int n) { *this = Array(n); }
    bool operator==(const Array& other) const
    {
        int n = count();
        if (n != other.count())
            return false;
        for (int i = 0; i < n; ++i)
            if ((*this)[i] != other[i])
                return false;
        return true;
    }
    bool operator==(const AppendableArray<T>& other) const
    {
        int n = count();
        if (n != other.count())
            return false;
        for (int i = 0; i < n; ++i)
            if ((*this)[i] != other[i])
                return false;
        return true;
    }
    bool operator!=(const Array& other) const { return !operator==(other); }
    bool operator!=(const AppendableArray<T>& other) const
    {
        return !operator==(other);
    }
    T& operator[](int i) { return (*body())[i]; }
    const T& operator[](int i) const { return (*body())[i]; }
    int count() const { return body()->size(); }
    Array copy() const
    {
        Array r(new Body(count(), 0));
        for (int i = 0; i < count(); ++i)
            r.body()->constructT((*this)[i]);
        return r;
    }

    typedef typename Body::Iterator Iterator;
    Iterator begin() const
    {
        if (body() != 0)
            return body()->begin();
        return Body::Iterator();
    }
    Iterator end() const
    {
        if (body() != 0)
            return body()->end();
        return Body::Iterator();
    }

private:
    Body* body() { return as<Body>(); }
};

template<class T> class AppendableArray : private Handle
{
    class BaseBody : public Handle::Body
    {
    public:
        int _allocated;
    };
    typedef BodyWithArray<BaseBody, T> Body;
private:
    static int roundUpToPowerOf2(int n)
    {
#ifdef _MSC_VER
        unsigned long k;
        _BitScanReverse(&k, n);
        if ((n & (n - 1)) != 0)
            ++k;
        return 1 << k;
#elif defined __GNUC__
        int k = (sizeof(int)*8 - 1) - __builtin_clz(n);
        if ((n & (n - 1)) != 0)
            ++k;
        return 1 << k;
#else
        --n;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        return n + 1;
#endif
    }
public:
    AppendableArray() { }
    AppendableArray(const List<T>& list) : AppendableArray(list.count())
    {
        for (auto p = list.begin(); p != list.end(); ++p)
            body()->constructT(*p);
    }
    explicit AppendableArray(int n)
    {
        // The 8 bytes is the observed malloc overhead on both GCC and VC,
        // 32-bit and 64-bit (though not VC with debug heap). The idea is that
        // we allocate actual memory blocks that are powers of 2 bytes to
        // minimize fragmentation, and allocate as many objects as we can in
        // that space so as to make the best use of it.
        int overhead = Body::headSize() + 8;
        int s = n*sizeof(T) + overhead;
        s = roundUpToPowerOf2(s) - overhead;
        int count = s/sizeof(T);
        int extra = s%sizeof(T);
        auto b = Body::create(count, 0, extra);
        b->_allocated = count;
        Handle::operator=(Handle(b));
    }
    void allocate(int n)
    {
        if (allocated() < n) {
            AppendableArray a(n);
            if (count() > 0)
                a.addUnchecked(body()->pointer(), count());
            *this = a;
        }
    }
    void append(const T& value) { append(&value, 1); }
    void append(const Array<T>& other)
    {
        if (other.count() > 0)
            append(&other[0], other.count());
    }
    void append(const AppendableArray<T>& other)
    {
        if (other.count() > 0)
            append(&other[0], other.count());
    }
    void append(const T* data, int length)
    {
        if (allocated() < count() + length) {
            AppendableArray a(count() + length);
            if (count() > 0)
                a.addUnchecked(body()->pointer(), count());
            a.addUnchecked(data, length);
            *this = a;
        }
        else {
            int oldCount = count();
            try {
                addUnchecked(data, length);
            }
            catch (...) {
                body()->setSize(oldCount);
                throw;
            }
        }
    }
    // Like append but with default construction instead of copying.
    void expand(int length)
    {
        if (allocated() < count() + length) {
            AppendableArray a(count() + length);
            if (count() > 0)
                a.addUnchecked(body()->pointer(), count());
            a.expandUnchecked(length);
            *this = a;
        }
        else {
            int oldCount = count();
            try {
                expandUnchecked(length);
            }
            catch (...) {
                body()->setSize(oldCount);
                throw;
            }
        }
    }
    void clear()
    {
        if (count() > 0)
            body()->setSize(0);
    }

    bool operator==(const Array<T>& other) const
    {
        int n = count();
        if (n != other.count())
            return false;
        for (int i = 0; i < n; ++i)
            if ((*this)[i] != other[i])
                return false;
        return true;
    }
    bool operator==(const AppendableArray<T>& other) const
    {
        int n = count();
        if (n != other.count())
            return false;
        for (int i = 0; i < n; ++i)
            if ((*this)[i] != other[i])
                return false;
        return true;
    }

    bool operator!=(const Array<T>& other) const { return !operator==(other); }
    bool operator!=(const AppendableArray<T>& other) const
    {
        return !operator==(other);
    }
    T& operator[](int i) { return (*body())[i]; }
    const T& operator[](int i) const { return (*body())[i]; }
    int count() const
    {
        if (body() == 0)
            return 0;
        return body()->size();
    }
    int allocated() const
    {
        if (body() == 0)
            return 0;
        return body()->_allocated;
    }
    AppendableArray copy() const
    {
        AppendableArray r(count());
        r.addUnchecked(&(*this)[0], count());
        return r;
    }

    typedef typename Body::Iterator Iterator;
    Iterator begin() const
    {
        if (body() != 0)
            return body()->begin();
        return Body::Iterator();
    }
    Iterator end() const
    {
        if (body() != 0)
            return body()->end();
        return Body::Iterator();
    }

private:
    void addUnchecked(const T* start, int c)
    {
        for (int i = 0; i < c; ++i) {
            body()->constructT(*start);
            ++start;
        }
    }
    void expandUnchecked(int c)
    {
        for (int i = 0; i < c; ++i)
            body()->constructT(T());
    }
    Body* body() { return as<Body>(); }
    const Body* body() const { return as<Body>(); }
};

#endif // INCLUDED_ARRAY_H
