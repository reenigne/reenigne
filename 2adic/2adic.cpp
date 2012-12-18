#include "alfe\main.h"

UInt32 divide2adic(UInt32 a, UInt32 b)
{
    if (b == 0)
        return 0; //a/b;
    while ((b & 1) == 0) {
        a >>= 1;
        b >>= 1;
    }
    UInt32 q = 0;
    UInt32 d = 1;
    while (a != 0 && d != 0) {
        if ((a & 1) != 0) {
            q |= d;
            a -= b;
        }
        d <<= 1;
        a >>= 1;
    }
    return q;
}

UInt32 divide(UInt32 a, UInt32 b)
{
    if (b == 0)
        return 0;
    return a/b;
}

UInt32 mod2adic(UInt32 a, UInt32 b)
{
    if (b == 0)
        return 0;
    return a - b*divide2adic(a, b);
}

UInt32 mod(UInt32 a, UInt32 b)
{
    if (b == 0)
        return 0;
    return a%b;
}

UInt32 mul(UInt32 a, UInt32 b)
{
    return a*b;
}

class Program : public ProgramBase
{
public:
    void makeImage(String name, UInt32 (*function)(UInt32, UInt32))
    {
        Array<UInt8> a(0x10000);
        UInt8* p = &a[0];
        for (int y = 0; y < 0x100; ++y)
            for (int x = 0; x < 0x100; ++x) {
                *p = static_cast<UInt8>(function(x, y));
                ++p;
            }
        File(name).save(&a[0], 0x10000);
    }

    void addColour(UInt8*&p, DWord c)
    {
        *p = (c >> 16);
        ++p;
        *p = (c >> 8);
        ++p;
        *p = c;
        ++p;
    }
    
