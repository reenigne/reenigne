#ifndef INCLUDED_VECTORS_H
#define INCLUDED_VECTORS_H

#include "unity/rotors.h"

template<class Real> class Complex;

template<class T> class Vector2
{
public:
    Vector2() { }
    Vector2(T xx, T yy) : x(xx), y(yy) { }
    Vector2(const Vector2& other) : x(other.x), y(other.y) { }
    template<class T2> Vector2(const Vector2<T2>& o) : x(o.x), y(o.y) { }
    template<class T2> Vector2(const Complex<T2>& o) : x(o.x), y(o.y) { }
    const Vector2& operator=(const Vector2& o) { x = o.x; y = o.y; return *this; }
    bool operator==(const Vector2& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vector2& o) const { return x != o.x || y != o.y; }
    const Vector2& operator+=(const T& n) { x += n; y += n; return *this; }
    const Vector2& operator-=(const T& n) { x -= n; y -= n; return *this; }
    const Vector2& operator*=(const T& n) { x *= n; y *= n; return *this; }
    const Vector2& operator/=(const T& n) { x /= n; y /= n; return *this; }
    const Vector2& operator%=(const T& n) { x %= n; y %= n; return *this; }
    const Vector2& operator&=(const T& n) { x &= n; y &= n; return *this; }
    const Vector2& operator|=(const T& n) { x |= n; y |= n; return *this; }
    const Vector2& operator+=(const Vector2& o) { x += o.x; y += o.y; return *this; }
    const Vector2& operator-=(const Vector2& o) { x -= o.x; y -= o.y; return *this; }
    const Vector2& operator*=(const Vector2& o) { x *= o.x; y *= o.y; return *this; }
    const Vector2& operator/=(const Vector2& o) { x /= o.x; y /= o.y; return *this; }
    const Vector2& operator%=(const Vector2& o) { x %= o.x; y %= o.y; return *this; }
    const Vector2& operator&=(const Vector2& o) { x &= o.x; y &= o.y; return *this; }
    const Vector2& operator|=(const Vector2& o) { x |= o.x; y |= o.y; return *this; }
    const Vector2& operator>>=(int n) { x >>= n; y >>= n; return *this; }
    const Vector2& operator<<=(int n) { x <<= n; y <<= n; return *this; }
    Vector2 operator+(const T& n) const { Vector2 t=*this; return t += n; }
    Vector2 operator-(const T& n) const { Vector2 t=*this; return t -= n; }
    Vector2 operator*(const T& n) const { Vector2 t=*this; return t *= n; }
    Vector2 operator/(const T& n) const { Vector2 t=*this; return t /= n; }
    Vector2 operator%(const T& n) const { Vector2 t=*this; return t %= n; }
    Vector2 operator&(const T& n) const { Vector2 t=*this; return t &= n; }
    Vector2 operator|(const T& n) const { Vector2 t=*this; return t |= n; }
    Vector2 operator+(const Vector2& o) const { Vector2 t=*this; return t += o; }
    Vector2 operator-(const Vector2& o) const { Vector2 t=*this; return t -= o; }
    Vector2 operator*(const Vector2& o) const { Vector2 t=*this; return t *= o; }
    Vector2 operator/(const Vector2& o) const { Vector2 t=*this; return t /= o; }
    Vector2 operator%(const Vector2& o) const { Vector2 t=*this; return t %= o; }
    Vector2 operator&(const Vector2& o) const { Vector2 t=*this; return t &= o; }
    Vector2 operator|(const Vector2& o) const { Vector2 t=*this; return t |= o; }
    Vector2 operator>>(int n) const { Vector2 t=*this; return t >>= n; }
    Vector2 operator<<(int n) const { Vector2 t=*this; return t <<= n; }
    template<class T2> Vector2 operator*(const Rotor2<T2>& r) const
    {
        return Vector2<T>(x*r._c + y*r._s, y*r._c - x*r._s);
    }
    template<class T2> Vector2 operator/(const Rotor2<T2>& r) const
    {
        return (*this) * r.conjugate();
    }
    Vector2 operator-() const { return Vector2<T>(-x, -y); }
    bool zeroArea() const { return x == 0 || y == 0; }
    T modulus2() const { return x*x + y*y; }
    bool inside(const Vector2& area) const
    {
        return x >= 0 && y >= 0 && x < area.x && y < area.y;
    }

    T x;
    T y;
};

template<class T> Vector2<T> floor(const Vector2<T>& vector)
{
    return Vector2<T>(floor(vector.x), floor(vector.y));
}

template<class T> class Vector2Cast : public Vector2<T>
{
public:
    template<class T2> Vector2Cast(const Vector2<T2>& other)
      : Vector2(static_cast<T>(other.x), static_cast<T>(other.y))
{ }
    template<class T2> Vector2Cast(const T2& x, const T2& y)
      : Vector2(static_cast<T>(x), static_cast<T>(y)) { }
};

typedef Vector2<int> Vector;

#endif // INCLUDED_VECTORS_H
