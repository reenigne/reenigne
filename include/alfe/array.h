#include "alfe/main.h"

#ifndef INCLUDED_ARRAY_H
#define INCLUDED_ARRAY_H

#include "alfe/bitwise.h"

// List is not quite a value type, since adding an element to a list will
// affect copies of the list.
template<class T> class List : private Handle
{
    class Body : public Handle::Body
    {
        class Node
        {
        public:
            template<typename... Args> Node(Args&&... args)
              : _value(std::forward<Args>(args)...), _next(0) { }
            Node* next() const { return _next; }
            void setNext(Node* next) { _next = next; }
            const T& value() const { return _value; }
        private:
            T _value;
            Node* _next;
            friend class Body;
            friend class List;
        };
    public:
        template<typename... Args> Body(Args&&... args)
          : _first(std::forward<Args>(args)...), _last(&_first), _count(1) { }
        ~Body()
        {
            Node* n = _first.next();
            while (n != 0) {
                Node* nn = n->next();
                delete n;
                n = nn;
            }
        }
        template<typename... Args> T* add(Args&&... args)
        {
            _last->setNext(new Node(std::forward<Args>(args)...));
            _last = _last->next();
            ++_count;
            return &_last->_value;
        }
        int count() const { return _count; }
        const Node* start() const { return &_first; }
    private:
        Node _first;
        Node* _last;
        int _count;

        friend class List;
        friend class List::Iterator;
    };
public:
    List() { }
    template<typename... Args> T* add(Args&&... args)
    {
        if (!valid()) {
            *this = List(create<Body>(std::forward<Args>(args)...));
            return &body()->_first._value;
        }
        return body()->add(std::forward<Args>(args)...);
    }
    int count() const
    {
        if (valid())
            return body()->count();
        return 0;
    }
    bool operator==(List other) const
    {
        Iterator l = begin();
        Iterator r = other.begin();
        while (l != end() && r != other.end()) {
            if (*l != *r)
                return false;
            ++l;
            ++r;
        }
        return l == end() && r == other.end();
    }
private:
    List(const Handle& other) : Handle(other) { }
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

template<class T> class Array;

template<class T, class Base = typename Array<T>::AppendableBaseBody>
    class AppendableArray;

// Array is not quite a value type, since changing an element in one array will
// affect copies of the same array unless a deep copy is made with copy().
template<class T> class Array : private Handle
{
public:
    // "allocate" is number of Ts to allocate space for.
    // "construct" is number of Ts to actually construct.
    template<class C = Handle, class H = Handle::Body> static
      C create(int allocate, int construct)
    {
        return create<C, H>(allocate, construct, 0);
    }
    template<class C = Handle, class H = Handle::Body, typename... Args> static
        C create(int allocate, int construct, int extraBytes, Args&&... args)
    {
        void* buffer = operator new(Body<H>::headSize() + allocate*sizeof(T) +
            extraBytes);
        Body<H>* b;
        try {
            b = new(buffer) Body<H>(std::forward<Args>(args)...);
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
        return C(b, false);
    }

    // This class combines an H and an array of Ts in a single memory block.
    // Also known as the "struct hack". T must be default-constructable.
    template<class H = Handle::Body> class Body : public H
    {
        // HT is never actually used directly - it's just used to figure out
        // the length and the address of the first T.
      //        class HT : public Body { public: T _t; };
    public:
        static int headSize() { return sizeof(HT<H>) - sizeof(T); }

        T* pointer() { return &static_cast<HT<H>*>(this)->_t; }
        const T* pointer() const
        {
            return &static_cast<const HT<H>*>(this)->_t;
        }
        T& operator[](int i) { return pointer()[i]; }
        const T& operator[](int i) const { return pointer()[i]; }

        void destroy() const
        {
            this->preDestroy();
            destruct();
            operator delete(const_cast<void*>(static_cast<const void*>(this)));
        }

        int size() const { return _size; }
        // Be careful to avoid calling setSize() with a size argument greater
        // than the "allocate" size passed to create().
        void setSize(int size)
        {
            if (size > _size)
                constructTail(size);
            else
                destructTail(size);
        }

        template<typename... Args> void constructT(Args&&... args)
        {
            new(static_cast<void*>(&(*this)[_size]))
                T(std::forward<Args>(args)...);
            ++_size;
        }

        class Iterator
        {
        public:
            Iterator() : _p(0) { }
            T& operator*() { return *_p; }
            T* operator->() { return _p; }
            const Iterator& operator++() { ++_p; return *this; }
            bool operator==(const Iterator& other) { return _p == other._p; }
            bool operator!=(const Iterator& other) { return _p != other._p; }
        private:
            T* _p;
            Iterator(T* p) : _p(p) { }
            friend class Body;
        };

        class ConstIterator
        {
        public:
            ConstIterator() : _p(0) { }
            const T& operator*() const { return *_p; }
            const T* operator->() const { return _p; }
            const ConstIterator& operator++() { ++_p; return *this; }
            bool operator==(const ConstIterator& other)
            {
                return _p == other._p;
            }
            bool operator!=(const ConstIterator& other)
            {
                return _p != other._p;
            }
        private:
            const T* _p;
            ConstIterator(const T* p) : _p(p) { }
            friend class Body;
        };

        ConstIterator begin() const { return ConstIterator(&((*this)[0])); }
        ConstIterator end() const { return ConstIterator(&((*this)[size()])); }
        Iterator begin() { return Iterator(&((*this)[0])); }
        Iterator end() { return Iterator(&((*this)[size()])); }

        void justSetSize(int size) const { _size = size; }

    private:
        void constructTail(int size)
        {
            int oldSize = _size;
            try {
                while (_size < size)
                    constructT();
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
            this->~Body();
        }

        mutable int _size;  // Needs to be mutable so destroy() can be const.

        // Only constructor is private to prevent inheritance, composition and
        // stack allocation. All instances are constructed via the placement
        // new call in create().
        template<typename... Args> Body(Args&&... args)
          : H(std::forward<Args>(args)...), _size(0) { }

        // HashTable and Set keep all elements constructed, and use _size to
        // keep track of the number of actual entries in the table.
        template<class Key, class Value> friend class HashTable;
        template<class Key> friend class Set;
        friend class Array;
    };

    template<class H> class HT : public Body<H> { public: T _t; };

    class AppendableBaseBody : public Handle::Body
    {
    public:
        int _allocated;
    };
    Array() { }
    Array(const List<T>& list)
    {
        int n = list.count();
        if (n != 0) {
            *this = Array(create<>(n, 0));
            for (auto p : list)
                body()->constructT(p);
        }
    }
    explicit Array(int n)
    {
        if (n != 0)
            *this = Array(create<>(n, n));
    }
    void allocate(int n) { *this = Array(n); }
    void ensure(int n) { if (count() < n) allocate(n); }
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
    template<class B> bool operator==(const AppendableArray<T, B>& other) const
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
    template<class B> bool operator!=(const AppendableArray<T, B>& other) const
    {
        return !operator==(other);
    }
    T& operator[](int i) { return (*body())[i]; }
    const T& operator[](int i) const { return (*body())[i]; }
    int count() const { return body() == 0 ? 0 : body()->size(); }
    Array copy() const
    {
        Array r(create<Body>(count(), 0));
        for (int i = 0; i < count(); ++i)
            r.body()->constructT((*this)[i]);
        return r;
    }

    typedef typename Body<>::Iterator Iterator;
    typedef typename Body<>::ConstIterator ConstIterator;
    ConstIterator begin() const
    {
        if (body() != 0)
            return body()->begin();
        return typename Body<>::ConstIterator();
    }
    ConstIterator end() const
    {
        if (body() != 0)
            return body()->end();
        return typename Body<>::ConstIterator();
    }
    Iterator begin()
    {
        if (body() != 0)
            return body()->begin();
        return typename Body<>::Iterator();
    }
    Iterator end()
    {
        if (body() != 0)
            return body()->end();
        return typename Body<>::Iterator();
    }

private:
    Array(const Handle& other) : Handle(other) { }
    Body<>* body() { return as<Body<>>(); }
    const Body<>* body() const { return as<Body<>>(); }
    template<class U, class B> friend class AppendableArray;
};

// AppendableArray is not quite a value type, since changing an element in one
// array will affect copies of the same array unless a deep copy is made with
// copy(). Appending to an array may cause it to become a deep copy, if more
// storage space was needed.
template<class T, class Base> class AppendableArray : private Handle
{
protected:
    typedef typename Array<T>::template Body<Base> Body;
public:
    AppendableArray() { }
    AppendableArray(const Handle& other) : Handle(other) { }
    AppendableArray(const List<T>& list) : AppendableArray(list.count())
    {
        for (auto p : list)
            body()->constructT(p);
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
        AppendableArray b =
            Array<T>::template create<Handle, Base>(count, 0, extra);
        b.body()->_allocated = count;
        *this = b;
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
    template<class B> void append(const AppendableArray<T, B>& other)
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
    void unappend(int elements = 1) { body()->setSize(count() - elements); }
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
    template<class B> bool operator==(const AppendableArray<T, B>& other) const
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
    template<class B> bool operator!=(const AppendableArray<T, B>& other) const
    {
        return !operator==(other);
    }
    T& operator[](int i) { return (*body())[i]; }
    const T& operator[](int i) const { return (*body())[i]; }
    int count() const { return body() == 0 ? 0 : body()->size(); }
    int allocated() const { return body() == 0 ? 0 : body()->_allocated; }
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
    Iterator begin()
    {
        if (body() != 0)
            return body()->begin();
        return Body::Iterator();
    }
    Iterator end()
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
            body()->constructT();
    }
    Body* body() { return as<Body>(); }
    const Body* body() const { return as<Body>(); }

    // For access to body().
    template<class Key, class Value> friend class HashTable;
    template<class Key> friend class Set;
};

#endif // INCLUDED_ARRAY_H
