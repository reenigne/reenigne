#ifndef INCLUDED_ARRAY_H
#define INCLUDED_ARRAY_H

#include <new>

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
        const Iterator& operator++() { _node = _node->next(); return *this; }
        bool operator==(const Iterator& other) { return _node == other._node; }
        bool operator!=(const Iterator& other) { return !operator==(other); }
    private:
        const typename Implementation::Node* _node;

        Iterator(const typename Implementation::Node* node) : _node(node) { }

        friend class List;
    };
    Iterator start() const
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
    Array() : _data(0) { }
    Array(const List<T>& list) : _data(0)
    {
        allocate(list.count());
        int i = 0;
        for (List<T>::Iterator p = list.start(); p != list.end(); ++p) {
            constructElement(i, *p);
            ++i;
        }
    }
    explicit Array(int n) : _data(0) { allocate(n); }
    void allocate(int n)
    {
        release();
        if (n != 0)
            _data = static_cast<T*>(operator new (n * sizeof(T)));
        _n = n;
    }
    void constructElements(const T& initializer)
    {
        for (int i = 0; i < _n; ++i)
            constructElement(i, initializer);
    }
    void constructElements()
    {
        for (int i = 0; i < _n; ++i)
            constructElement(i);
    }
    void constructElement(int i, const T& initializer)
    {
        new(static_cast<void*>(&(*this)[i])) T(initializer);
    }
    void constructElement(int i)
    {
        new(static_cast<void*>(&(*this)[i])) T();
    }
    void destructElements()
    {
        for (int i = 0; i < _n; ++i)
            (&(*this)[i])->~T();
    }
    ~Array() { release(); }
    T& operator[](int i) { return _data[i]; }
    const T& operator[](int i) const { return _data[i]; }
    int count() const { return _n; }
    void swap(Array<T>& other)
    {
        T* d = other._data;
        other._data = _data;
        _data = d;
        int n = other._n;
        other._n = _n;
        _n = n;
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
private:
    void release() { if (_data != 0) operator delete(_data); _data = 0; }

    T* _data;
    int _n;
};

template<class T> class AppendableArray : Uncopyable
{
public:
    AppendableArray() : _n(0)
    {
        _array.allocate(1);
    }
    ~AppendableArray()
    {
        for (int i = 0; i < _n; ++i)
            (&(*this)[i])->~T();
    }
    T& operator[](int i) { return _array[i]; }
    const T& operator[](int i) const { return _array[i]; }
    int count() const { return _n; }
    bool operator==(const AppendableArray& other) const
    {
        if (_n != other._n)
            return false;
        for (int i = 0; i < _n; ++i)
            if ((*this)[i] != other[i])
                return false;
        return true;
    }
    bool operator!=(const AppendableArray& other) const
    {
        return !operator==(other);
    }
    void append(const T& value)
    {
        if (_n == _array.count()) {
            Array<T> n;
            n.allocate(_n*2);
            n.swap(_array);
            for (int i = 0; i < _n; ++i)
                new(static_cast<void*>(&(*this)[i])) T(n[i]);
        }
        new(static_cast<void*>(&(*this)[_n])) T(value);
        ++_n;
    }
private:
    Array<T> _array;
    int _n;
};

template<class T> class PrependableArray : Uncopyable
{
public:
    PrependableArray() : _n(0)
    {
        _array.allocate(1);
    }
    ~PrependableArray()
    {
        for (int i = 0; i < _n; ++i)
            (&(*this)[i])->~T();
    }
    T& operator[](int i) { return _array[i + offset()]; }
    const T& operator[](int i) const { return _array[i + offset()]; }
    int count() const { return _n; }
    bool operator==(const PrependableArray& other) const
    {
        if (_n != other._n)
            return false;
        for (int i = 0; i < _n; ++i)
            if ((*this)[i] != other[i])
                return false;
        return true;
    }
    bool operator!=(const PrependableArray& other) const
    {
        return !operator==(other);
    }
    void prepend(const T& value)
    {
        if (_n == _array.count()) {
            Array<T> n;
            n.allocate(_n*2);
            n.swap(_array);
            for (int i = 0; i < _n; ++i)
                new(static_cast<void*>(&(*this)[i])) T(n[i]);
        }
        ++_n;
        new (static_cast<void*>(&(*this)[0])) T(value);
    }
private:
    int offset() const { return _array.count() - _n; }
    Array<T> _array;
    int _n;

    friend class Array<T>;
};

#endif // INCLUDED_ARRAY_H
