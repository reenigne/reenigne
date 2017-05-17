template<class T> void swap(T& x, T& y) { T z = x; x = y; y = z; }

unsigned int udiv_hisihi(unsigned long int ad, unsigned int b)
{
    unsigned int a = (unsigned int)(ad);
    unsigned int d = (unsigned int)(ad >> 16);
    asm ("div %2"
        : "+a" (a), "+d" (d)
        : "rm" (b)
        );
    return a;
}

class UFix8p8
{
public:
    UFix8p8() { }
    UFix8p8(int x) : _x(x<<8) { }
    static UFix8p8 fromRepresentation(unsigned int x) { UFix8p8 r; r._x = x; return r; }
    bool operator<(const UFix8p8& x) const { return _x < x._x; }
    bool operator>(const UFix8p8& x) const { return _x > x._x; }
    bool operator==(const UFix8p8& x) const { return _x == x._x; }
    UFix8p8 operator-() const { UFix8p8 x; x._x = -_x; return x; }
    const UFix8p8& operator+=(const UFix8p8& x) { _x += x._x; return *this; }
    const UFix8p8& operator-=(const UFix8p8& x) { _x -= x._x; return *this; }
    const UFix8p8& operator*=(const UFix8p8& x)
    {
        _x = (unsigned long)_x * (unsigned long)x._x >> 8;
        return *this;
    }
    const UFix8p8& operator/=(const UFix8p8& x)
    {
        _x = udiv_hisihi((unsigned long)_x << 8, x._x);
        return *this;
    }
    UFix8p8 operator+(const UFix8p8& x) const { UFix8p8 y = *this; return y += x; }
    UFix8p8 operator-(const UFix8p8& x) const { UFix8p8 y = *this; return y -= x; }
    UFix8p8 operator*(const UFix8p8& x) const { UFix8p8 y = *this; return y *= x; }
    UFix8p8 operator/(const UFix8p8& x) const { UFix8p8 y = *this; return y /= x; }
    int intFloor() const { return static_cast<int>(_x>>8); }
    int intCeiling() const
    {
        return static_cast<int>((_x + 0xff) >> 8);
    }

    unsigned int _x;
};

UFix8p8 operator-(int x, const UFix8p8& y)
{
    return static_cast<UFix8p8>(x) - y;
}

UFix8p8 muld(UFix8p8 a, UFix8p8 b, UFix8p8 c)
{
    return udiv_hisihi((unsigned long)(a._x)*(unsigned long)(b._x), c._x);
}

static UFix8p8 _xL;
static UFix8p8 _xR;
UFix8p8 _dxL;
UFix8p8 _dxR;

void fillTrapezoid(int yStart, int yEnd, UFix8p8 dxL, UFix8p8 dxR, int colour)
{
    _dxL = dxL;
    _dxR = dxR;
    asm volatile ("call fillTrapezoid1"
        : "+S" (colour), "+b" (yStart), "+c" (yEnd), "+d" (_xL), "+a" (_xR)
        :
        : "di" );
}

class Point2
{
public:
    UFix8p8 x;
    UFix8p8 y;
};

UFix8p8 slopeLeft(UFix8p8 dx, UFix8p8 dy, UFix8p8 x0, UFix8p8 y0,
    UFix8p8* x)
{
    if (dy < 1) {
        *x = x0 - muld(y0, dx, dy);
        return UFix8p8::fromRepresentation(0xffff);
    }
    else {
        UFix8p8 dxdy = dx/dy;
        *x = x0 - y0*dxdy;
        return dxdy;
    }
}
UFix8p8 slopeRight(UFix8p8 dx, UFix8p8 dy, UFix8p8 x0, UFix8p8 y0,
    UFix8p8* x)
{
    if (dy < 1) {
        *x = x0 + muld(y0, dx, dy);
        return UFix8p8::fromRepresentation(0xffff);
    }
    else {
        UFix8p8 dxdy = dx/dy;
        *x = x0 + y0*dxdy;
        return dxdy;
    }
}
UFix8p8 slope(UFix8p8 ux, UFix8p8 vx, UFix8p8 dy, UFix8p8 y0, UFix8p8* x)
{
    if (ux > vx)
        return slopeRight(ux - vx, dy, vx, y0, x);
    return -slopeLeft(vx - ux, dy, vx, y0, x);
}

