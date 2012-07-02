#include "alfe/main.h"

#ifndef INCLUDED_REFERENCE_COUNTED_ARRAY_H
#define INCLUDED_REFERENCE_COUNTED_ARRAY_H

// This class combines an H and an array of Ts in a single memory block. H
// must inherit from ReferenceCountedArray<H, T> (curiously recurring template
// pattern). H must have alignment equal to or greater than T. This can be
// achieved generically by making H have a member of type T. The resulting
// object can be used as a base class for a Reference Implementation.
template<class H, class T> class ReferenceCountedArray
  : public ReferenceCounted
{
public:
    static H* create(int size)
    {
        void* buffer =
            static_cast<void*>(operator new(sizeof(H) + size*sizeof(T)));
        H* h;
        try {
            h = new(buffer) H();
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
        return h;
    }
protected:
    T* pointer()
    {
        return static_cast<T*>(static_cast<Byte*>(static_cast<void*>(this)) +
            sizeof(H));
    }
    const T* pointer() const
    {
        return static_cast<const T*>(
            static_cast<const Byte*>(static_cast<const void*>(this)) +
            sizeof(H));
    }
    T& operator[](int i) { return pointer()[i]; }
    const T& operator[](int i) const { return pointer()[i]; }

    int _size;

    void destroy()
    {
        for (int i = _size - 1; i >= 0; --i)
            (&(*this)[i])->~T();
        static_cast<H*>(this)->~H();
        operator delete(static_cast<void*>(this));
    }
};

#endif // INCLUDED_REFERENCE_COUNTED_ARRAY_H
