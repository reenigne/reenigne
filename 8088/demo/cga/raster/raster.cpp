#include "unity/main.h"

static const int nBars = 10;
static const int barWidth = 7;
int barColours[barWidth] = {4, 6, 14, 15, 11, 9, 1};

class Program : public ProgramBase
{
public:
    int run()
    {
        int barY[nBars];
        FILE* out = fopen("raster.raw", "wb");
        for (int t = 0; t < 320; ++t) {
            //for (int bar = 0; bar < nBars; ++bar)
            //    barY[bar] =

            for (int y = 0; y < 200; ++y) {

            }
        }
        fclose(out);
        return 0;
    }
};