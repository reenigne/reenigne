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

    static T MultiplyShiftRight(T x, T y)
    {
        return sp((dp(x)*dp(y) /*+ (1 << (N - 1))*/) >> N);
    }
    static T ShiftLeftDivide(T x, T y)
    {
        return sp(((dp(x) << N) /*+ (1 << (N - 1))*/) / dp(y));
    }
    static T MultiplyShiftLeftDivide(T x, T y, T z)
    {
        return sp((dp(x)*dp((y << N) /*+ (1 << (N - 1))*/)) / dp(z));
    }
    static T MultiplyDivide(T x, T y, T z)
    {
        return sp((dp(x)*dp(y))/dp(z));
    }
    static TT lmul(T x, T y) { return dp(x)*dp(y); }
};

template<int N, class T> class ArithmeticHelper
{
private:
    friend class Fixed<N, T, ArithmeticHelper<N, T> >;
    static T MultiplyShiftRight(T x, T y) { }
    static T ShiftLeftDivide(T x, T y) { }
    static T MultiplyShiftLeftDivide(T x, T y, T z) { }
    static T MultiplyDivide(T x, T y, T z) { }
};

template<int N> class ArithmeticHelper<N, Int8>
  : DoublePrecisionArithmeticHelper<N, Int8, Int16>
{
    friend class Fixed<N, Int8, ArithmeticHelper<N, Int8> >;
public:
    typedef Int16 DoubleType;
};

template<int N> class ArithmeticHelper<N, Int16>
  : DoublePrecisionArithmeticHelper<N, Int16, Int32>
{
    friend class Fixed<N, Int16, ArithmeticHelper<N, Int16> >;
public:
    typedef Int32 DoubleType;
};

template<int N> class ArithmeticHelper<N, Byte>
  : DoublePrecisionArithmeticHelper<N, Byte, Word>
{
    friend class Fixed<N, Byte, ArithmeticHelper<N, Byte> >;
public:
    typedef Word DoubleType;
};

template<int N> class ArithmeticHelper<N, Word>
  : DoublePrecisionArithmeticHelper<N, Word, DWord>
{
    friend class Fixed<N, Word, ArithmeticHelper<N, Word> >;
public:
    typedef DWord DoubleType;
};

#ifdef _WIN32

