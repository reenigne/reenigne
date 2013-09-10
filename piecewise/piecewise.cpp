#include "alfe/main.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        File inFile("../../Projects/Electronics/music5000/back_traces.raw");
        File outFile("../../Projects/Electronics/music5000/back_traces_interpolated.raw");

        static const int width = 1648;
        static const int inHeight = 1026;
        static const int outHeight = 1043;

        Array<Byte> inData(width*inHeight);
        inFile.openRead().read(&inData[0], width*inHeight);
        Array<Byte> outData(width*outHeight);
        static const int back[]  = {37, 110, 186, 260, 334, 403, 532, 602, 687, 758, 844, 913, 966};
        static const int front[] = {30, 101, 169, 243, 312, 382, 512, 584, 678, 751, 843, 919, 972};
        int e = 0;
        for (int yF = 0; yF < outHeight; ++yF) {
            if (yF >= front[e + 1] && e < 11)
                ++e;
            double f0 = front[e];
            double f1 = front[e+1];
            double b0 = back[e];
            double b1 = back[e+1];
            double p = (yF - f0) / (f1 - f0);
            int yB = clamp(0, static_cast<int>(p*(b1 - b0) + b0), inHeight - 1);
            for (int x = 0; x < width; ++x)
                outData[yF*width + x] = inData[yB*width + x];
        }
        outFile.openWrite().write(outData);
    }
};