#include "alfe/main.h"
#include "alfe/colour_space.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        int palette[16 * 3] = {
            0, 0, 0,
            0, 0, 2,
            0, 2, 0,
            0, 2, 2,
            2, 0, 0,
            2, 0, 2,
            2, 1, 0,
            2, 2, 2,
            1, 1, 1,
            1, 1, 3,
            1, 3, 1,
            1, 3, 3,
            3, 1, 1,
            3, 1, 3,
            3, 3, 1,
            3, 3, 3 };

        Linearizer linearizer;

        for (int i = 0; i < 16; ++i)
            for (int j = 0; j < 16; ++j) {
                float r = (3*linearizer.linear(palette[i * 3 + 0]*0x55) + linearizer.linear(palette[j * 3 + 0]*0x55))/4;
                float g = (3*linearizer.linear(palette[i * 3 + 1] * 0x55) + linearizer.linear(palette[j * 3 + 1] * 0x55)) / 4;
                float b = (3*linearizer.linear(palette[i * 3 + 2] * 0x55) + linearizer.linear(palette[j * 3 + 2] * 0x55)) / 4;
                printf("%f %f %f\n", r, g, b);
            }
    }
};