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

    T* getNext(LinkedListMember<T>* item = 0)
    {
        if (item == 0)
            item = this;
        LinkedListMember<T>* next = item->_next;
        if (next == this)
            return 0;
        return static_cast<T*>(next);
    }

    void release()
    {
        while (_next != this) {
            T* t = static_cast<T*>(_next);
            t->remove();
            delete t;
        }
    }

    void clear() { _next = _prev = this; }
    bool empty() const { return _next == this; }

    class Iterator
    {
    public:
        T& operator*() const { return *_node; }
        T* operator->() const { return _node; }
        const Iterator& operator++()
        {
            _node = _next;
            _next = _list->getNext(_node);
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
        bool end() const { return _node == 0; }
    private:
        T* _node;
        T* _next;
        LinkedList* _list;

        Iterator(LinkedList* list, T* node) : _list(list), _node(node)
        {
            _next = _list->getNext(_node);
        }

        friend class LinkedList;
    };
    Iterator begin() { return Iterator(this, getNext()); }
    Iterator end() { return Iterator(this, 0); }
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
};

#endif // INCLUDED_LINKED_LIST_H
