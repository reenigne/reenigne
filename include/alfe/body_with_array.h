#include "alfe/main.h"
#include <utility>

#ifndef INCLUDED_BODY_WITH_ARRAY_H
#define INCLUDED_BODY_WITH_ARRAY_H

// This class combines an H and an array of Ts in a single memory block.
// Also known as the "struct hack". T must be default-constructable.
template<class H, class T> class BodyWithArray : public H
{
    // HT is never actually used directly - it's just used to figure out the
    // length and the address of the first T.
    class HT : public BodyWithArray { public: T _t; };
public:
    static int headSize() { return sizeof(HT) - sizeof(T); }

    // "allocate" is number of Ts to allocate space for.
    // "construct" is number of Ts to actually construct.
    template<typename... Args> static BodyWithArray* create(int allocate,
        int construct, int extraBytes = 0, Args&&... args)
    {
        void* buffer = operator new(headSize() + allocate*sizeof(T) +
            extraBytes);
        BodyWithArray* b;
        try {
            b = new(buffer) BodyWithArray(std::forward<Args>(args)...);
            try {
                b->constructTail(construct);
            }
            catch (...) {
                b->destruct();
                throw;
            }
        }
        catch (...) {
            operator delete(buffer);
            throw;
        }
        return b;
    }

    T* pointer() { return &static_cast<HT*>(this)->_t; }
    const T* pointer() const { return &static_cast<const HT*>(this)->_t; }
    T& operator[](int i) { return pointer()[i]; }
    const T& operator[](int i) const { return pointer()[i]; }

    void destroy() const
    {
        destruct();
        operator delete(const_cast<void*>(static_cast<const void*>(this)));
    }

    int size() const { return _size; }
    // Be careful to avoid calling setSize() with a size argument greater than
    // the "allocate" size passed to create(). 
    void setSize(int size)
    {
        if (size > _size)
            constructTail(size);
        else
            destructTail(size);
    }
    void constructT(const T& other)
    {
        new(static_cast<void*>(&(*this)[_size])) T(other);
        ++_size;
    }

    class Iterator
    {
    public:
        Iterator() : _p(0) { }
        const T& operator*() const { return *_p; }
        const T* operator->() const { return _p; }
        const Iterator& operator++() { ++_p; return *this; }
        bool operator==(const Iterator& other) { return _p == other._p; }
        bool operator!=(const Iterator& other) { return !operator==(other); }
    private:
        const T* _p;

        Iterator(const T* p) : _p(p) { }

        template<class T> friend class ArrayTemplate;
    };

    Iterator begin() const { return Iterator(&((*this)[0])); }
    Iterator end() const { return Iterator(&((*this)[size()])); }

private:
    void constructTail(int size)
    {
        int oldSize = _size;
        try {
            while (_size < size)
                constructT(T());
        }
        catch (...) {
            destructTail(oldSize);
            throw;
        }
    }
    void destructTail(int size) const
    {
        for (; _size > size; --_size)
            (&(*this)[_size - 1])->~T();
    }
    void destruct() const
    {
        destructTail(0);
        this->~BodyWithArray();
    }

    mutable int _size;  // Needs to be mutable so destroy() can be const.

    // Only constructor is private to prevent inheritance, composition and
    // stack allocation. All instances are constructed via the placement new
    // call in create().
    template<typename... Args> BodyWithArray(Args&&... args)
      : H(std::forward<Args>(args)...), _size(0) { }
};

#endif // INCLUDED_BODY_WITH_ARRAY_H
