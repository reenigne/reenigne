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
        FileHandle inputHandle = File("D:\\t\\pictures\\reenigne\\c2.raw", true).openRead();
        int nn = inputHandle.size();

        FileHandle h = File("D:\\t\\vcr_decoded.raw", true).openWrite();

        int samplesPerFrame = 1824*253;
        Array<Byte> input(samplesPerFrame);
        Array<Complex<float>> fftData(2048);
        fftwf_plan forward = fftwf_plan_dft_1d(2048, reinterpret_cast<fftwf_complex*>(&fftData[0]), reinterpret_cast<fftwf_complex*>(&fftData[0]), -1, FFTW_MEASURE);
        fftwf_plan backward = fftwf_plan_dft_1d(2048, reinterpret_cast<fftwf_complex*>(&fftData[0]), reinterpret_cast<fftwf_complex*>(&fftData[0]), 1, FFTW_MEASURE);
        Complex<float> localOscillator = 1;
        Complex<float> localOscillatorFrequency = unit(4.4f*11/315);

        static const float cutoff = 3.3f*2048*11/315;

        Array<Byte> outputb(2048);

        int frame = 0;
        while (nn > 0) {
            inputHandle.read(&input[0], min(nn, samplesPerFrame));
            nn -= samplesPerFrame;

            int p = 0;
            for (int y = 0; y < 253; ++y) {
                float pFrac;
                int x;
                int lows = 0;
                for (x = 0; x < 1820; ++x) {
                    if (input[p] < 9)
                        ++lows;
                    if (lows > 20)
                        break;
                    ++p;
                    if (p >= samplesPerFrame - 1)
                        break;
                }
                for (x = 0; x < 1820; ++x) {
                    if (input[p] >= 12)
                        break;
                    ++p;
                    if (p >= samplesPerFrame - 1)
                        break;
                }
                int d = input[p] - input[p - 1];
                if (d == 0)
                    pFrac = 0;
                else
                    pFrac = static_cast<float>(input[p] - 12)/d;
                // 12 position is input[p - pFrac]
                    
                for (x = 0; x < 1820; ++x) {
                    if (p + x - 64 >= samplesPerFrame)
                        break;
                    fftData[x] = input[p + x - 64];
                }
                for (; x < 2048; ++x)
                    fftData[x] = 0;

                // TODO: shift by pFrac

                Complex<float> localOscillator = 1;
                for (int i = 0; i < 2048; ++i) {
                    fftData[i] *= localOscillator;
                    localOscillator *= localOscillatorFrequency;
                }
                fftwf_execute(forward);
                for (int x = cutoff; x < 2048 - cutoff; ++x)
                    fftData[x] = 0;
                fftwf_execute(backward);
                float lastPhase = 0;
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
                    outputb[x] = byteClamp(255 - 255*deltaPhase);

                    fftData[x] = 255 - 255*deltaPhase;
                }
                fftwf_execute(forward);
                float highestAmplitude = 0;
                int strongestFrequency = 0;
                for (int x = 0; x < 2048; ++x) {
                    float amplitude = fftData[x].modulus();
                    if (amplitude > highestAmplitude) {
                        highestAmplitude = amplitude;
                        strongestFrequency = x;
                    }
                }
                printf("frame %i line %i strongest frequency %i amplitude %f\n", frame, y, strongestFrequency, highestAmplitude);

                h.write(outputb);
            }
            ++frame;
        }
    }
};