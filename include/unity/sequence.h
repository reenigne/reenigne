#ifndef INCLUDED_SEQUENCE_H
#define INCLUDED_SEQUENCE_H

template<class T> class Sequence
{
public:
    virtual bool empty() = 0;
    virtual T first() = 0;
    virtual Sequence<T> rest() = 0;
};

#endif // INCLUDED_SEQUENCE_H
