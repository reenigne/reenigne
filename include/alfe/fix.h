#include "alfe/main.h"

#ifndef INCLUDED_FIX_H
#define INCLUDED_FIX_H

#include <intrin.h>
#pragma intrinsic(__emul)
#pragma intrinsic(__emulu)
#pragma intrinsic(__ll_lshift)

template<int N, class T, class M> class Fixed;

template<int N, class T, class TT> class DoublePrecisionArithmeticHelper
{
private:
    template<int N, class T, class M> friend class Fixed;

    // Convert double precision to single precision
    static T sp(const TT& d) { return static_cast<T>(d); }

    // Convert single precision to double precision
    static TT dp(const T& s) { return static_cast<TT>(s); }

    static T MultiplyShiftRight(T x, T y) { return sp((dp(x)*dp(y))>>N); }
    static T ShiftLeftDivide(T x, T y) { return sp((dp(x)<<N)/dp(y)); }
    static T MultiplyShiftLeftDivide(T x, T y, T z)
    {
        return sp((dp(x)*dp(y<<N))/dp(z));
    }
};

template<int N, class T> class ArithmeticHelper
{
private:
    friend class Fixed<N, T, ArithmeticHelper<N, T> >;
    static T MultiplyShiftRight(T x, T y) { }
    static T ShiftLeftDivide(T x, T y) { }
    static T MultiplyShiftLeftDivide(T x, T y, T z) { }
};

template<int N> class ArithmeticHelper<N, Int8>
  : DoublePrecisionArithmeticHelper<N, Int8, Int16>
{
    friend class Fixed<N, Int8, ArithmeticHelper<N, Int8> >;
};

template<int N> class ArithmeticHelper<N, Int16>
  : DoublePrecisionArithmeticHelper<N, Int16, Int32>
{
    friend class Fixed<N, Int16, ArithmeticHelper<N, Int16> >;
};

template<int N> class ArithmeticHelper<N, Byte>
  : DoublePrecisionArithmeticHelper<N, Byte, Word>
{
    friend class Fixed<N, Byte, ArithmeticHelper<N, Byte> >;
};

template<int N> class ArithmeticHelper<N, Word>
  : DoublePrecisionArithmeticHelper<N, Word, DWord>
{
    friend class Fixed<N, Word, ArithmeticHelper<N, Word> >;
};

#ifdef _WIN32

template<int N> class ArithmeticHelper<N, Int32>
{
private:
    friend class Fixed<N, Int32, ArithmeticHelper<N, Int32> >;

    // VC's inline asm doesn't allow "shl eax, N" directly, but this works...
    static const int nn[N];

    static Int32 MultiplyShiftRight(Int32 x, Int32 y)
    {
        __asm {
            mov eax, x
            imul y
            shrd eax, edx, length nn
        }
    }
    static Int32 ShiftLeftDivide(Int32 x, Int32 y)
    {
        __asm {
            mov eax, x
            mov edx, 0
            shld edx, eax, length nn
            idiv y
        }
    }

    static Int32 MultiplyShiftLeftDivide(Int32 x, Int32 y, Int32 z)
    {
        __asm {
            mov eax, y
            shl eax, length nn
            imul x
            idiv z
        }
    }
};

template<int N> class ArithmeticHelper<N, DWord>
{
private:
    friend class Fixed<N, DWord, ArithmeticHelper<N, DWord> >;

    // VC's inline asm doesn't allow "shl eax, N" directly, but this works...
    static const int nn[N];

    static DWord MultiplyShiftRight(DWord x, DWord y)
    {
        __asm {
            mov eax, x
            mul y
            shrd eax, edx, length nn
        }
    }
    static DWord ShiftLeftDivide(DWord x, DWord y)
    {
        __asm {
            mov eax, x
            mov edx, 0
            shld edx, eax, length nn
            div y
        }
    }
    static DWord MultiplyShiftLeftDivide(DWord x, DWord y, DWord z)
    {
        __asm {
            mov eax, y
            shl eax, length nn
            mul y
            div z
        }
    }
};

#else  // _WIN32

template<int N> class ArithmeticHelper<N, Int32>
  : DoublePrecisionArithmeticHelper<N, Int32, Int64>
{
    friend class Fixed<N, Int32, ArithmeticHelper<N, Int32> >;
};

