#include "alfe/main.h"
#include "alfe/complex.h"

float sinc(float z)
{
    if (z == 0.0f)
        return 1.0f;
    z *= M_PI;
    return sin(z)/z;
}

static const int lobes = 35;

float lanczos(float z)
{
    return sinc(z)*sinc(z/lobes);
}

class Program : public ProgramBase
{
public:
    void run()
    {
        Array<Byte> input;
        FileHandle inputHandle = File("D:\\t\\c2.raw", true).openRead();
        int nn = inputHandle.size();
        input.allocate(nn);
        inputHandle.read(&input[0], nn);

        Array<Complex<float>> heterodyned(1820*2000);
        Complex<float> localOscillator = 1;
        Complex<float> localOscillatorFrequency = unit(3.9f*11/315);
        for (int i = 0; i < 1820*2000; ++i) {
            heterodyned[i] = static_cast<float>(input[i])*localOscillator;
            localOscillator *= localOscillatorFrequency;
        }

        Array<Byte> outputb(1820*2000);
        int z = 1;
        float kernelSize = lobes;
        float sampleFrequency = 157500000.0*2/11;
        float cutoffFrequency = 3000000;
        float cutoffSamples = sampleFrequency / (cutoffFrequency * 2); // 2.8
        float lastPhase = 0;

        int firstInput = static_cast<int>(-kernelSize*cutoffSamples);
        int lastInput = static_cast<int>(kernelSize*cutoffSamples);
        for (int x = -firstInput; x < 1820*2000 - lastInput; ++x) {
            Complex<float> v = 0;
            float t = 0;
            for (int j = firstInput; j <= lastInput; ++j) {
                float s = lanczos(j/cutoffSamples);
                Complex<float> z = s*heterodyned[j + x];
                v += z;
                t += s;
            }
            v /= t;

            float phase = v.argument() / static_cast<float>(tau);
            double deltaPhase = 1.5 + phase - lastPhase;
            int deltaPhaseInt = static_cast<int>(deltaPhase);
            deltaPhase -= deltaPhaseInt;
            lastPhase = phase;
            deltaPhase -= 0.5f;
            deltaPhase *= 57.0f/2;
            deltaPhase += 0.5f;
            outputb[x] = byteClamp(255 - 255*deltaPhase);
            //outputb[x] = static_cast<int>(255*phase);
            //outputb[x] = static_cast<int>(heterodyned[x].x);
        }
        FileHandle h = File("D:\\t\\vcr_decoded.raw", true).openWrite();
        h.write(outputb);
    }
};