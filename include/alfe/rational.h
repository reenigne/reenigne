#include "alfe/main.h"

#ifndef INCLUDED_RATIONAL_H
#define INCLUDED_RATIONAL_H

#include "alfe/gcd.h"

class ZeroDivideException : public Exception
{
public:
    ZeroDivideException() : Exception("Division by zero") { }
};

template<class T> class RationalTemplate
{
public:
    RationalTemplate(const T& n, const T& d) : numerator(n), denominator(d)
    {
        normalize();
    }
    RationalTemplate(const T& n) : numerator(n), denominator(1) { }
    RationalTemplate() { }
    RationalTemplate& operator=(const RationalTemplate& r)
    {
        numerator = r.numerator; denominator = r.denominator;
        return *this;
    }
    RationalTemplate& operator=(const T& n) { numerator = n; denominator = 1; }
    RationalTemplate operator*(const RationalTemplate& other) const
    {
        RationalTemplate r = *this;
        r *= other;
        return r;
    }
    RationalTemplate operator/(const RationalTemplate& other) const
    {
        RationalTemplate r = *this;
        r /= other;
        return r;
    }
    RationalTemplate operator+(const RationalTemplate& other) const
    {
        RationalTemplate r = *this;
        r += other;
        return r;
    }
    RationalTemplate operator-(const RationalTemplate& other) const
    {
        RationalTemplate r = *this;
        r -= other;
        return r;
    }
    RationalTemplate operator-() const
    {
        return RationalTemplate(-numerator, denominator);
    }
    RationalTemplate& operator+=(const RationalTemplate& other)
    {
        T n = other.numerator;
        T d = other.denominator;
        T g = gcd(denominator, d);
        denominator /= g;
        numerator = numerator*(d/g) + n*denominator;
        g = gcd(numerator, g);
        numerator /= g;
        denominator *= (d/g);
        return *this;
    }
    RationalTemplate& operator-=(const RationalTemplate& other)
    {
        *this += (-other);
        return *this;
    }
    RationalTemplate& operator*=(const RationalTemplate& other)
    {
        T n = other.numerator;
        T d = other.denominator;
        T a = gcd(numerator, d);
        T b = gcd(n, denominator);
        numerator = (numerator/a)*(n/b);
        denominator = (denominator/b)*(d/a);
        return *this;
    }
    RationalTemplate& operator/=(const RationalTemplate& other)
    {
        T n = other.numerator;
        T d = other.denominator;
        if (n == 0)
            throw ZeroDivideException();
        if (numerator == 0)
            return *this;
        T a = gcd(numerator, n);
        T b = gcd(denominator, d);
        numerator = (numerator/a)*(d/b);
        denominator = (denominator/b)*(n/a);
        normalizeSign();
        return *this;
    }
    bool operator<(const RationalTemplate& other) const
    {
        return ((*this) - other).numerator < 0;
    }
    bool operator>(const RationalTemplate& other) const
    {
        return other < (*this);
    }
    bool operator<=(const RationalTemplate& other) const
    {
        return !((*this) > other);
    }
    bool operator>=(const RationalTemplate& other) const
    {
        return !((*this) < other);
    }
    bool operator==(const RationalTemplate& other) const
    {
        return numerator == other.numerator &&
            denominator == other.denominator;
    }
    bool operator!=(const RationalTemplate& other) const
    {
        return !operator==(other);
    }
    template<class U> U value() const
    {
        return static_cast<U>(numerator)/static_cast<U>(denominator);
    }
    T ceiling() const { return (numerator + denominator - 1)/denominator; }
    UInt32 hash() const
    {
        return Hash(typeid(RationalTemplate<T>)).mixin(numerator).
            mixin(denominator);
    }
    RationalTemplate frac() const
    {
        return RationalTemplate(numerator%denominator, denominator);
    }

    T numerator;
    T denominator;
private:
    void normalize()
    {
        if (denominator == 0)
            throw ZeroDivideException();
        if (numerator == 0) {
            denominator = 1;
            return;
        }
        T g = gcd(numerator, denominator);
        numerator /= g;
        denominator /= g;
        normalizeSign();
    }
    void normalizeSign()
    {
        if (denominator < 0) {
            numerator = -numerator;
            denominator = -denominator;
        }
    }
};

template<class T> RationalTemplate<T>
    lcm(const RationalTemplate<T>& x, const RationalTemplate<T>& y)
{
    return RationalTemplate<T>(
        lcm(x.numerator*y.denominator, y.numerator*x.denominator),
        lcm(x.denominator, y.denominator));
}

template<class T> RationalTemplate<T> operator*(const T& x,
    const RationalTemplate<T>& y)
{
    return y*x;
}

template<class T> RationalTemplate<T> operator/(const T& x,
    const RationalTemplate<T>& y)
{
    return RationalTemplate<T>(x)/y;
}

template<class T> RationalTemplate<T> operator+(const T& x,
    const RationalTemplate<T>& y)
{
    return y+x;
}

template<class T> RationalTemplate<T> operator-(const T& x,
    const RationalTemplate<T>& y)
{
    return x+(-y);
}

typedef RationalTemplate<int> Rational;

#endif // INCLUDED_RATIONAL_H
