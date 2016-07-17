#include <math.h>
#include <dos.h>
static const double tau = 6.283185307179586476925286766559;

typedef unsigned char Byte;

void main(void)
{
    union REGS regs;
    regs.x.ax = 0x13;
    int86(0x10, &regs, &regs);

    Byte* rn = (Byte*)0x100;
    Byte __far* d = (Byte __far*)MK_FP(0x9000,0);
    volatile Byte __far* v = (volatile Byte __far*)MK_FP(0xa000,0);

    int i;
    for (i = 0; i < 0x10000; ++i)
        v[i] = i;

    for (i = 0; i < 0x10000; ++i)
        d[i] = 128;
    for (int xf = 1; xf < 25; ++xf) {
        for (int yf = 1; yf < 25; ++yf) {
            float phase = *rn++;
            float amplitude = (*rn++)/(float)(4*xf*yf);
            for (int y = 0; y < 256; ++y) {
                for (int x = 0; x < 256; ++x)
                    d[y*256 + x] += sin((x*xf + y*yf + phase)*tau/256)*amplitude;
            }
        }
    }
    int xp = 0;
    do {
        for (int x = 0; x < 320; ++x) {
            int maxY = 0;
            float theta = (x + xp)/320.0f*tau/4;
            for (int r = 1; r < 100; ++r) {
                int xh = (xp + (int)(r*sin(theta))) & 0xff;
                int yh = (int)(r*cos(theta)) & 0xff;
                int h = d[yh*256 + xh];
                int y = (int)(25*(h - 256)/r) + 180;
                for (int yy = maxY; yy <= y; ++yy)
                    v[(199-yy)*320 + x] = d[yh*256 + ((xh + 128)&0xff)];
                if (y > maxY)
                    maxY = y;
            }
            for (int yy = maxY; yy < 200; ++yy)
                v[(199-yy)*320 + x] = 0;
        }
        ++xp;
    } while (1);
}

