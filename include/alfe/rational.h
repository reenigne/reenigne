#include "alfe/main.h"

#ifndef INCLUDED_RATIONAL_H
#define INCLUDED_RATIONAL_H

#include "alfe/gcd.h"

class ZeroDivideException : public Exception
{
public:
    ZeroDivideException() : Exception("Division by zero") { }
};

template<class T> class Rational
{
public:
    Rational(const T& n, const T& d) : numerator(n), denominator(d)
    {
        normalize();
    }
    Rational(const T& n) : numerator(n), denominator(1) { }
    Rational() { }
    Rational& operator=(const Rational& r)
    {
        numerator = r.numerator; denominator = r.denominator;
        return *this;
    }
    Rational& operator=(const T& n) { numerator = n; denominator = 1; }
    Rational operator*(const Rational& other) const
    {
        Rational r = *this;
        r *= other;
        return r;
    }
    Rational operator/(const Rational& other) const
    {
        Rational r = *this;
        r /= other;
        return r;
    }
    Rational operator+(const Rational& other) const
    {
        Rational r = *this;
        r += other;
        return r;
    }
    Rational operator-(const Rational& other) const
    {
        Rational r = *this;
        r -= other;
        return r;
    }
    Rational operator-() const { return Rational(-numerator, denominator); }
    Rational& operator+=(const Rational& other)
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
    Rational& operator-=(const Rational& other)
    {
        *this += (-other);
        return *this;
    }
    Rational& operator*=(const Rational& other)
    {
        T n = other.numerator;
        T d = other.denominator;
        T a = gcd(numerator, d);
        T b = gcd(n, denominator);
        numerator = (numerator/a)*(n/b);
        denominator = (denominator/b)*(d/a);
        return *this;
    }
    Rational& operator/=(const Rational& other)
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
    bool operator<(const Rational& other) const
    {
        return ((*this) - other).numerator < 0;
    }
    bool operator>(const Rational& other) const { return other < (*this); }
    bool operator<=(const Rational& other) const
    {
        return !((*this) > other);
    }
    bool operator>=(const Rational& other) const
    {
        return !((*this) < other);
    }
    bool operator==(const Rational& other) const
    {
        return numerator == other.numerator &&
            denominator == other.denominator;
    }
    bool operator!=(const Rational& other) const
    {
        return !operator==(other);
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

template<class T> Rational<T> lcm(const Rational<T>& x, const Rational<T>& y)
{
    return Rational<T>(
        lcm(x.numerator*y.denominator, y.numerator*x.denominator),
        lcm(x.denominator, y.denominator));
}

template<class T> Rational<T> operator*(const T& x, const Rational<T>& y)
{
    return y*x;
}

template<class T> Rational<T> operator/(const T& x, const Rational<T>& y)
{
    return Rational<T>(x)/y;
}


#endif // INCLUDED_RATIONAL_H
