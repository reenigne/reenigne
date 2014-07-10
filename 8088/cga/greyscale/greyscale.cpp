#include "alfe/main.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        String data = File(_arguments[1], true).contents();
        String cgaROM = File("q:\\external\\8088\\roms\\5788005.u33", true).contents();
        //for (int y = 0; y < 100; ++y) {
        //    console.write("  dw ");
        //    for (int x = 0; x < 80; ++x) {
        //        int bestPair = 0;
        //        int bestScore = 0x7fffffff;
        //        for (int pair = 0; pair < 0x10000; ++pair) {
        //            int score = 0;
        //            for (int yy = 0; yy < 2; ++yy) {
        //                int error = 0;
        //                int bits = cgaROM[(3*256 + (pair & 0xff))*8 + yy];
        //                for (int xx = 0; xx < 8; ++xx) {
        //                    int target = data[(y*2 + yy)*640 + x*8 + xx] + error;
        //                    int test = (pair >> 12) & 0xf;
        //                    if ((bits & (128 >> xx)) != 0)
        //                        test = (pair >> 8) & 0xf;
        //                    test *= 17;
        //                    score += (test - target)*(test - target);
        //                    error = target - test;
        //                }
        //            }
        //            if (score < bestScore) {
        //                bestScore = score;
        //                bestPair = pair;
        //            }
        //        }
        //        console.write(String(hex(bestPair, 4)));
        //        if (x < 79)
        //            console.write(", ");
        //    }
        //    console.write("\n");
        //}

        for (int y = 0; y < 200; ++y) {
            console.write("  dw ");
            for (int x = 0; x < 40; ++x) {
                int bestPair = 0;
                int bestScore = 0x7fffffff;
                for (int pair = 0; pair < 0x10000; ++pair) {
                    int score = 0;
                    int error = 0;
                    int bits = cgaROM[(3*256 + (pair & 0xff))*8];
                    for (int xx = 0; xx < 8; ++xx) {
                        int target = data[y*320 + x*8 + xx] + error;
                        int test = (pair >> 12) & 0xf;
                        if ((bits & (128 >> xx)) != 0)
                            test = (pair >> 8) & 0xf;
                        test *= 17;
                        score += (test - target)*(test - target);
                        error = target - test;
                    }
                    if (score < bestScore) {
                        bestScore = score;
                        bestPair = pair;
                    }
                }
                console.write(String(hex(bestPair, 4)));
                if (x < 39)
                    console.write(", ");
            }
            console.write("\n");
        }

    }

};