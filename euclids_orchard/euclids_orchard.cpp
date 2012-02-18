#include "alfe/main.h"
#include "alfe/file.h"

class Program : public ProgramBase
{
    void run()
    {
        //Vector f(492, 285);
        Vector f(625, 312);
        Vector i = f*16;
        int s = 16;
        Array<Byte> d(i.x*i.y);
        for (int y = 0; y < i.y; ++y)
            for (int x = 0; x < i.x; ++x)
                d[y*i.x+x] = 0;
        for (int q = 2; q <= 2*i.y; ++q) {
            int hh = 2*i.y/q;
            for (int n = 1; n < q; ++n) {
                int x = i.x*n/q;
                int t = 8*s/q;
                if (t > 8)
                    t = 8;
                for (int y = 0; y < hh; ++y)
                    for (int xx = -t; xx <= t; ++xx) {
                        int xxx = x + xx;
                        if (xxx >= 0 && xxx < i.x)
                            d[y*i.x+x+xx] = 255;
                    }
            }
        }
        File("orchard.raw").save(d);
    }
};
