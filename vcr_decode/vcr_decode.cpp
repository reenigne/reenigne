#include "alfe/main.h"
#include "alfe/complex.h"
#include "fftw3.h"

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
        Complex<float> localOscillatorFrequency = unit(4.4f*11/315);
        for (int i = 0; i < 1820*2000; ++i) {
            heterodyned[i] = static_cast<float>(input[i])*localOscillator;
            localOscillator *= localOscillatorFrequency;
        }

        Array<Byte> outputb(2048*2000);

        Array<Complex<float>> fftData(2048);
        fftwf_plan forward = fftwf_plan_dft_1d(2048, reinterpret_cast<fftwf_complex*>(&fftData[0]), reinterpret_cast<fftwf_complex*>(&fftData[0]), -1, FFTW_MEASURE);
        fftwf_plan backward = fftwf_plan_dft_1d(2048, reinterpret_cast<fftwf_complex*>(&fftData[0]), reinterpret_cast<fftwf_complex*>(&fftData[0]), 1, FFTW_MEASURE);

        static const float cutoff = 3.3f*2048*11/315;

        float lastPhase = 0;
        for (int i = 0; i < 1999; ++i) {
            for (int x = 0; x < 2048; ++x)
                fftData[x] = heterodyned[i*1820 + x];
            fftwf_execute(forward);
            for (int x = cutoff; x < 2048 - cutoff; ++x)
                fftData[x] = 0;
            fftwf_execute(backward);
            for (int x = 0; x < 2048; ++x) {
                //outputb[i*2048*3 + x*3 + 0] = byteClamp(128 + fftData[x].x/10);
                //outputb[i*2048*3 + x*3 + 1] = byteClamp(128 + fftData[x].y/10);
                //outputb[i*2048*3 + x*3 + 2] = 0;
                //outputb[i*2048 + x] = byteClamp(fftData[x].modulus()/10);

                float phase = fftData[x].argument() / static_cast<float>(tau);
                double deltaPhase = 1.5 + phase - lastPhase;
                int deltaPhaseInt = static_cast<int>(deltaPhase);
                deltaPhase -= deltaPhaseInt;
                lastPhase = phase;
                deltaPhase -= 0.51f;
                deltaPhase *= 57.0f/2;
                deltaPhase += 0.5f;
                outputb[i*2048 + x] = byteClamp(255 - 255*deltaPhase);
            }
        }

        FileHandle h = File("D:\\t\\vcr_decoded.raw", true).openWrite();
        h.write(outputb);
    }
};