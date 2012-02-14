#ifndef INCLUDED_ARRAY_H
#define INCLUDED_ARRAY_H

#include <new>
#include "unity/swap.h"

template<class T> class List
{
public:
    List() { }
    void add(const T& t)
    {
        if (!_implementation.valid())
            _implementation = new Implementation(t);
        else
            _implementation->add(t);
    }
    int count() const
    {
        if (_implementation.valid())
            return _implementation->count();
        return 0;
    }
private:
    class Implementation : public ReferenceCounted
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
        Implementation(const T& t) : _first(t), _last(&_first), _count(1) { }
        ~Implementation()
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

        friend class Iterator;
    };
public:
    class Iterator
    {
    public:
        const T& operator*() const { return _node->value(); }
        const T* operator->() const { return &_node->value(); }
        const Iterator& operator++() { _node = _node->next(); return *this; }
        bool operator==(const Iterator& other) { return _node == other._node; }
        bool operator!=(const Iterator& other) { return !operator==(other); }
    private:
        const typename Implementation::Node* _node;

        Iterator(const typename Implementation::Node* node) : _node(node) { }

        friend class List;
    };
    Iterator begin() const
    {
        if (_implementation.valid())
            return Iterator(_implementation->start());
        return end();
    }
    Iterator end() const { return Iterator(0); }

    Reference<Implementation> _implementation;
};

template<class T> class Array : Uncopyable
{
public:
    Array() : _data(0), _n(0) { }
    Array(const List<T>& list) : _data(0), _n(0)
    {
        _n = list.count();
        _data = static_cast<T*>(operator new (_n * sizeof(T)));
        int i = 0;
        try {
            for (auto p = list.begin(); p != list.end(); ++p) {
                constructElement(i, *p);
                ++i;
            }
        }
        catch (...) {
            while (i > 0) {
                --i;
                destructElement(i);
            }
            throw;
        }
    }
    explicit Array(int n)
    {
        _n = n;
        _data = static_cast<T*>(operator new (_n * sizeof(T)));
        int i = 0;
        try {
            for (i = 0; i < n; ++i) {
                constructElement(i);
                ++i;
            }
        }
        catch (...) {
            while (i > 0) {
                --i;
                destructElement(i);
            }
            throw;
        }
    }
    void allocate(int n)
    {
        Array<T> other(n);
        swap(other);
    }
    bool operator==(const Array& other) const
    {
        if (_n != other._n)
            return false;
        for (int i = 0; i < _n; ++i)
            if (_data[i] != other._data[i])
                return false;
        return true;
    }
    bool operator!=(const Array& other) const
    {
        return !operator==(other);
    }
    void swap(Array& other)
    {
        swap(_data, other._data);
        swap(_n, other._n);
    }
    ~Array() { release(); }
    T& operator[](int i) { return _data[i]; }
    const T& operator[](int i) const { return _data[i]; }
    int count() const { return _n; }
protected:
    Array(const Array& other, int allocated)
    {
        _data = static_cast<T*>(operator new(allocated * sizeof(T));
        int i;
        try {
            for (int i = 0; i < other._n; ++i)
                constructElement(i, other[i]);
        }
        catch (...) {
            while (i > 0) {
                --i;
                destructElement(i);
            }
            throw;
        }
    }
    void constructElement(int i, const T& initializer)
    {
        new(static_cast<void*>(&(*this)[i])) T(initializer);
    }

    int _n;
private:
    void release()
    {
        if (_data != 0) {
            for (int i = _n - 1; i >= 0; --i)
                destructElement(i);
            operator delete(_data);
        }
        _data = 0;
    }
    void constructElement(int i)
    {
        new(static_cast<void*>(&(*this)[i])) T();
    }
    void destructElement(int i)
    {
        (&(*this)[i])->~T();
    }

    T* _data;
};

template<class T> class AppendableArray : public Array
{
public:
    AppendableArray(), _allocated(0) { }
    AppendableArray(const List<T>& list)
      : Array(list), _allocated(list.count()) { }
    explicit AppendableArray(int n) : Array(n), _allocated(n) { }
    void swap(AppendableArray& other)
    {
        Array::swap(other);
        swap(_allocated, other._allocated);
    }
    void append(const T& value)
    {
        if (_allocated == _n) {
            Array<T> n(*this, _allocated*2);
            Array::swap(n);
            _allocated *= 2;
        }
        constructElement(_n, value);
        ++_n;
    }
private:
    int _allocated;
};

#endif // INCLUDED_ARRAY_H
