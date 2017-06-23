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

void addSpan(Byte c, Byte xL, Byte xR, Byte* p)
{
    if (xL >= xR)
        return;

    Byte* n = *(Byte**)(p - 1);

    Byte* i = p + 2;
    do {
        i += 2;
    } while (xL >= *i);
    i -= 2;
    Byte* j = p;
    do {
        j += 2;
    } while (xR >= *j);

    if (c == i[1])
        xL = *i;
    else
        if (i > 0 && xL == *i && c == i[-1]) {
            i -= 2;
            xL = *i;
        }
    if (c == j[-1])
        xR = *j;
    else
        if (j < n && xR == *j && c == j[1]) {
            j += 2;
            xR = *j;
        }

    int o = (j - 2) - i;
    if (xL == *i) {
        // Left of new span at left of left old
        if (xR == *j) {
            // Right of new span at right of right old
            n -= o;
            *(Byte**)(p - 1) = n;
            ++i;
            *i = c;


            _s[i]._c = c;
            _n -= o;
            for (int k = i + 1; k <= _n; ++k)
                _s[k] = _s[k + o];
        }
        else {
            --o;
            _n -= o;
            switch (o) {
                case -1:
                    // Have to do the update after the move or we'd
                    // be stomping on data we need to keep.
                    for (int k = _n; k >= i - o; --k)
                        _s[k] = _s[k + o];
                    _s[i]._c = c;
                    _s[i + 1]._x = xR;
                    break;
                case 0:
                    _s[i]._c = c;
                    _s[i + 1]._x = xR;
                    break;
                default:
                    _s[i]._c = c;
                    _s[i + 1]._x = xR;
                    _s[i + 1]._c = _s[i + 1 + o]._c;
                    for (int k = i + 2; k <= _n; ++k)
                        _s[k] = _s[k + o];
                    break;
            }
        }
        if (_s[_n]._x != 255 || _s[_n - 1]._c != 0)
            printf("Error\n");
        return;
    }
    if (xR == _s[j + 1]._x) {
        // Right of new span at right of right old
        --o;
        _n -= o;
        switch (o) {
            case -1:
                // Untested
                _s[i + 1]._x = xL;
                _s[i + 1]._c = c;
                for (int k = _n; k > i - o; --k)
                    _s[k] = _s[k + o];
                break;
            case 0:
                _s[i + 1]._x = xL;
                _s[i + 1]._c = c;
                break;
            default:
                _s[i + 1]._x = xL;
                _s[i + 1]._c = c;
                for (int k = i + 2; k <= _n; ++k)
                    _s[k] = _s[k + o];
                break;
        }
    }
    else {
        o -= 2;
        _n -= o;
        switch (o) {
            case -2:
                // Have to do the update after the move
                for (int k = _n; k > i - o; --k)
                    _s[k] = _s[k + o];
                _s[i + 2]._c = _s[i]._c;
                _s[i + 2]._x = xR;
                _s[i + 1]._x = xL;
                _s[i + 1]._c = c;
                break;
            case -1:
                // Have to do the update after the move
                for (int k = _n; k > i - o; --k)
                    _s[k] = _s[k + o];
                _s[i + 2]._x = xR;
                _s[i + 1]._x = xL;
                _s[i + 1]._c = c;
                break;
            case 0:
                _s[i + 2]._x = xR;
                _s[i + 1]._x = xL;
                _s[i + 1]._c = c;
                break;
            default:
                // Untested
                _s[i + 2]._x = xR;
                _s[i + 2]._c = _s[i + 2 + o]._c;
                _s[i + 1]._x = xL;
                _s[i + 1]._c = c;
                for (int k = i + 3; k <= _n; ++k)
                    _s[k] = _s[k + o];
        }
    }
    if (_s[_n]._x != 255 || _s[_n - 1]._c != 0)
        printf("Error\n");
}

void renderDeltas(Byte* vram, const Line* o) const
{
    ++globalCount;
    if (globalCount == 0x00002a47)
        printf("Break");
    Byte* vram0 = vram;

    static const Byte mask[4] = {0xff, 0x3f, 0x0f, 0x03};

    const Span* sn = _s;
    const Span* so = o->_s;
    int cn = sn->_c;
    ++sn;
    int xLn = 0;
    int xRn = sn->_x;
    int co = so->_c;
    ++so;
    int xLo = 0;
    int xRo = so->_x;

    bool havePartial = false;
    Byte partial;
    do {
        if ((xRn & 0xfc) == (xLn & 0xfc)) {
            if (cn != co || havePartial) {
                Byte newBits = cn & mask[xLn & 3] & ~mask[xRn & 3];
                if (!havePartial) {
                    if ((xLn & 3) == 0)
                        partial = 0;
                    else
                        partial = vram[xLn >> 2] & ~mask[xLn & 3];
                    havePartial = true;
                }
                partial |= newBits;
            }
            else {
                if ((xRo & 0xfc) == (xLn & 0xfc)) {
                    partial = ((vram[xLn >> 2] & ~mask[xRo & 3]) | (cn & mask[xRo & 3])) & ~mask[xRn & 3];
                    havePartial = true;
                }
            }
        }
        else {
            if (cn != co) {
                if (!havePartial)
                    partial = vram[xLn >> 2] & ~mask[xLn & 3];
                vram[xLn >> 2] = partial | (cn & mask[xLn & 3]);
            }
            else {
                if (havePartial)
                    vram[xLn >> 2] = partial | (cn & mask[xLn & 3]);
                else {
                    if ((xRo & 0xfc) == (xLn & 0xfc)) {
                        Byte* p = vram + (xLn >> 2);
                        *p = (*p & ~mask[xLn & 3]) | (cn & mask[xLn & 3]);
                    }
                }
            }
            xLn = (xLn + 3) & 0xfc;
            int storeStart = xLn;
            int storeEnd = xLn;
            bool store = false;
            do {
                if (store) {
                    if (cn == co) {
                        int aligned = (xLo + 3) & 0xfc;
                        if (xRn >= aligned)
                            storeEnd = aligned;
                        else
                            storeEnd = xLo;
                        store = false;
                    }
                }
                else {
                    if (cn != co) {
                        if ((xLo & 0xfc) > (storeEnd & 0xfc) + 4) {
                            if (storeStart < storeEnd) {
                                int startByte = storeStart >> 2;
                                memset(vram + startByte, cn, (storeEnd >> 2) - startByte);
                            }
                            storeStart = xLo;
                        }
                        store = true;
                    }
                }
                if (xRo >= xRn)
                    break;
                xLo = xRo;
                co = so->_c;
                ++so;
                xRo = so->_x;
            } while (true);
            if (store)
                storeEnd = xRn;
            if (storeStart < storeEnd) {
                int startByte = storeStart >> 2;
                memset(vram + startByte, cn, (storeEnd >> 2) - startByte);
                if ((xRn & 3) != 0) {
                    partial = cn & ~mask[xRn & 3];
                    havePartial = true;
                }
                else
                    havePartial = false;
            }
            else {
                if (co != cn && (xRn & 3) != 0) {
                    partial = cn & ~mask[xRn & 3];
                    havePartial = true;
                }
                else
                    havePartial = false;
            }
        }
        xLn = xRn;
        cn = sn->_c;
        ++sn;
        xRn = sn->_x;
        if (xRo == xLn) {
            xLo = xRo;
            co = so->_c;
            ++so;
            xRo = so->_x;
        }
    } while (xLn < 0xff);
}

