#ifndef INCLUDED_MININUM_MAXIMUM_H
#define INCLUDED_MININUM_MAXIMUM_H

template<class T> T max(const T& a, const T& b) { return a > b ? a : b; }

template<class T> T min(const T& a, const T& b) { return a < b ? a : b; }

template<class T> T clamp(T low, T value, T high)
{
    return min(max(low, value), high);
}

template<class T> T max(const T& a, const T& b, const T& c)
{
    return max(a, max(b, c));
}


template<class T> T min(const T& a, const T& b, const T& c)
{
    return min(a, min(b, c));
}


template<class T> T max(const T& a, const T& b, const T& c, const T& d)
{
    return max(a, max(b, c, d));
}


template<class T> T min(const T& a, const T& b, const T& c, const T& d)
{
    return min(a, min(b, c, d));
}

#endif // INCLUDED_MININUM_MAXIMUM_H
