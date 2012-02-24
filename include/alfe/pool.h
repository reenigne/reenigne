#include "alfe/main.h"

#ifndef INCLUDED_POOL_H
#define INCLUDED_POOL_H

#include <vector>
#include "alfe/linked_list.h"

// A memory pool is a fast, low-overhead way to allocate objects of type T when
// they're all going to be deleted at once. We allocate objects in blocks. Each
// block is a std::vector of blockSize Ts of size unitSize*blockSize. Note that
// Ts are constructed when a block is allocated, not when a T is requested.
//
// This is used by the grid tree Fractal programs. When those are replaced by
// quadtree equivalents, this class will be unused.
template<class T> class MemoryPool : Uncopyable
{
private:
    // Inner class representing a block (a set of Ts all allocateda at once)
    template<class T> class MemoryBlock : Uncopyable
    {
    public:
        // Constructor, creates a new MemoryBlock
        MemoryBlock(int size) : _data(size), _nextBlock(0) { }

        std::vector<T> _data;        // The actual T data
        MemoryBlock<T>* _nextBlock;  // Pointer to the next MemoryBlock in the list
    };

    // Deletes the rest of the MemoryBlock list for destruction or reset
    void release()
    {
        MemoryBlock<T>* block = _firstBlock._nextBlock;
        while (block != 0) {
            MemoryBlock<T>* next = block->_nextBlock;
            delete block;
            block = next;
        }
        _firstBlock._nextBlock = 0;
    }

public:
    // Constructor. Initializes the parameters and the initial MemoryBlock
    MemoryPool(int blockSize = 1024)
      : _firstBlock(blockSize),
        _blockSize(blockSize)
    { reset(); }

    // Gets a new unit of Ts, allocating a new MemoryBlock if necessary
    T* get()
    {
        T* unit = &*_end;

        // Here we are effectively incrementing the iterator _end but we can't
        // just do ++_end because we might need to create a new block, for
        // which we will need _blockSize, which is inaccessible to the iterator
        ++_end._object;
        if (_end._object == _end._block->_data.end()) {
            _end._block->_nextBlock = new MemoryBlock<T>(_blockSize);
            _end._block = _end._block->_nextBlock;
            _end._object = _end._block->_data.begin();
        }
        return unit;
    }

    void reset() { release(); _end = begin(); }

    class Iterator
    {
    public:
        Iterator() { }
        Iterator(typename std::vector<T>::iterator object, MemoryBlock<T>* block)
          : _object(object),
            _block(block)
        { }
        typename std::vector<T>::iterator _object; // Pointer to object within MemoryBlock
        MemoryBlock<T>* _block;           // Pointer to MemoryBlock
        Iterator& operator++() // preincrement
        {
            ++_object;
            if (_object == _block->_data.end()) {
                _block = _block->_nextBlock;
                _object = _block->_data.begin();
            }
            return *this;
        }
        Iterator& operator++(int) // postincrement
        {
            Iterator i = *this; ++*this; return t;
        }
        bool operator==(const Iterator& x) { return x._block==_block && x._object==_object; }
        bool operator!=(const Iterator& x) { return !((*this)==x); }
        T& operator*() const { return *_object; }
        T* operator->() const { return &**this; }
    };

    Iterator begin() { return Iterator(_firstBlock._data.begin(), &_firstBlock); }
    Iterator end() { return _end; }

    ~MemoryPool() { release(); }

private:
    MemoryBlock<T> _firstBlock;  // First MemoryBlock in pool (always exists)
    int _blockSize; // Number of Ts per MemoryBlock
    Iterator _end; // Next unit that will be returned
};


template<class T, class P> class Pool : Uncopyable
{
public:
    Pool() : _p(0) { }
    ~Pool() { deleteReserve(); _active.release(); }
    void create(P* p) { _p = p; }
    T* aquire()
    {
        T* t = _reserve.getNext();
        if (t == 0)
            t = _p->create();
        else
            t->remove();
        _active.add(t);
        return t;
    }
    void release(T* t) { t->remove(); _reserve.add(t); }
    void deleteReserve() { _reserve.release(); }
private:
    P* _p;
    LinkedList<T> _reserve;
    LinkedList<T> _active;
    int _activeCount;
};

#endif // INCLUDED_POOL_H