    void run()
    {
        Array<DWord> hues(0x200);
        for (int c = 0; c < 0x200; ++c) {
            int r = clamp(0, static_cast<int>(sin(           c*M_PI*2/0x200)*128.0+128.0), 255);
            int g = clamp(0, static_cast<int>(sin(2*M_PI/3 + c*M_PI*2/0x200)*128.0+128.0), 255);
            int b = clamp(0, static_cast<int>(sin(4*M_PI/3 + c*M_PI*2/0x200)*128.0+128.0), 255);
            hues[c] = (r << 16) + (g << 8) + b;
        }

        Array<UInt8> a(3*0x1000*0x1000);
        UInt8* p;
#if 0
        makeImage("divide2adic.raw", divide2adic);
        makeImage("divide.raw", divide);
        makeImage("mod2adic.raw", mod2adic);
        makeImage("mod.raw", mod);
        makeImage("mul.raw", mul);

        p = &a[0];
        for (int y = 0; y < 0x1000; ++y)
            for (int x = 0; x < 0x1000; ++x) {
                UInt32 q = divide2adic(x, y);
                *p = ((q & 15)*0x11);
                ++p;
                *p = (((q >> 4) & 15)*0x11);
                ++p;
                *p = (((q >> 8) & 15)*0x11);
                ++p;
            }
        File("divide2adic4096.raw").save(&a[0], 3*0x1000*0x1000);

        p = &a[0];
        for (int y = 0; y < 0x200; ++y)
            for (int x = 0; x < 0x200; ++x) {
                UInt32 q = divide2adic(x, y);
                *p = ((q & 7)*0x49) >> 1;
                ++p;
                *p = (((q >> 3) & 7)*0x49) >> 1;
                ++p;
                *p = (((q >> 6) & 7)*0x49) >> 1;
                ++p;
            }
        File("divide2adic512.raw").save(&a[0], 3*0x200*0x200);

        p = &a[0];
        for (int y = 0; y < 0x40; ++y)
            for (int x = 0; x < 0x40; ++x) {
                UInt32 q = divide2adic(x, y);
                *p = (q & 3)*0x55;
                ++p;
                *p = ((q >> 2) & 3)*0x55;
                ++p;
                *p = ((q >> 4) & 3)*0x55;
                ++p;
            }
        File("divide2adic64.raw").save(&a[0], 3*0x40*0x40);

            p = &a[0];
        for (int y = 0; y < 8; ++y)
            for (int x = 0; x < 8; ++x) {
                UInt32 q = divide2adic(x, y);
                *p = (q & 1)*0xff;
                ++p;
                *p = ((q >> 1) & 1)*0xff;
                ++p;
                *p = ((q >> 2) & 1)*0xff;
                ++p;
            }
        File("divide2adic8.raw").save(&a[0], 3*8*8);

        p = &a[0];
        for (int y = 0; y < 0x100; ++y)
            for (int x = 0; x < 0x100; ++x) {
                UInt32 q = static_cast<UInt8>(divide2adic(x, y));
                *p = static_cast<UInt8>(((q & 0x80) >> 7) | ((q & 0x40) >> 5) | ((q & 0x20) >> 3) | ((q & 0x10) >> 1) | ((q & 8) << 1) | ((q & 4) << 3) | ((q & 2) << 5) | (q << 7));
                ++p;
            }
        File("divide2adic_r.raw").save(&a[0], 0x10000);

        p = &a[0];
        for (int y = 0; y < 0x200; ++y)
            for (int x = 0; x < 0x200; ++x) {
                UInt32 q = divide2adic(x, y);
                q = (((q & 0x100) >> 8) | ((q & 0x80) >> 6) | ((q & 0x40) >> 4) | ((q & 0x20) >> 2) | (q & 0x10) | ((q & 8) << 2) | ((q & 4) << 4) | ((q & 2) << 6) | (q << 8));
                *p = ((q & 1) >> 0)*0x92 + ((q & 8) >> 3)*0x49 + ((q & 0x40) >> 6)*0x24;
                ++p;
                *p = ((q & 2) >> 1)*0x92 + ((q & 0x10) >> 4)*0x49 + ((q & 0x80) >> 7)*0x24;
                ++p;
                *p = ((q & 4) >> 2)*0x92 + ((q & 0x20) >> 5)*0x49 + ((q & 0x100) >> 8)*0x24;
                ++p;
            }
        File("divide2adic512_r.raw").save(&a[0], 3*0x200*0x200);

        p = &a[0];
        for (int y = 0; y < 0x100; ++y)
            for (int x = 0; x < 0x100; ++x) {
                UInt32 q = static_cast<UInt8>(divide2adic(x, y));
                addColour(p, hues[((q & 0x80) >> 6) | ((q & 0x40) >> 4) | ((q & 0x20) >> 2) | ((q & 0x10) >> 0) | ((q & 8) << 2) | ((q & 4) << 4) | ((q & 2) << 6) | ((q & 1) << 8)]);
            }
        File("divide2adic_c.raw").save(&a[0], 3*0x100*0x100);

        p = &a[0];
        for (int y = 0; y < 0x100; ++y)
            for (int x = 0; x < 0x100; ++x) {
                UInt32 q = static_cast<UInt8>(divide2adic(x, y));
                addColour(p, hues[(q << 1) & 0x1ff]);
            }
        File("divide2adic_fc.raw").save(&a[0], 3*0x100*0x100);

        p = &a[0];
        for (int y = 0; y < 0x200; ++y)
            for (int x = 0; x < 0x200; ++x) {
                UInt32 q = static_cast<UInt8>(divide2adic(x, y));
                addColour(p, hues[((q & 0x100) >> 8) | ((q & 0x80) >> 6) | ((q & 0x40) >> 4) | ((q & 0x20) >> 2) | ((q & 0x10) >> 0) | ((q & 8) << 2) | ((q & 4) << 4) | ((q & 2) << 6) | ((q & 1) << 8)]);
            }
        File("divide2adic512_c.raw").save(&a[0], 3*0x200*0x200);

        p = &a[0];
        for (int y = 0; y < 0x100; ++y)
            for (int x = 0; x < 0x100; ++x) {
                UInt32 q = static_cast<UInt8>(mul(x, y));
                addColour(p, hues[((q & 0x80) >> 6) | ((q & 0x40) >> 4) | ((q & 0x20) >> 2) | ((q & 0x10) >> 0) | ((q & 8) << 2) | ((q & 4) << 4) | ((q & 2) << 6) | ((q & 1) << 8)]);
            }
        File("mul_rc.raw").save(&a[0], 3*0x100*0x100);

        p = &a[0];
        for (int y = 0; y < 0x100; ++y)
            for (int x = 0; x < 0x100; ++x) {
                UInt32 q = static_cast<UInt8>(mul(x, y));
                addColour(p, hues[(q << 1)&0x1ff]);
            }
        File("mul_fc.raw").save(&a[0], 3*0x100*0x100);

        p = &a[0];
        for (int y = 0; y < 0x100; ++y)
            for (int x = 0; x < 0x100; ++x) {
                UInt32 q = static_cast<UInt8>(divide2adic(x*0x10, y));
                addColour(p, hues[((q & 0x80) >> 6) | ((q & 0x40) >> 4) | ((q & 0x20) >> 2) | ((q & 0x10) >> 0) | ((q & 8) << 2) | ((q & 4) << 4) | ((q & 2) << 6) | ((q & 1) << 8)]);
            }
        File("divide2adic_c_fixed.raw").save(&a[0], 3*0x100*0x100);

        p = &a[0];
        for (int y = 0; y < 0x100; ++y)
            for (int x = 0; x < 0x100; ++x) {
                UInt32 q = static_cast<UInt8>(divide2adic(x*0x10, y));
                addColour(p, hues[(q << 1) & 0x1ff]);
            }
        File("divide2adic_fc_fixed.raw").save(&a[0], 3*0x100*0x100);

        p = &a[0];
        for (int y = 0; y < 0x100; ++y)
            for (int x = 0; x < 0x100; ++x) {
                UInt32 q = static_cast<UInt8>(divide2adic(x*0x101, y));
                addColour(p, hues[((q & 0x80) >> 6) | ((q & 0x40) >> 4) | ((q & 0x20) >> 2) | ((q & 0x10) >> 0) | ((q & 8) << 2) | ((q & 4) << 4) | ((q & 2) << 6) | ((q & 1) << 8)]);
            }
        File("divide2adic_c_ffixed.raw").save(&a[0], 3*0x100*0x100);

        p = &a[0];
        for (int y = 0; y < 0x100; ++y)
            for (int x = 0; x < 0x100; ++x) {
                UInt32 q = static_cast<UInt8>(divide2adic(x*0x101, y));
                addColour(p, hues[(q << 1) & 0x1ff]);
            }
        File("divide2adic_fc_ffixed.raw").save(&a[0], 3*0x100*0x100);
#endif
        p = &a[0];
        for (int y = 0; y < 0x100; ++y)
            for (int x = 0; x < 0x100; ++x) {
                UInt32 q = static_cast<UInt8>(divide2adic(1, y));
                addColour(p, hues[(q << 1) & 0x1ff]);
            }
        File("divide2adic_fc_ffixed.raw").save(&a[0], 3*0x100*0x100);
    }
};