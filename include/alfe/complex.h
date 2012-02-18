#ifndef INCLUDED_COMPLEX_H
#define INCLUDED_COMPLEX_H

#define _USE_MATH_DEFINES
#include <math.h>
#include "alfe/vectors.h"

template<class Real> class Complex
{
public:
    Complex(Real re, Real im) : x(re), y(im) { }
    Complex(Real re) : x(re), y(0) { }
    Complex() { }
    Complex(const Complex& other) : x(other.x), y(other.y) { }
    explicit Complex(const Vector2<Real>& vector) : x(vector.x), y(vector.y) { }
    Real modulus() const { return sqrt(modulus2()); }
    Real modulus2() const { return x*x + y*y; }
    Real argument() const { return atan2(y, x); }
    Complex conjugate() const { return Complex(x, -y); }
    const Complex& operator+=(const Complex& other) { x += other.x; y += other.y; return *this; }
    const Complex& operator-=(const Complex& other) { x -= other.x; y -= other.y; return *this; }
    const Complex& operator*=(const Complex& other)
    {
        Real xx = x*other.x;
        Real yy = y*other.y;
        y = (x+y)*(other.x+other.y)-xx-yy;
        x = xx-yy;
        return *this;
    }
    const Complex& operator/=(const Complex& other)
    {
        Real q = other.modulus2();
        *this *= other.conjugate();
        *this /= q;
        return *this;
    }
    const Complex& operator+=(const Real& other) { x += other; return *this; }
    const Complex& operator-=(const Real& other) { x -= other; return *this; }
    const Complex& operator*=(const Real& other) { x *= other; y *= other; return *this; }
    const Complex& operator/=(const Real& other) { x /= other; y /= other; return *this; }
    Complex operator+(const Complex& other) const { Complex t(*this); t += other; return t; }
    Complex operator-(const Complex& other) const { Complex t(*this); t -= other; return t; }
    Complex operator*(const Complex& other) const { Complex t(*this); t *= other; return t; }
    Complex operator/(const Complex& other) const { Complex t(*this); t /= other; return t; }
    Complex operator+(const Real& other) const { Complex t(*this); t += other; return t; }
    Complex operator-(const Real& other) const { Complex t(*this); t -= other; return t; }
    Complex operator*(const Real& other) const { Complex t(*this); t *= other; return t; }
    Complex operator/(const Real& other) const { Complex t(*this); t /= other; return t; }
    Complex operator-() const { return Complex(-x, -y); }
    const Complex& operator=(const Real& re) { x = re; y = 0; return *this; }
    const Complex& operator=(const Complex& other) { x = other.x; y = other.y; return *this; }

    bool operator==(const Complex& other) { return x==other.x && y==other.y; }

    Real x, y;
};

template<class Real> Complex<Real> operator*(const Real& x, const Complex<Real>& c) { return c*x; }
template<class Real> Complex<Real> operator/(const Real& x, const Complex<Real>& c) { return Complex<Real>(x)/c; }
template<class Real> Complex<Real> operator-(const Real& x, const Complex<Real>& c) { return Complex<Real>(x)-c; }
template<class Real> Complex<Real> operator+(const Real& x, const Complex<Real>& c) { return c+x; }

template<class Real> Complex<Real> exp(const Complex<Real>& z)
{
    Real magnitude = exp(z.x);
    return Complex<Real>(magnitude*cos(z.y), magnitude*sin(z.y));
}

template<class Real> Complex<Real> log(const Complex<Real>& z)
{
    return Complex<Real>(log(z.modulus()), z.argument());
}

template<class Real> Complex<Real> log(const Complex<Real>& z, Real theta)
{
    Real n = floor(((theta - z.argument())/static_cast<Real>(M_PI)+1)/2);
    return Complex<Real>(log(z.modulus()), z.argument() + 2*static_cast<Real>(M_PI)*n);
}

template<class Real> Complex<Real> sqrt(const Complex<Real>& z)
{
    Real r = z.modulus();
    Real x = z.x;
    if (z.y > 0)
        return Complex<Real>(sqrt((r+x)/2), sqrt((r-x)/2));
    return Complex<Real>(sqrt((r+x)/2), -sqrt((r-x)/2));
}

template<class Real> Complex<Real> pow(const Complex<Real>& a,const Complex<Real>& b)
{
    return exp(b*log(a));
}

template<class Real> Complex<Real> pow(const Complex<Real>& a,const Complex<Real>& b, Real theta)
{
    return exp(b*log(a, theta));
}

template<class Real> Complex<Real> sin(const Complex<Real>& z)
{
    return Complex<Real>(sin(z.x)*cosh(z.y), cos(z.x)*sinh(z.y));
}

template<class Real> Complex<Real> cos(const Complex<Real>& z)
{
    return Complex<Real>(cos(z.x)*cosh(z.y), -sin(z.x)*sinh(z.y));
}

template<class Real> Complex<Real> tan(const Complex<Real>& z)
{
    return sin(z)/cos(z);
}

template<class Real> Complex<Real> atan(const Complex<Real>& z)
{
    return Complex<Real>(0, -1)*(log(Complex<Real>(1+z.y, z.x))-log(Complex<Real>(1-z.y, z.x)))/2;
}

template<class Real> Complex<Real> sinh(const Complex<Real>& z)
{
    return Complex<Real>(sinh(z.x)*cos(z.y), cosh(z.x)*sin(z.y));
}

template<class Real> Complex<Real> cosh(const Complex<Real>& z)
{
    return Complex<Real>(cosh(z.x)*cos(z.y), sinh(z.x)*sin(z.y));
}

template<class Real> Complex<Real> tanh(const Complex<Real>& z)
{
    return sinh(z)/cosh(z);
}

template<class Real> Complex<Real> atanh(const Complex<Real>& z)
{
    return log(1+z)-log(1-z)/2;
}

#endif // INCLUDED_COMPLEX_H