void fillTriangle(Point2 a, Point2 b, Point2 c, int colour)
{
    if (a.y > b.y) swap(a, b);
    if (b.y > c.y) swap(b, c);
    if (a.y > b.y) swap(a, b);

    if (a.y == b.y) {
        if (b.y == c.y)
            return;
        if (a.x > b.x)
            swap(a, b);
        int yab = a.y.intCeiling();
        int yc = (c.y + 1).intFloor();
        UFix8p8 yac = c.y - a.y;
        UFix8p8 yaa = yab - a.y;
        fillTrapezoid(yab, yc, slope(c.x, a.x, yac, yaa, &_xL), slope(c.x, b.x, yac, yaa, &_xR), colour);
        return;
    }
    int ya = (a.y + 1).intFloor();
    UFix8p8 yab = b.y - a.y;
    if (b.y == c.y) {
        if (b.x > c.x)
            swap(b, c);
        int ybc = (b.y + 1).intFloor();
        UFix8p8 yaa = ya - a.y;
        fillTrapezoid(ya, ybc, slope(b.x, a.x, yab, yaa, &_xL), slope(c.x, a.x, yab, yaa, &_xR), colour);
        return;
    }

    int yb = (b.y + 1).intFloor();
    int yc = (c.y + 1).intFloor();
    UFix8p8 xb;
    UFix8p8 yaa = ya - a.y;
    UFix8p8 ybb = yb - b.y;
    UFix8p8 yac = c.y - a.y;
    UFix8p8 ybc = c.y - b.y;
    if (b.x > a.x) {
        UFix8p8 dab = slopeRight(b.x - a.x, yab, a.x, yaa, &xb);
        if (c.x > a.x) {
            UFix8p8 xc;
            UFix8p8 dac = slopeRight(c.x - a.x, yac, a.x, yaa, &xc);
            if (dab < dac) {
                _xL = xb;
                _xR = xc;
                fillTrapezoid(ya, yb, dab, dac, colour);
                fillTrapezoid(yb, yc, slope(c.x, b.x, ybc, ybb, &_xL), dac, colour);
            }
            else {
                _xL = xc;
                _xR = xb;
                fillTrapezoid(ya, yb, dac, dab, colour);
                fillTrapezoid(yb, yc, dac, slope(c.x, b.x, ybc, ybb, &_xR), colour);
            }
        }
        else {
            UFix8p8 dca = slopeLeft(a.x - c.x, yac, a.x, yaa, &_xL);
            _xR = xb;
            fillTrapezoid(ya, yb, -dca, dab, colour);
            fillTrapezoid(yb, yc, -dca, -slopeLeft(b.x - c.x, ybc, b.x, ybb, &_xR), colour);
        }
    }
    else {
        UFix8p8 dba = slopeLeft(a.x - b.x, yab, a.x, yaa, &xb);
        if (c.x > a.x) {
            UFix8p8 dac = slopeRight(c.x - a.x, yac, a.x, yaa, &_xR);
            _xL = xb;
            fillTrapezoid(ya, yb, -dba, dac, colour);
            fillTrapezoid(yb, yc, slopeRight(c.x - b.x, ybc, b.x, ybb, &_xL), dac, colour);
        }
        else {
            UFix8p8 xc;
            UFix8p8 dca = slopeLeft(a.x - c.x, yac, a.x, yaa, &xc);
            if (dba > dca) {
                _xL = xb;
                _xR = xc;
                fillTrapezoid(ya, yb, -dba, -dca, colour);
                fillTrapezoid(yb, yc, slope(c.x, b.x, ybc, ybb, &_xL), -dca, colour);
            }
            else {
                _xL = xc;
                _xR = xb;
                fillTrapezoid(ya, yb, -dca, -dba, colour);
                fillTrapezoid(yb, yc, -dca, slope(c.x, b.x, ybc, ybb, &_xR), colour);
            }
        }
    }
}

