#include "alfe/main.h"

#ifndef INCLUDED_ARRAY_H
#define INCLUDED_ARRAY_H

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
    Array(const List<T>& list)
    {
        int n = list.count();
        _data = static_cast<T*>(operator new(n * sizeof(T)));
        _n = 0;
        try {
            for (auto p = list.begin(); p != list.end(); ++p) {
                constructElement(_n, *p);
                ++_n;
            }
        }
        catch (...) {
            destructElements();
            throw;
        }
    }
    explicit Array(int n)
    {
        _data = static_cast<T*>(operator new(n * sizeof(T)));
        try {
            for (_n = 0; _n < n; ++_n)
                constructElement(_n);
        }
        catch (...) {
            destructElements();
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
        ::swap(_data, other._data);
        ::swap(_n, other._n);
    }
    ~Array() { release(); }
    T& operator[](int i) { return _data[i]; }
    const T& operator[](int i) const { return _data[i]; }
    int count() const { return _n; }

    class Iterator
    {
    public:
        const T& operator*() const { return *_p; }
        const T* operator->() const { return _p; }
        const Iterator& operator++() { ++_p; return *this; }
        bool operator==(const Iterator& other) { return _p == other._p; }
        bool operator!=(const Iterator& other) { return !operator==(other); }
    private:
        const T* _p;

        Iterator(const T* p) : _p(p) { }

        friend class Array;
    };

    Iterator begin() const { return Iterator(_data); }
    Iterator end() const { return Iteraotr(_data + _n); }

private:
    Array(const Array& other, int allocated)
    {
        _data = static_cast<T*>(operator new(allocated * sizeof(T)));
        try {
            for (_n = 0; _n < other._n; ++_n)
                constructElement(_n, other[_n]);
        }
        catch (...) {
            destructElements();
            throw;
        }
    }
    void constructElement(int i, const T& initializer)
    {
        new(static_cast<void*>(&(*this)[i])) T(initializer);
    }
    void destructElement(int i)
    {
        (&(*this)[i])->~T();
    }
    void constructElement(int i)
    {
        new(static_cast<void*>(&(*this)[i])) T();
    }

    void release()
    {
        if (_data != 0) {
            for (int i = _n - 1; i >= 0; --i)
                destructElement(i);
            operator delete(_data);
        }
        _data = 0;
    }
    void destructElements()
    {
        while (_n > 0) {
            --_n;
            destructElement(_n);
        }
    }

    T* _data;
    int _n;

    template<class T> friend class AppendableArray;
};

template<class T> class AppendableArray : public Array<T>
{
public:
    AppendableArray() : _allocated(0) { }
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
            int allocate = _allocated *2;
            if (allocate == 0)
                allocate = 1;
            Array<T> n(*this, allocate);
            Array::swap(n);
            _allocated = allocate;
        }
        constructElement(_n, value);
        ++_n;
    }
    void trim()
    {
        if (_allocated != _n) {
            Array<T> n(*this, _n);
            Array::swap(n);
            _allocated = _n;
        }
    }
    void reserve(int size)
    {
        int allocate = _allocated;
        if (allocate == 0 && size > 0)
            allocate = 1;
        while (allocate < size)
            allocate *= 2;
        if (_allocated < allocate) {
            Array<T> n(*this, allocate);
            Array::swap(n);
            _allocated = allocate;
        }
    }
    void append(const Array& other)
    {
        reserve(_n + other._n);
        int i;
        try {
            for (i = 0; i < other._n; ++i)
                constructElement(_n + i, other[i]);
        }
        catch (...) {
            destructElements(i);
            throw;
        }
        _n += other._n;
    }
    void append(const T* data, int length)
    {
        reserve(_n + length);
        int i;
        try {
            for (i = 0; i < length; ++i) {
                constructElement(_n + i, *data);
                ++data;
            }
        }
        catch (...) {
            destructElements(i);
            throw;
        }
        _n += length;
    }
    // Like append(const Array& other) but with default construction instead of
    // copying.
    void expand(int length)
    {
        reserve(_n + length);
        int i;
        try {
            for (i = 0; i < length; ++i)
                constructElement(_n + i);
        }
        catch (...) {
            destructElements(i);
            throw;
        }
        _n += length;
    }
    void clear()
    {
        int n = _n;
        _n = 0;
        destructElements(n);
    }

    Iterator end() const { return Iterator(_data + _allocated); }
private:
    void destructElements(int n)
    {
        while (n > 0) {
            --n;
            destructElement(_n + n);
        }
    }

    int _allocated;
};

#endif // INCLUDED_ARRAY_H
