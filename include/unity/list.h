#ifndef INCLUDED_LIST_H
#define INCLUDED_LIST_H

#include "unity/sequence.h"

template<class T> class List
{
public:
    List() : _first(0) { }
    ~List()
    {
        while (_first != 0) {
            Link* t = _first;
            _first = t->_next;
            delete t;
        }
    }
    void add(T value)
    {
        Link* t = _first;
        _first = new Link;
        _first->_next = t;
        _first->_value = value;
    }
    bool empty() const { return _first == 0; }
    T first() { return _first->_value; }
    Sequence<T> rest() { return *_first; }

private:
    class Link
    {
    public:
        T _value;
        Link* _next;
    };
    Link* _first;
};

#endif // INCLUDED_LIST_H
