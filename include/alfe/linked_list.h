#include "alfe/main.h"

#ifndef INCLUDED_LINKED_LIST_H
#define INCLUDED_LINKED_LIST_H

template<class T> class LinkedList;
template<class T> class OwningLinkedList;

template<class T> class LinkedListMember : Uncopyable
{
public:
    LinkedListMember() : _next(this), _prev(this) { }

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

    LinkedListMember<T>* next() { return _next; }
    LinkedListMember<T>* previous() { return _prev; }
    void insertBefore(T* item)
    {
        item->_prev = _prev;
        _prev->_next = item;
        item->_next = this;
        _prev = item;
    }
    void insertAfter(T* item)
    {
        item->_prev = this;
        _next->_prev = item;
        item->_next = _next;
        _next = item;
    }

private:
    LinkedListMember<T>* _next;
    LinkedListMember<T>* _prev;
    friend class LinkedList<T>;
    //friend class LinkedList<T>::Iterator;
    //template<> friend class OwningLinkedList<T>;
};

template<class T> class LinkedList : public LinkedListMember<T>
{
public:
    void add(T* item) { insertBefore(item); }

    T* getNext(LinkedListMember<T>* c = 0)
    {
        if (c == 0)
            c = this;
        LinkedListMember<T>* n = c->next();
        if (n == this)
            return 0;
        return static_cast<T*>(n);
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
        T& operator*() const { return *static_cast<T*>(_node); }
        T* operator->() const { return static_cast<T*>(_node); }
        const Iterator& operator++()
        {
            _node = _node->_next;
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
        LinkedListMember<T>* _node;

        Iterator(LinkedListMember<T>* node) : _node(node) { }

        friend class LinkedList<T>;
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

#endif // INCLUDED_LINKED_LIST_H
