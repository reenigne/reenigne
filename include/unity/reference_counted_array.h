#ifndef INCLUDED_REFERENCE_COUNTED_ARRAY_H
#define INCLUDED_REFERENCE_COUNTED_ARRAY_H

#include "unity/reference_counted.h"
#include "unity/integer_types.h"

// This class combines an H and an array of Ts in a single memory block. H
// must inherit from ReferenceCountedArray<H, T> (curiously recurring template
// pattern). H must have alignment equal to or greater than T. This can be
// achieved generically by making H have a member of type T. The resulting
// object can be used with the Reference and ConstReference classes.
template<class H, class T> class ReferenceCountedArray
{
public:
    static ReferenceCountedArray* create(int size)
    {
        void* buffer =
            static_cast<void*>(operator new(sizeof(H) + size*sizeof(T)));
        try {
            H* h = new(buffer) H();
            h->_size = size;
            int i = 0;
            try {
                for (i = 0; i < size; ++i)
                    new(static_cast<void*>(&(*h)[i])) T();
            }
            catch (...) {
                while (i > 0) {
                    --i;
                    (&(*h)[i])->~T();
                }
                h->~H();
                throw;
            }
        }
        catch (...) {
            operator delete(buffer);
            throw;
        }
    }

    void addReference() const { ++_count; }

    template<class U> U* cast() { return dynamic_cast<U*>(this); }
protected:
    virtual ~ReferenceCountedArray() { };

    ReferenceCountedArray() : _count(0) { }

    T& operator[] (int i)
    {
        return static_cast<T*>(
            static_cast<Byte*>(static_cast<void*>(this)) + sizeof(H))[i];
    }
    const T& operator[] (int i) const
    {
        return static_cast<const T*>(
            static_cast<const Byte*>(static_cast<const void*>(this)) +
            sizeof(H))[i];
    }

    const int _size;
private:
    void release() const
    {
        --_count;
        if (_count == 0) {
            for (int i = _size - 1; i >= 0; --i)
                (&(*this)[i])->~T();
            static_cast<const H*>(this)->~H();
            operator delete(static_cast<const void*>(this));
        }
    }

    mutable int _count;

    template<class T> friend class Reference;
    template<class T> friend class ConstReference;
};

#endif // INCLUDED_REFERENCE_COUNTED_ARRAY_H
