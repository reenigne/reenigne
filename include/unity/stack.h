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
        _top = new Entry(t);
        _top->_next = top;
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
        T t = top();
        Entry* top = _top;
        _top = top->_next;
        delete top;
        --_n;
        return t;
    }
    T value(int offset) const
    {
        Entry* top = _top;
        while (offset > 0) {
            --offset;
            top = top->_next;
        }
        return top->_t;
    }
    T top() const { return _top->_t; }
    T fromTop(int n) const
    {
        Entry* top = _top;
        while (n > 0)
            top = top->_next;
        return top->_t;
    }
private:
    bool empty() const { return _top == 0; }

    class Entry
    {
    public:
        Entry(T t) : _t(t) { }
        T _t;
        Entry* _next;
    };
    Entry* _top;
    int _n;
};

#endif // INCLUDED_STACK_H
