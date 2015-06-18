#include "alfe/main.h"

#ifndef INCLUDED_STATISTICS_H
#define INCLUDED_STATISTICS_H

template<class T> class Statistics
{
public:
    Statistics() : _n(0), _total(0), _total2(0) { }
    void add(const T& value)
    {
        ++_n;
        _total += value;
        _total2 += value*value;
    }
    T mean() const { return _total/_n; }
    T variance() const { return _total2/_n - mean()*mean(); }
    T standardDeviation() const { return sqrt(variance()); }
    T populationDeviation() const { return sqrt(variance()*_n/(_n - 1)); }
private:
    int _n;
    T _total;
    T _total2;
};

#define INCLUDED_STATISTICS_H
