#ifndef INCLUDED_LIST_H
#define INCLUDED_LIST_H

template<class T> class List
{
public:
    virtual bool empty() = 0;
    virtual T first() = 0;
    virtual List<T> rest() = 0;
};

#endif // INCLUDED_LIST_H
