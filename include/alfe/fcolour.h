#include "alfe/main.h"

#ifndef INCLUDED_FCOLOUR_H
#define INCLUDED_FCOLOUR_H

#include "alfe/fix.h"

typedef Fixed<8, Word> UFix8p8;

class FColour
{
public:
    FColour() { }
    FColour(const FColour& x) : _r(x._r), _g(x._g), _b(x._b) { }
    const FColour& operator=(const FColour& x)
    {
        _r = x._r;
        _g = x._g;
        _b = x._b;
        return *this;
    }
    const FColour& operator+=(const FColour& x)
    {
        _r += x._r;
        _g += x._g;
        _b += x._b;
        return *this;
    }
    const FColour& operator-=(const FColour& x)
    {
        _r -= x._r;
        _g -= x._g;
        _b -= x._b;
        return *this;
    }
    const FColour& operator*=(const UFix8p8& x)
    {
        _r *= x;
        _g *= x;
        _b *= x;
        return *this;
    }
    const FColour& operator/=(const UFix8p8& x)
    {
        _r /= x;
        _g /= x;
        _b /= x;
        return *this;
    }
    const FColour& operator<<=(int n) 
    {
        _r <<= n;
        _g <<= n;
        _b <<= n;
        return *this;
    }
    const FColour& operator>>=(int n)
    {
        _r >>= n;
        _g >>= n;
        _b >>= n;
        return *this;
    }
    FColour operator+(const FColour& x) const
    {
        FColour y = *this;
        return y += x;
    }
    FColour operator-(const FColour& x) const
    {
        FColour y = *this;
        return y -= x;
    }
    FColour operator*(const UFix8p8& x) const
    {
        FColour y = *this; 
        return y *= x;
    }
    FColour operator/(const UFix8p8& x) const
    {
        FColour y = *this;
        return y /= x;
    }
    FColour operator<<(int n) const { FColour y = *this; return y <<= n; }
    FColour operator>>(int n) const { FColour y = *this; return y >>= n; }

    DWord asDWord() const
    {
        return 0xff000000
          | (static_cast<DWord>(_r.intPart())    )
          | (static_cast<DWord>(_g.intPart())<< 8)
          | (static_cast<DWord>(_b.intPart())<<16);
    }

    static FColour black() { return FColour(0, 0, 0); }
    static FColour white() { return FColour(255, 255, 255); }
    FColour(UFix8p8 r, UFix8p8 g, UFix8p8 b) : _r(r), _g(g), _b(b) { }
protected:
    UFix8p8 _r;
    UFix8p8 _g;
    UFix8p8 _b;
};

#endif // INCLUDED_FCOLOUR_H