template<int N> class ArithmeticHelper<N, DWord>
  : DoublePrecisionArithmeticHelper<N, DWord, QWord>
{
    friend class Fixed<N, DWord, ArithmeticHelper<N, DWord> >;
};

#endif

template<int N, class T, class M = ArithmeticHelper<N, T> > class Fixed
{
public:
    Fixed() { }
    Fixed(int x) : _x(x<<N) { }
    Fixed(double x) : _x(static_cast<T>(x*static_cast<double>(1<<N))) { }
    Fixed(const Fixed& x) : _x(x._x) { }
    template<int N2, class T2, class M2> Fixed(const Fixed<N2, T2, M2>& x)
      : _x(N<N2 ? (x._x>>(N2-N)) : (x._x<<(N-N2)))
    { }
    Fixed(T x, T m, T d) : _x(M::MultiplyShiftLeftDivide(x, m, d)) { }
    const Fixed& operator=(const Fixed& x) { _x = x._x; return *this; }
    const Fixed& operator=(T x) { _x = x<<N; return *this; }
    const Fixed& operator+=(const Fixed& x) { _x += x._x; return *this; }
    const Fixed& operator-=(const Fixed& x) { _x -= x._x; return *this; }
    const Fixed& operator*=(const Fixed& x)
    {
        _x = M::MultiplyShiftRight(_x, x._x);
        return *this;
    }
    const Fixed& operator/=(const Fixed& x)
    {
        _x = M::ShiftLeftDivide(_x, x._x);
        return *this;
    }
    const Fixed& operator<<=(int n) { _x <<= n; return *this; }
    const Fixed& operator>>=(int n) { _x >>= n; return *this; }
    const Fixed& operator&=(const Fixed& x) { _x &= x._x; return *this; }
    Fixed operator+(const Fixed& x) const { Fixed y = *this; return y += x; }
    Fixed operator-(const Fixed& x) const { Fixed y = *this; return y -= x; }
    Fixed operator*(const Fixed& x) const { Fixed y = *this; return y *= x; }
    Fixed operator/(const Fixed& x) const { Fixed y = *this; return y /= x; }
    Fixed operator<<(int n) const { Fixed y = *this; return y <<= n; }
    Fixed operator>>(int n) const { Fixed y = *this; return y >>= n; }
    Fixed operator&(const Fixed& x) const { Fixed y=*this; return y &= x; }
    bool operator<(const Fixed& x) const { return _x < x._x; }
    bool operator>(const Fixed& x) const { return _x > x._x; }
    bool operator<=(const Fixed& x) const { return _x <= x._x; }
    bool operator>=(const Fixed& x) const { return _x >= x._x; }
    bool operator==(const Fixed& x) const { return _x == x._x; }
    bool operator!=(const Fixed& x) const { return _x != x._x; }
    int intPart() const { return static_cast<int>(_x>>N); }
    T floor() const { return static_cast<T>(_x>>N); }
    double toDouble() const
    {
        return static_cast<double>(_x)/static_cast<double>(1<<N);
    }
    Fixed operator-() const { Fixed x; x._x = -_x; return x; }
    Fixed frac() const { Fixed x; x._x = _x & ((1<<N)-1); return x; }
private:
    T _x;

    // This says that different instantiations of this template are friends
    // with each other, allowing the constructor which converts between
    // instances to access the raw value.
    template<int N2, class T2, class TT2> friend class Fixed;
};

template<int N, class T, class TT> Fixed<N, T, TT>
    operator*(const T& x, const Fixed<N, T, TT>& y)
{
    return y*x;
}

template<int N, class T, class TT> Fixed<N, T, TT>
    operator/(const T& x, const Fixed<N, T, TT>& y)
{
    return static_cast<Fixed<N, T, TT> >(x)/y;
}

template<int N, class T, class TT> Fixed<N, T, TT>
    operator-(const T& x, const Fixed<N, T, TT>& y)
{
    return static_cast<Fixed<N, T, TT> >(x) - y;
}

template<int N, class T, class TT> Fixed<N, T, TT>
    operator+(const T& x, const Fixed<N,T,TT>& y)
{
    return y + x;
}

#endif // INCLUDED_FIX_H
