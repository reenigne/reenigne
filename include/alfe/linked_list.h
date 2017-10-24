#include "alfe/main.h"

#ifndef INCLUDED_LINKED_LIST_H
#define INCLUDED_LINKED_LIST_H

template<class T> class LinkedListMember;

template<class T> class LinkedList : LinkedListMember<T>
{
public:
    LinkedList() { clear(); }

    void add(T* item)
    {
        item->_prev = _prev;
        _prev->_next = item;
        item->_next = this;
        _prev = item;
    }

    void release()
    {
        _next = this;
        _prev = this;
    }

    bool empty() const { return _next == this; }

    class Iterator
    {
    public:
        T& operator*() const { return *_node; }
        T* operator->() const { return _node; }
        const Iterator& operator++()
        {
            _node = static_cast<T*>(
                static_cast<LinkedListMember<T>*>(_node)->_next);
            return *this;
        }
        bool operator==(const Iterator& other) const
        {
            return _node == other._node;
        }
        bool operator!=(const Iterator& other) const
        {
            return !operator==(other);
        }
    private:
        T* _node;

        Iterator(T* node) : _node(node) { }

        friend class LinkedList;
    };
    Iterator begin() { return Iterator(_next); }
    Iterator end() { return Iterator(this); }
};

template<class T> class OwningLinkedList : LinkedList<T>
{
public:
    ~OwningLinkedList() { release(); }

    void release()
    {
        while (_next != this) {
            T* t = static_cast<T*>(_next);
            t->remove();
            delete t;
        }
    }
};

template<class T> class LinkedListMember : Uncopyable
{
public:
    LinkedListMember() : _next(0), _prev(0) { }

    void remove()
    {
        _next->_prev = _prev;
        _prev->_next = _next;
    }

    void moveFrom(LinkedListMember<T>* oldLocation)
    {
        _next = oldLocation->_next;
        _prev = oldLocation->_prev;
        _next->_prev = this;
        _prev->_next = this;
    }

private:
    LinkedListMember<T>* _next;
    LinkedListMember<T>* _prev;
    friend class LinkedList<T>;
    friend class LinkedList<T>::Iterator;
    friend class OwningLinkedList<T>;
};

#endif // INCLUDED_LINKED_LIST_H
