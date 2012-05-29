#include "alfe/main.h"
#include <stdio.h>

class Program : public ProgramBase
{
protected:
    void run()
    {
        FILE* out = fopen("rom.raw", "wb");
        for (int y = 0; y < 8*16; ++y)
            for (int x = 0; x < 8*16; ++x) {
                int ch = (y/8)*16 + (x/8);
                int xx = x % 8;
                int yy = y % 8;
                int bit;
                if (yy < 4)
                    bit = xx;
                else
                    bit = (yy & 1)*4 + (xx/2);
                int dot = ((ch & (1 << bit)) != 0 ? 255 : 0);
                fputc(dot, out);
            }
        fclose(out);
    }
};