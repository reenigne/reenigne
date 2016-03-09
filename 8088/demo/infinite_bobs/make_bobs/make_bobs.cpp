#include "alfe/main.h"

int fade[] = {4, 12, 13, 15, 15};

int bayer[] = {
    0,  8,  2, 10,
   12,  4, 14,  6,
    3, 11,  1,  9,
   15,  7, 13,  5};

//  left 8 yellow
//       4 red
//       2 blue
// right 1 green

class Program : public ProgramBase
{
public:
    void run()
    {
        double yr = 20;
        double xr = 6*yr/5;

        double l = sqrt(1/3.0);
        double lx = -l;
        double ly = -l;
        double lz = l;

        // Maximum l is 1
        // Minimum l is -2*sqrt(1/2)*sqrt(1/3) = -2*sqrt(1/6) = -sqrt(2/3)
        double n = sqrt(2/3.0);

        int colours = 3*16 + 1;

        Array<int> cc(160);
        AppendableArray<Byte> code;
        AppendableArray<Byte> data;

        for (int x0 = 0; x0 < 2; ++x0) {
            for (int y = 0; y < yr*2 + 1; ++y) {
                double yy = (y - (yr-1))/yr;
                double y2 = yy*yy;

                for (int x = 1-xr; x < xr; ++x) {
                    double xx = x/xr;
                    double x2 = xx*xx;
                    double z2 = 1 - (x2 + y2);
                    if (z2 >= 0) {
                        double zz = sqrt(z2);
                        double l = xx*lx + yy*ly + zz*lz;
                        double l1 = (l + n)/(1 + n);
                        int c = clamp(0, static_cast<int>(l1*(colours + 1)),
                            colours - 1);
                        int f = c >> 4;
                        if ((c & 15) > bayer[(y & 3)*4 + (x & 3)])
                            ++f;
                        cc[x - (1-xr)] = fade[f];
                    }
                }

            }
        }
    }
};