#ifndef INCLUDED_ARRAY_H
#define INCLUDED_ARRAY_H

#include <new>

template<class T> class Array : Uncopyable
{
public:
    Array() : _data(0) { }
    explicit Array(int n) : _data(0) { allocate(n); }
    void allocate(int n)
    {
        release();
        if (n != 0)
            _data = static_cast<T*>(operator new (n * sizeof(T)));
        _n = n;
    }
    void constructElements(const T& initializer = T())
    {
        for (int i = 0; i < _n; ++i)
            new(static_cast<void*>(&(*this)[i])) T(initializer);
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
