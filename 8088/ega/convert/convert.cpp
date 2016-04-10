#include "alfe/main.h"
#include "alfe/bitmap.h"
#include "alfe/bitmap_png.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] +
                " <input file name.png>\n");
            return;
        }
        int greyToEGA[16] =
            {0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15};
        auto input = PNGFileFormat<DWORD>().load(File(_arguments[1], true));
        Array<Byte> output(80*200*4);
        for (int plane = 0; plane < 4; ++plane) {
            auto row = input.data();
            for (int y = 0; y < 200; ++y) {
                auto p = reinterpret_cast<DWORD*>(row);
                for (int x = 0; x < 80; ++x) {
                    Byte b = 0;
                    for (int xx = 0; xx < 8; ++xx) {
                        Byte i = (p[x*8 + xx] >> 8) & 0xff;
                        if ((greyToEGA[i >> 4] & (1 << plane)) != 0)
                            b |= 128 >> xx;
                    }
                    output[(plane*200 + y)*80 + x] = b;
                }
                row += input.stride();
            }
        }
        File("image.dat").openWrite().write(output);
    }
};