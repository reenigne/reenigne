#include "alfe/main.h"
#include "alfe/vectors.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        FileHandle output = File("tables.asm").openWrite();
        output.write("sineTable:");
        for (int y = 0; y < 838 + 199; ++y) {
            int x = static_cast<int>(78 + 78 * sin(5 * tau*y / 256));
            if (x >= 157)
                x = 156;
            if (x < 0)
                x = 0;
            if ((y & 7) == 0)
                output.write("\n  dw ");
            else
                output.write(", ");
            output.write(String(hex(x*2, 4)));
        }
        //output.write("\npixelTable:");
        //for (int x = 0; x < 320; ++x) {
        //    int xx = x % 160;
        //    
        //}
        output.write("\n\nunrolledCode:\n");
    }
};
