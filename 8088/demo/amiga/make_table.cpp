#include "alfe/main.h"
#include "alfe/vectors.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        Array<Word> cga(16384);
        FileStream output = File("tables.asm").openWrite();
        for (int frame = 0; frame < 8; ++frame) {
            String data = File("..\\..\\..\\..\\external\\demo_assets\\shadow_pal1low_frames\\" +
                decimal(frame + 1) + ".raw").contents();
            for (int y = 0; y < 80; ++y) {
                for (int x = 0; x < 96; x += 4) {
                    int p = ((y&1) << 13) | ((((y >> 1) + frame*40)*96/4) + (x >> 2));
                    Byte b = 0;
                    for (int xx = 0; xx < 4; ++xx)
                        b |= data[y * 96 + x + xx]<<((3 - xx) << 1);
                    cga[p] = b;
                }
            }
        }
        output.write("align 16\n\n");
        output.write("frameData:");
        for (int b = 0; b < cga.count(); b += 2) {
            if ((b % 24) == 0)
                output.write("\n  dw ");
            else
                output.write(", ");
            output.write(String(hex(cga[b] | (cga[b+1] << 8), 4)));
        }
        output.write("\n\nunrolledCode:\n");
    }
};
