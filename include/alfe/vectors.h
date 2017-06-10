#include "alfe/main.h"

#ifndef INCLUDED_VECTORS_H
#define INCLUDED_VECTORS_H

#include "alfe/rotors.h"

template<class Real> class Complex;

template<class T> class Vector2
{
public:
    Vector2() { }
    Vector2(T xx, T yy) : x(xx), y(yy) { }
    Vector2(const Vector2& other) : x(other.x), y(other.y) { }
    template<class T2> Vector2(const Vector2<T2>& o) : x(o.x), y(o.y) { }
    template<class T2> Vector2(const Complex<T2>& o) : x(o.x), y(o.y) { }
    const Vector2& operator=(const Vector2& o)
    {
        x = o.x;
        y = o.y;
        return *this;
    }
    bool operator==(const Vector2& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Vector2& o) const { return x != o.x || y != o.y; }
    const Vector2& operator+=(const T& n) { x += n; y += n; return *this; }
    const Vector2& operator-=(const T& n) { x -= n; y -= n; return *this; }
    const Vector2& operator*=(const T& n) { x *= n; y *= n; return *this; }
    const Vector2& operator/=(const T& n) { x /= n; y /= n; return *this; }
    const Vector2& operator%=(const T& n) { x %= n; y %= n; return *this; }
    const Vector2& operator&=(const T& n) { x &= n; y &= n; return *this; }
    const Vector2& operator|=(const T& n) { x |= n; y |= n; return *this; }
    const Vector2& operator+=(const Vector2& o)
    {
        x += o.x;
        y += o.y;
        return *this;
    }
    const Vector2& operator-=(const Vector2& o)
    {
        x -= o.x;
        y -= o.y;
        return *this;
    }
    const Vector2& operator*=(const Vector2& o)
    {
        x *= o.x;
        y *= o.y;
        return *this;
    }
    const Vector2& operator/=(const Vector2& o)
    {
        x /= o.x;
        y /= o.y;
        return *this;
    }
    const Vector2& operator%=(const Vector2& o)
    {
        x %= o.x;
        y %= o.y;
        return *this;
    }
    const Vector2& operator&=(const Vector2& o)
    {
        x &= o.x;
        y &= o.y;
        return *this;
    }
    const Vector2& operator|=(const Vector2& o)
    {
        x |= o.x;
        y |= o.y;
        return *this;
    }
    const Vector2& operator>>=(int n) { x >>= n; y >>= n; return *this; }
    const Vector2& operator<<=(int n) { x <<= n; y <<= n; return *this; }
    Vector2 operator+(const T& n) const { Vector2 t = *this; return t += n; }
    Vector2 operator-(const T& n) const { Vector2 t = *this; return t -= n; }
    Vector2 operator*(const T& n) const { Vector2 t = *this; return t *= n; }
    Vector2 operator/(const T& n) const { Vector2 t = *this; return t /= n; }
    Vector2 operator%(const T& n) const { Vector2 t = *this; return t %= n; }
    Vector2 operator&(const T& n) const { Vector2 t = *this; return t &= n; }
    Vector2 operator|(const T& n) const { Vector2 t = *this; return t |= n; }
    Vector2 operator+(const Vector2& o) const
    {
        Vector2 t = *this;
        return t += o;
    }
    Vector2 operator-(const Vector2& o) const
    {
        Vector2 t = *this;
        return t -= o;
    }
    Vector2 operator*(const Vector2& o) const
    {
        Vector2 t = *this;
        return t *= o;
    }
    Vector2 operator/(const Vector2& o) const
    {
        Vector2 t = *this;
        return t /= o;
    }
    Vector2 operator%(const Vector2& o) const
    {
        Vector2 t = *this;
        return t %= o;
    }
    Vector2 operator&(const Vector2& o) const
    {
        Vector2 t = *this;
        return t &= o;
    }
    Vector2 operator|(const Vector2& o) const
    {
        Vector2 t = *this;
        return t |= o;
    }
    Vector2 operator>>(int n) const { Vector2 t = *this; return t >>= n; }
    Vector2 operator<<(int n) const { Vector2 t = *this; return t <<= n; }
    template<class T2> Vector2 operator*(const Rotor2<T2>& r) const
    {
        T2 matrix[4];
        r.toMatrix(matrix);
        return Vector2<T>(
            matrix[0]*x + matrix[1]*y,
            matrix[2]*x + matrix[3]*y);
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

template<class T> T dot(const Vector2<T>& a, const Vector2<T>& b)
{
    return a.x*b.x + a.y*b.y;
}

template<class T> T cross(const Vector2<T>& a, const Vector2<T>& b)
{
    return a.x*b.y - a.y*b.x;
}

template<class T> Vector2<T> operator+(const T& n, const Vector2<T>& v)
{
    return v + n;
}
template<class T> Vector2<T> operator-(const T& n, const Vector2<T>& v)
{
    return (-v) + n;
}
template<class T> Vector2<T> operator*(const T& n, const Vector2<T>& v)
{
    return v * n;
}
template<class T> Vector2<T> operator/(const T& n, const Vector2<T>& v)
{
    return Vector2<T>(n/v.x, n/v.y);
}
template<class T> Vector2<T> operator%(const T& n, const Vector2<T>& v)
{
    return Vector2<T>(n%v.x, n%v.y);
}
template<class T> Vector2<T> operator&(const T& n, const Vector2<T>& v)
{
    return v & n;
}
template<class T> Vector2<T> operator|(const T& n, const Vector2<T>& v)
{
    return v | n;
}
template<class T> Vector2<T> operator^(const T& n, const Vector2<T>& v)
{
    return v ^ n;
}

template<class T> Vector2<T> floor(const Vector2<T>& vector)
{
    return Vector2<T>(floor(vector.x), floor(vector.y));
}

template<class T> class Vector2Cast : public Vector2<T>
{
public:
    template<class T2> Vector2Cast(const Vector2<T2>& other)
      : Vector2<T>(static_cast<T>(other.x), static_cast<T>(other.y)) { }
    template<class T2> Vector2Cast(const T2& x, const T2& y)
      : Vector2<T>(static_cast<T>(x), static_cast<T>(y)) { }
};

typedef Vector2<int> Vector;

template<class T> class Vector3
{
public:
    Vector3() { }
    Vector3(T xx, T yy, T zz) : x(xx), y(yy), z(zz) { }
    Vector3(const Vector3& other) : x(other.x), y(other.y), z(other.z) { }
    template<class T2> Vector3(const Vector3<T2>& o) : x(o.x), y(o.y), z(o.z)
    { }
    const Vector3& operator=(const Vector3& o)
    {
        x = o.x;
        y = o.y;
        z = o.z;
        return *this;
    }
    bool operator==(const Vector3& o) const
    {
        return x == o.x && y == o.y && z == o.z;
    }
    bool operator!=(const Vector3& o) const
    {
        return x != o.x || y != o.y && z == o.z;
    }
    const Vector3& operator+=(const T& n)
    {
        x += n;
        y += n;
        z += n;
        return *this;
    }
    const Vector3& operator-=(const T& n)
    {
        x -= n;
        y -= n;
        z -= n;
        return *this;
    }
    const Vector3& operator*=(const T& n)
    {
        x *= n;
        y *= n;
        z *= n;
        return *this;
    }
    const Vector3& operator/=(const T& n)
    {
        x /= n;
        y /= n;
        z /= n;
        return *this;
    }
    const Vector3& operator%=(const T& n)
    {
        x %= n;
        y %= n;
        z %= n;
        return *this;
    }
    const Vector3& operator&=(const T& n)
    {
        x &= n;
        y &= n;
        z &= n;
        return *this;
    }
    const Vector3& operator|=(const T& n)
    {
        x |= n;
        y |= n;
        z |= n;
        return *this;
    }
    const Vector3& operator+=(const Vector3& o)
    {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }
    const Vector3& operator-=(const Vector3& o)
    {
        x -= o.x;
        y -= o.y;
        z -= o.z;
        return *this;
    }
    const Vector3& operator*=(const Vector3& o)
    {
        x *= o.x;
        y *= o.y;
        z *= o.z;
        return *this;
    }
    const Vector3& operator/=(const Vector3& o)
    {
        x /= o.x;
        y /= o.y;
        z /= o.z;
        return *this;
    }
    const Vector3& operator%=(const Vector3& o)
    {
        x %= o.x;
        y %= o.y;
        z %= o.z;
        return *this;
    }
    const Vector3& operator&=(const Vector3& o)
    {
        x &= o.x;
        y &= o.y;
        z &= o.z;
        return *this;
    }
    const Vector3& operator|=(const Vector3& o)
    {
        x |= o.x;
        y |= o.y;
        z |= o.z;
        return *this;
    }
    const Vector3& operator>>=(int n)
    {
        x >>= n;
        y >>= n;
        z >>= n;
        return *this;
    }
    const Vector3& operator<<=(int n)
    {
        x <<= n;
        y <<= n;
        z <<= n;
        return *this;
    }
    Vector3 operator+(const T& n) const { Vector3 t = *this; return t += n; }
    Vector3 operator-(const T& n) const { Vector3 t = *this; return t -= n; }
    Vector3 operator*(const T& n) const { Vector3 t = *this; return t *= n; }
    Vector3 operator/(const T& n) const { Vector3 t = *this; return t /= n; }
    Vector3 operator%(const T& n) const { Vector3 t = *this; return t %= n; }
    Vector3 operator&(const T& n) const { Vector3 t = *this; return t &= n; }
    Vector3 operator|(const T& n) const { Vector3 t = *this; return t |= n; }
    Vector3 operator+(const Vector3& o) const
    {
        Vector3 t = *this;
        return t += o;
    }
    Vector3 operator-(const Vector3& o) const
    {
        Vector3 t = *this;
        return t -= o;
    }
    Vector3 operator*(const Vector3& o) const
    {
        Vector3 t = *this;
        return t *= o;
    }
    Vector3 operator/(const Vector3& o) const
    {
        Vector3 t = *this;
        return t /= o;
    }
    Vector3 operator%(const Vector3& o) const
    {
        Vector3 t = *this;
        return t %= o;
    }
    Vector3 operator&(const Vector3& o) const
    {
        Vector3 t = *this;
        return t &= o;
    }
    Vector3 operator|(const Vector3& o) const
    {
        Vector3 t = *this;
        return t |= o;
    }
    Vector3 operator>>(int n) const { Vector3 t = *this; return t >>= n; }
    Vector3 operator<<(int n) const { Vector3 t = *this; return t <<= n; }
    template<class T2> Vector3 operator*(const Rotor3<T2>& r) const
    {
        T2 matrix[9];
        r.toMatrix(&matrix);
        return Vector3<T>(
            matrix[0]*x + matrix[1]*y + matrix[2]*z,
            matrix[3]*x + matrix[4]*y + matrix[5]*z,
            matrix[6]*x + matrix[7]*y + matrix[8]*z);
    }
    template<class T2> Vector3 operator/(const Rotor3<T2>& r) const
    {
        return (*this) * r.conjugate();
    }
    Vector3 operator-() const { return Vector3<T>(-x, -y, -z); }
    bool zeroVolume() const { return x == 0 || y == 0 || z == 0; }
    T modulus2() const { return x*x + y*y + z*z; }
    T modulus() const { return sqrt(modulus2()); }
    T dot(const Vector3& o) { return x*o.x + y*o.y + z*o.z; }
    void normalize() { *this /= modulus(); }
    Vector3 normalized() const { return *this / modulus(); }
    bool inside(const Vector3& volume) const
    {
        return x >= 0 && y >= 0 && z >= 0 && x < volume.x && y < volume.y &&
            z < volume.z;
    }
	Hash hash() const
	{
		return Hash(typeid(Vector3)).mixin(x).mixin(y).mixin(z);
	}


    T x;
    T y;
    T z;
};

template<class T> Vector3<T> operator+(const T& n, const Vector3<T>& v)
{
    return v + n;
}
template<class T> Vector3<T> operator-(const T& n, const Vector3<T>& v)
{
    return (-v) + n;
}
template<class T> Vector3<T> operator*(const T& n, const Vector3<T>& v)
{
    return v * n;
}
template<class T> Vector3<T> operator/(const T& n, const Vector3<T>& v)
{
    return Vector2<T>(n/v.x, n/v.y, n/v.z);
}
template<class T> Vector3<T> operator%(const T& n, const Vector3<T>& v)
{
    return Vector2<T>(n%v.x, n%v.y, n%v.z);
}
template<class T> Vector3<T> operator&(const T& n, const Vector3<T>& v)
{
    return v & n;
}
template<class T> Vector3<T> operator|(const T& n, const Vector3<T>& v)
{
    return v | n;
}
template<class T> Vector3<T> operator^(const T& n, const Vector3<T>& v)
{
    return v ^ n;
}

template<class T> Vector3<T> floor(const Vector3<T>& vector)
{
    return Vector3<T>(floor(vector.x), floor(vector.y), floor(vector.z));
}

template<class T> T dot(const Vector3<T>& a, const Vector3<T>& b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

template<class T> class Vector3Cast : public Vector3<T>
{
public:
    template<class T2> Vector3Cast(const Vector3<T2>& other)
      : Vector3<T>(static_cast<T>(other.x), static_cast<T>(other.y),
        static_cast<T>(other.z)) { }
    template<class T2> Vector3Cast(const T2& x, const T2& y, const T2& z)
      : Vector3<T>(static_cast<T>(x), static_cast<T>(y), static_cast<T>(z)) { }
};

#endif // INCLUDED_VECTORS_H
