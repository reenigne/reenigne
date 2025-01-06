#include "alfe/main.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        File in("Q:\\canabaltbg_320x200.raw", true);
        Array<Byte> bg(320 * 200);
        in.openRead().read(&bg[0], 320 * 200);
        Array<Byte> pixels(320 * 200);
        for (int y = 0; y < 200; ++y) {
            for (int x = 0; x < 320; ++x) {
                int c = bg[y * 320 + x];
                int p = 0;
                switch (c) {
                    //case 2: p = (((x + 200 - y) % 3) == 0 ? 2 : 1); break;
                    case 2: p = (((1 + x + y*2) & 3) == 0 ? 2 : 1); break;
                    //case 2: p = 3; break;
                    case 1: p = (((x + y) & 1) != 0 ? 1 : 2); break;
                    case 0: p = (((x + y) % 3) == 0 ? 1 : 2); break;
                    default: p = 0; break;
                }
                pixels[y*320 + x] = p;
            }
        }
        Array<Byte> lines(80 * 200);
        for (int y = 0; y < 200; ++y) {
            for (int x = 0; x < 80; ++x) {
                int b = 0;
                for (int p = 0; p < 4; ++p) {
                    int c = pixels[y * 320 + (x << 2) + p];
                    b += (c << (6 - (p << 1)));
                }
                lines[y * 80 + x] = b;
            }
        }
        File("Q:\\canabaltbg_vram.dat", true).openWrite().write(lines);
    }
};