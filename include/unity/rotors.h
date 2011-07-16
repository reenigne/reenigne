#ifndef INCLUDED_ROTORS_H
#define INCLUDED_ROTORS_H

#define _USE_MATH_DEFINES
#include <math.h>

template<class T> class Vector2;
template<class T> class Rotor2_static_cast;

// A rotor is a way of representing a rotation.
template<class T> class Rotor2
{
public:
    Rotor2() : _c(1), _s(0) { }
    Rotor2(T angle)
    {
        T a = static_cast<T>(angle*2*M_PI);
        _c = cos(a);
        _s = sin(a);
    }
    // Construct a rotor that rotates a onto b
    Rotor2(Vector2<T>& a, Vector2<T>& b)
    {
        T m = sqrt(a.modulus2()*b.modulus2());
        _c = (b.x*a.x + b.y*a.y)/m;
        _s = (b.x*a.y - b.y*a.x)/m;
    }
    Rotor2(const Rotor2& other) : _c(other._c), _s(other._s) { }
    template<class T2> Rotor2(const Rotor2<T2>& other) : _c(other._c), _s(other._s) { }
    const Rotor2& operator=(const Rotor2& other) { _c = other._c; _s = other._s; return *this; }
    const Rotor2& operator*=(const Rotor2& other) { *this = *this * other; return *this; }
    const Rotor2& operator/=(const Rotor2& other) { *this = *this / other; return *this; }
    Rotor2 operator*(const Rotor2& other) const { return Rotor2(_c*other._c-_s*other._s, _s*other._c+_c*other._s); }
    Rotor2 operator/(const Rotor2& other) const { return *this * other.conjugate(); }
    Rotor2 operator-() const { return Rotor2(-_c, -_s); } // The negative rotor rotates by an extra half-rotation
    Rotor2 conjugate() const { return Rotor2(_c, -_s); }  // The conjugate rotor rotates in the opposite direction
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
    template<class T2> Rotor2_static_cast(const Rotor2<T2>& other) : Rotor2(static_cast<T>(other._c), static_cast<T>(other._s)) { }
};

template<class T> class Vector3;
template<class T> class Rotor3_static_cast;

template<class T> class Rotor3
{
public:
    Rotor3() { }
    // TODO: Implement using a 4-component representation (scalar + bivector = exponential of a bivector)

private:

    template<class T2> friend class Vector3;
    template<class T2> friend class Rotor3_static_cast;
};

#endif // INCLUDED_ROTORS_H
