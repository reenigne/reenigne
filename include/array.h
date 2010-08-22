#ifndef INCLUDED_ARRAY_H
#define INCLUDED_ARRAY_H

template<class T> class Array
{
public:
    Array() : _data(0) { }
    void allocate(int n) { release(); _data = new T[n]; }
    ~Array() { release(); }
    T& operator[](int i) { return _data[i]; }
    const T& operator[](int i) const { return _data[i]; }
private:
    void release() { if (_data != 0) delete[] _data; }

    T* _data;
};

#endif // INCLUDED_ARRAY_H
