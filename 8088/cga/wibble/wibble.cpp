#include "alfe/main.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        String s = File("Q:\\Projects\\code\\8088mph\\wibble\\t\\wibdata.dat", true).contents();
        for (int f = 0; f < 1000; ++f) {
            int cx = 0;
            for (int y = 0; y < 200; ++y) {
                Byte bb = s[f*200 + y];
                char al = bb;
                int ax = al;
                if (y == 0)
                    cx = ax;
                else
                    cx += ax;
                if (cx < -50 || cx >= 150)
                    printf("Error!\n");

            }
        }
    }
};
