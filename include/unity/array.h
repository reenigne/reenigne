#ifndef INCLUDED_ARRAY_H
#define INCLUDED_ARRAY_H

template<class T> class Array : Uncopyable
{
public:
    Array() : _data(0) { }
    void allocate(int n)
    { 
        release();
        if (n != 0)
            _data = new T[n];
        _n = n;
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
    void release() { if (_data != 0) delete[] _data; _data = 0; }

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
    T& operator[](int i) { return _array[i]; }
    const T& operator[](int i) const { return _array[i]; }
    int count() const { return _n; }
    bool operator==(const AppendableArray& other) const
    {
        if (_n != other._n)
            return false;
        for (int i = 0; i < _n; ++i)
            if (_array[i] != other._array[i])
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
                _array[i] = n[i];
        }
        _array[_n] = value;
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
                _array[i + offset()] = n[i];
        }
        ++_n;
        (*this)[0] = value;
    }
private:
    int offset() const { return _array.count() - _n; }
    Array<T> _array;
    int _n;

    friend class Array<T>;
};

#endif // INCLUDED_ARRAY_H