#include "alfe/main.h"

#ifndef INCLUDED_ROTORS_H
#define INCLUDED_ROTORS_H

#define _USE_MATH_DEFINES
#include <math.h>

template<class T> class Vector2;
template<class T> class Rotor2_static_cast;

// A Rotor is a way of representing a rotation. It's faster than Euler angles
// (since the sines are cosines don't have to be computed each time the rotor
// is applied) but more memory efficient than a matrix (for dimensions <7).

// A Rotor can be thought of as the exponential of a bivector. The plane
// described by the bivector is the plane left invariant by the rotation. The
// magnitude of the bivector is half the rotation angle.

// 2D Rotors are isomorphic to unit complex numbers, and
// 3D Rotors are isomorphic to unit quaternions.

// Rotors R and -R have the same meaning geometrically, so they form a
// double-cover.

template<class T> class Rotor2
{
public:
    Rotor2() : _c(1), _s(0) { }
    Rotor2(T a)
    {
        _c = cos(a/2);
        _s = sin(a/2);
    }
    // Construct a rotor that rotates a onto b
    Rotor2(Vector2<T>& a, Vector2<T>& b)
    {
        T m = sqrt(a.modulus2()*b.modulus2());
        _c = dot(a, b)/m;
        _s = cross(b, a)/m;
    }
    Rotor2(const Rotor2& other) : _c(other._c), _s(other._s) { }
    template<class T2> Rotor2(const Rotor2<T2>& other)
      : _c(other._c), _s(other._s) { }
    const Rotor2& operator=(const Rotor2& other)
    {
        _c = other._c;
        _s = other._s;
        return *this;
    }
    const Rotor2& operator*=(const Rotor2& other)
    {
        *this = *this * other;
        return *this;
    }
    const Rotor2& operator/=(const Rotor2& other)
    {
        *this = *this / other;
        return *this;
    }
    Rotor2 operator*(const Rotor2& other) const
    {
        return Rotor2(_c*other._c-_s*other._s, _s*other._c+_c*other._s);
    }
    Rotor2 operator/(const Rotor2& other) const
    {
        return *this * other.conjugate();
    }
    void toMatrix(T* matrix) const
    {
        matrix[0] = 1 - 2*(_s*_s);
        matrix[1] =     2*(_s*_c);
        matrix[2] =   - 2*(_s*_c);
        matrix[3] = 1 - 2*(_s*_s);
    }

    // The conjugate rotor rotates in the opposite direction
    Rotor2 conjugate() const { return Rotor2(_c, -_s); }
private:
    Rotor2(const T& c, const T& s) : _c(c), _s(s) { }
    T _c;
    T _s;

    template<class T2> friend class Vector2;
    template<class T2> friend class Rotor2_static_cast;
};

template<class T> class Rotor2_static_cast : public Rotor2<T>
{
public:
    template<class T2> Rotor2_static_cast(const Rotor2<T2>& other)
      : Rotor2<T>(static_cast<T>(other._c), static_cast<T>(other._s)) { }
};

template<class T> class Vector3;
template<class T> class Rotor3_static_cast;

template<class T> class Rotor3
{
public:
    Rotor3() : _sc(1), _yz(0), _zx(0), _xy(0) { }
    static Rotor3<T> yz(T a) { return Rotor3<T>(cos(a/2), sin(a/2), 0, 0); }
    static Rotor3<T> zx(T a) { return Rotor3<T>(cos(a/2), 0, sin(a/2), 0); }
    static Rotor3<T> xy(T a) { return Rotor3<T>(cos(a/2), 0, 0, sin(a/2)); }

    //    // Construct a rotor that rotates a onto b
    //Rotor3(Vector3<T>& a, Vector3<T>& b)
    //{
    //    T m = sqrt(a.modulus2()*b.modulus2());
    //    _c = (b.x*a.x + b.y*a.y)/m;
    //    _s = (b.x*a.y - b.y*a.x)/m;
    //}
    Rotor3(const Rotor3& other)
      : _sc(other._sc), _yz(other._yz), _zx(other._zx), _xy(other._xy) { }
    template<class T2> Rotor3(const Rotor3<T2>& other)
      : _sc(other._sc), _yz(other._yz), _zx(other._zx), _xy(other._xy) { }
    const Rotor3& operator=(const Rotor3& other)
    {
        _sc = other._sc;
        _yz = other._yz;
        _zx = other._zx;
        _xy = other._xy;
        return *this;
    }
    const Rotor3& operator*=(const Rotor3& other)
    {
        *this = *this * other;
        return *this;
    }
    const Rotor3& operator/=(const Rotor3& other)
    {
        *this = *this / other;
        return *this;
    }
    Rotor3 operator*(const Rotor3& other) const
    {
        return Rotor3(
            _sc*other._sc - _yz*other._yz - _zx*other._zx - _xy*other._xy,
            _sc*other._yz + _yz*other._sc - _zx*other._xy + _xy*other._zx,
            _sc*other._zx + _yz*other._xy + _zx*other._sc - _xy*other._yz,
            _sc*other._xy - _yz*other._zx + _zx*other._yz + _xy*other._sc);
    }
    Rotor3 operator/(const Rotor3& other) const
    {
        return *this * other.conjugate();
    }

    // The conjugate rotor rotates in the opposite direction.
    Rotor3 conjugate() const { return Rotor3(_sc, -_yz, -zx, -xy); }

    void toMatrix(T* matrix) const
    {
        matrix[0] = 1 - 2*(_zx*_zx + _xy*_xy);
        matrix[1] =     2*(_xy*_sc + _zx*_yz);
        matrix[2] =     2*(_xy*_yz - _zx*_sc);
        matrix[3] =     2*(_zx*_yz - _xy*_sc);
        matrix[4] = 1 - 2*(_yz*_yz + _xy*_xy);
        matrix[5] =     2*(_yz*_sc + _xy*_zx);
        matrix[6] =     2*(_zx*_sc + _xy*_yz);
        matrix[7] =     2*(_xy*_zx - _yz*_sc);
        matrix[8] = 1 - 2*(_yz*_yz + _zx*_zx);
    }
private:
    Rotor3(const T& sc, const T& yz, const T& zx, const T& xy)
      : _sc(sc), _yz(yz), _zx(zx), _xy(xy) { }

    T _sc;
    T _yz;
    T _zx;
    T _xy;

    template<class T2> friend class Vector3;
    template<class T2> friend class Rotor3_static_cast;
};

#endif // INCLUDED_ROTORS_H