template<int N> class ArithmeticHelper<N, Int32>
{
private:
    friend class Fixed<N, Int32, ArithmeticHelper<N, Int32> >;

    // VC's inline asm doesn't allow "shl eax, N" directly, but this works...
    static const int nn[N];
    static const int n2[1 << (N - 1)];

    static Int32 MultiplyShiftRight(Int32 x, Int32 y)
    {
        __asm {
            mov eax, x
            imul y
            add eax, length n2
            shrd eax, edx, length nn
        }
    }
    static Int32 ShiftLeftDivide(Int32 x, Int32 y)
    {
        __asm {
            mov eax, x
            cdq
            shld edx, eax, length nn
            add eax, length n2
            adc edx, 0
            idiv y
        }
    }
    static Int32 MultiplyShiftLeftDivide(Int32 x, Int32 y, Int32 z)
    {
        __asm {
            mov eax, y
            shl eax, length nn
            add eax, length n2
            imul x
            idiv z
        }
    }
    static Int32 MultiplyDivide(Int32 x, Int32 y, Int32 z)
    {
        __asm {
            mov eax, y
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
    static const int n2[1 << (N - 1)];

    static DWord MultiplyShiftRight(DWord x, DWord y)
    {
        __asm {
            mov eax, x
            mul y
            add eax, length n2
            shrd eax, edx, length nn
        }
    }
    static DWord ShiftLeftDivide(DWord x, DWord y)
    {
        __asm {
            mov eax, x
            mov edx, 0
            shld edx, eax, length nn
            add eax, length n2
            adc edx, 0
            div y
        }
    }
    static DWord MultiplyShiftLeftDivide(DWord x, DWord y, DWord z)
    {
        __asm {
            mov eax, y
            shl eax, length nn
            add eax, length n2
            mul y
            div z
        }
    }
    static DWord MultiplyDivide(DWord x, DWord y, DWord z)
    {
        __asm {
            mov eax, y
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
    Fixed(float x) : _x(static_cast<T>(x*static_cast<float>(1<<N))) { }
    Fixed(const Fixed& x) : _x(x._x) { }
    template<int N2, class T2, class M2> Fixed(const Fixed<N2, T2, M2>& x)
      : _x(N<N2 ? (x._x>>(N2-N)) : (x._x<<(N-N2)))
    { }
    Fixed(T x, T m, T d) : _x(M::MultiplyShiftLeftDivide(x, m, d)) { }
    static Fixed fromRepresentation(T x) { Fixed r; r._x = x; return r; }
    T representation() const { return _x; }
    const Fixed& operator=(const Fixed& x) { _x = x._x; return *this; }
    const Fixed& operator=(T x) { _x = x<<N; return *this; }
    const Fixed& operator=(double x)
    {
        _x = static_cast<T>(x*static_cast<double>(1<<N));
        return *this;
    }
    const Fixed& operator=(float x)
    {
        _x = static_cast<T>(x*static_cast<float>(1<<N));
        return *this;
    }
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
    int intFloor() const { return static_cast<int>(_x>>N); }
    int intCeiling() const
    {
        return static_cast<int>((_x + ((1 << N) - 1)) >> N);
    }
    int intRound() const
    {
        return static_cast<int>((_x + (1 << (N - 1))) >> N);
    }
    Fixed floor() const { Fixed x; x._x = _x & ((-1) << N); return x; }
    Fixed ceiling() const
    {
        Fixed x;
        x._x = (_x + (1 << N) - 1) & ((-1) << N);
        return x;
    }
    Fixed round() const
    {
        Fixed x;
        x._x = (_x + (1 << (N - 1))) & ((-1) << N);
        return x;
    }
    float toFloat() const
    {
        return static_cast<float>(_x)/static_cast<float>(1<<N);
    }
    double toDouble() const
    {
        return static_cast<double>(_x)/static_cast<double>(1<<N);
    }
    Fixed operator-() const { Fixed x; x._x = -_x; return x; }
    Fixed frac() const { Fixed x; x._x = _x & ((1<<N)-1); return x; }
    explicit operator int() const { return static_cast<int>(_x >> N); }
    Fixed muld(const Fixed& y, const Fixed& z) const
    {
        Fixed x;
        x._x = M::MultiplyDivide(_x, y._x, z._x);
        return x;
    }
    template<int N2 = N + N,
        class T2 = typename ArithmeticHelper<N, T>::DoubleType,
        class M2 = ArithmeticHelper<N2, T2>>
        Fixed<N2, T2, M2> lmul(const Fixed& x) const
    {
        Fixed<N2, T2, M2> r;
        r._x = M::lmul(_x, x._x);
        const int s = N + N - N2;
        if (s < 0)
            r._x <<= -s;
        else
            r._x >>= s;
        return r;
    }

private:
    T _x;

    // This says that different instantiations of this template are friends
    // with each other, allowing the constructor which converts between
    // instances to access the raw value.
    template<int N2, class T2, class M2> friend class Fixed;
};

template<int N, class T, class M> Fixed<N, T, M>
    operator*(const T& x, const Fixed<N, T, M>& y)
{
    return y*x;
}

template<int N, class T, class M> Fixed<N, T, M>
    operator/(const T& x, const Fixed<N, T, M>& y)
{
    return static_cast<Fixed<N, T, M>>(x)/y;
}

template<int N, class T, class M> Fixed<N, T, M>
    operator-(const T& x, const Fixed<N, T, M>& y)
{
    return static_cast<Fixed<N, T, M>>(x) - y;
}

template<int N, class T, class M> Fixed<N, T, M>
    operator-(int x, const Fixed<N, T, M>& y)
{
    return static_cast<Fixed<N, T, M>>(x) - y;
}

template<int N, class T, class M> Fixed<N, T, M>
    operator-(const Fixed<N, T, M>& x, const T& y)
{
    return x - static_cast<Fixed<N, T, M>>(y);
}

template<int N, class T, class M> Fixed<N, T, M>
    operator+(const T& x, const Fixed<N, T, M>& y)
{
    return y + x;
}

template<int N, class T, class M> Fixed<N, T, M>
    floor(const Fixed<N, T, M>& x)
{
    return x.floor();
}

template<int N, class T, class M> Fixed<N, T, M>
    ceil(const Fixed<N, T, M>& x)
{
    return x.ceil();
}

template<int N, class T, class M, int N2 = N + N,
    class T2 = typename ArithmeticHelper<N, T>::DoubleType,
    class M2 = ArithmeticHelper<N2, T2>>
    Fixed<N2, T2, M2>
    lmul(const Fixed<N, T, M>& x, const Fixed<N, T, M>& y)
{
    return x.lmul<N2, T2, M2>(y);
}

template<int N, class T, class M> Fixed<N, T, M>
    muld(const Fixed<N, T, M>& a, const Fixed<N, T, M>& b,
    const Fixed<N, T, M>& c)
{
    return a.muld(b, c);
}

#endif // INCLUDED_FIX_H
