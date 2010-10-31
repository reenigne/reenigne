#ifndef INCLUDED_STACK_H
#define INCLUDED_STACK_H

#include "unity/array.h"

template<class T> class Stack
{
public:
    Stack() : _top(0), _n(0) { }
    ~Stack()
    {
        while (!empty())
            pop();
    }
    void push(T t)
    {
        Entry* top = _top;
        _top = new Entry;
        _top->_next = top;
        _top->_t = t;
        ++_n;
    }
    void toArray(Array<T>* array)
    {
        array->allocate(_n);
        for (int i = _n - 1; i >= 0; --i)
            (*array)[i] = pop();
    }
    T pop()
    {
        T t = _top->_t;
        Entry* top = _top;
        _top = top->_next;
        delete top;
        --_n;
        return t;
    }
private:
    bool empty() const { return _top == 0; }

    class Entry
    {
    public:
        T _t;
        Entry* _next;
    };
    Entry* _top;
    int _n;
};

#endif // INCLUDED_LIST_H
