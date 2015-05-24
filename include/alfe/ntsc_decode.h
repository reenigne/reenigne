#include "alfe/main.h"
#include "alfe/bitmap.h"

#ifndef INCLUDED_NTSC_DECODE_H
#define INCLUDED_NTSC_DECODE_H

float sinc(float z)
{
    if (z == 0.0f)
        return 1.0f;
    z *= M_PI;
    return sin(z)/z;
}

static const int lobes = 3;

float lanczos(float z)
{
    return sinc(z)*sinc(z/lobes);
}

template<class T> Byte checkClamp(T x)
{
    int y = static_cast<int>(x);
    return clamp(0, y, 255);
//    return x;
}

Complex<float> rotor(float phase)
{
    float angle = static_cast<float>(phase*tau);
    return Complex<float>(cos(angle), sin(angle));
}

template<class T> class NTSCCaptureDecoder
{
public:
    NTSCCaptureDecoder()
    {
        _contrast = 2.3;
        _brightness = -33.0;
        _saturation = 0.34;
        _hue = 0;
        _outputPixelsPerLine = 760;
    }
    void setOutputPixelsPerLine(int outputPixelsPerLine)
    {
        // outputPixelsPerLine  active width  with 20% overscan  per color carrier cycle  active scanlines  with 20% overscan
        //  380                  266+2/3       320                1+2/3                    200               240
        //  456                  320           384                2                        240               288
        //  570                  400           480                2.5                      300               360
        //  760                  533+1/3       640                3+1/3                    400               480
        //  912                  640           768                4                        480               576
        // 1140                  800           960                5                        600               720
        _outputPixelsPerLine = outputPixelsPerLine;
    }
    void setBuffers(Byte* input, Bitmap<T> output)
    {
        _input = input;
        _output = output;
    }
    void setContrast(float contrast) { _contrast = contrast; }
    void setBrightness(float brightness) { _brightness = brightness; }
    void setSaturation(float saturation) { _saturation = saturation; }
    void setHue(float hue) { _hue = hue; }

    void decode()
    {
        // Settings

        static const int lines = 240;
        static const int nominalSamplesPerLine = 1820;
        static const int firstSyncSample = -40;  // Assumed position of previous hsync before our samples started (was -130)
        static const float kernelSize = lobes;  // Lanczos parameter
        static const int nominalSamplesPerCycle = 8;
        static const int driftSamples = 40;
        static const int burstSamples = 64; //40; ?                         // TODO: figure out optimal for real NTSC
        static const int firstBurstSample = 30 + driftSamples;  //118; ?    // TODO: figure out optimal for real NTSC
        static const int burstCenter = firstBurstSample + burstSamples/2;

        Byte* b = _input;


        // Pass 1 - find sync and burst pulses, compute wobble amplitude and phase

        float deltaSamplesPerCycle = 0;

        int syncPositions[lines + 1];
        int fracSyncPositions[lines + 1];
        int oldP = firstSyncSample - driftSamples;                  // -170
        int p = oldP + nominalSamplesPerLine;                       
        float samplesPerLine = nominalSamplesPerLine;               
        Complex<float> bursts[lines + 1];
        float burstDCs[lines + 1];
        Complex<float> hueRotor = rotor((33 + _hue)/360);
        float burstDCAverage = 0;
        float x = 0;
        for (int line = 0; line < lines + 1; ++line) {
            Complex<float> burst = 0;
            float burstDC = 0;
            for (int i = firstBurstSample; i < firstBurstSample + burstSamples; ++i) {
                int j = oldP + i;                                   // 38
                int sample = b[j];
                float phase = (j&7)/8.0f;
                burst += rotor(phase)*sample;
                burstDC += sample;
            }

            float burstAmplitude = burst.modulus()/burstSamples;
            bursts[line] = burst*hueRotor/burstSamples;
            burstDC /= burstSamples;
            burstDCs[line] = burstDC;

            syncPositions[line] = p;
            fracSyncPositions[line] = x;
            oldP = p;
            int i;
            for (i = 0; i < driftSamples*2; ++i) {
                if (b[p] < 9)
                    break;
                ++p;
            }
            for (; i < driftSamples*2; ++i) {
                if (b[p] >= 12)
                    break;
                ++p;
            }
            // b[p] >= 12
            // b[p - 1] < 12
            // b[p - 1]*x + b[p]*(1 - x) == 12
            // x == (b[p] - 12)/(b[p] - b[p-1])
            //   12 position is b[p - x]
            int d = b[p] - b[p - 1];
            if (d == 0)
                x = 0;
            else
                x = (b[p] - 12)/d;

            p += nominalSamplesPerLine - driftSamples;

            if (line < 200) {
                samplesPerLine = (2*samplesPerLine + p - oldP)/3;
                burstDCAverage = (2*burstDCAverage + burstDC)/3;
            }
        }

        float deltaSamplesPerLine = samplesPerLine - nominalSamplesPerLine;


        // Pass 2 - render

        Byte* outputRow = _output.data();

        float q = syncPositions[1] - samplesPerLine;
        syncPositions[0] = q;
        Complex<float> burst = bursts[0];
        float rotorTable[8];
        for (int i = 0; i < 8; ++i)
            rotorTable[i] = rotor(i/8.0).x*_saturation;
        Complex<float> expectedBurst = burst;
        int oldActualSamplesPerLine = nominalSamplesPerLine;
        for (int line = 0; line < lines; ++line) {
            // Determine the phase, amplitude and DC offset of the color signal
            // from the color burst, which starts shortly after the horizontal
            // sync pulse ends. The color burst is 9 cycles long, and we look
            // at the middle 5 cycles.

            float contrast1 = _contrast;
            Complex<float> actualBurst = bursts[line];
            burst = (expectedBurst*2 + actualBurst)/3;

            float phaseDifference = (actualBurst*(expectedBurst.conjugate())).argument()/tau;
            float adjust = -phaseDifference/_outputPixelsPerLine;

            Complex<float> chromaAdjust = burst.conjugate()*contrast1*_saturation;
            burstDCAverage = (2*burstDCAverage + burstDCs[line])/3;
            float brightness1 = _brightness + 65 - burstDCAverage;

            // Resample the image data

            //float samplesPerLine = nominalSamplesPerLine + deltaSamplesPerLine;
            T* output = reinterpret_cast<T*>(outputRow);
            int xStart = 65*_outputPixelsPerLine/760;
            for (int x = xStart; x < xStart + _output.size().x; ++x) {
                float y = 0;
                Complex<float> c = 0;
                float t = 0;

                float kFrac0 = x*samplesPerLine/_outputPixelsPerLine;
                float kFrac = q + kFrac0;
                int k = static_cast<int>(kFrac);
                kFrac -= k;
                float samplesPerCycle = nominalSamplesPerCycle + deltaSamplesPerCycle;
                float z0 = -kFrac/samplesPerCycle;
                int firstInput = -kernelSize*samplesPerCycle + kFrac;
                int lastInput = kernelSize*samplesPerCycle + kFrac;

                for (int j = firstInput; j <= lastInput; ++j) {
                    // The input sample corresponding to the output pixel is k+kFrac
                    // The sample we're looking at in this iteration is j+k
                    // The difference is j-kFrac
                    // So the value we pass to lanczos() is (j-kFrac)/samplesPerCycle
                    // So z0 = -kFrac/samplesPerCycle;

                    float s = lanczos(j/samplesPerCycle + z0);
                    int i = j + k;
                    float z = s*(b[i] - 23);
                    y += z;
                    c.x += rotorTable[i & 7]*z;
                    c.y += rotorTable[(i + 6) & 7]*z;
                    //c += rotor((i&7)/8.0)*z*saturation;
                    t += s;
                }

                y = y*contrast1/t + brightness1;
                c = c*chromaAdjust*rotor((x - burstCenter*_outputPixelsPerLine/samplesPerLine)*adjust)/t;

                setOutput(output, SRGB(
                    checkClamp(y + 0.9563*c.x + 0.6210*c.y),
                    checkClamp(y - 0.2721*c.x - 0.6474*c.y),
                    checkClamp(y - 1.1069*c.x + 1.7046*c.y)));
                ++output;
            }

            int p = syncPositions[line + 1];
            int actualSamplesPerLine = p - syncPositions[line];
            samplesPerLine = (2*samplesPerLine + actualSamplesPerLine + fracSyncPositions[line] - fracSyncPositions[line + 1])/3;
            q += samplesPerLine;
            q = (10*q + p)/11;

            expectedBurst = actualBurst;

            outputRow += _output.stride();
        }
    }
private:
    void setOutput(SRGB* output, SRGB x) { *output = x; }
    void setOutput(UInt32* output, SRGB x)
    {
        *output = (x.x << 16) | (x.y << 8) | x.z;
    }

    int _outputPixelsPerLine;
    float _contrast;
    float _brightness;
    float _saturation;
    float _hue;
    Byte* _input;
    Bitmap<T> _output;
};

#endif // INCLUDED_NTSC_DECODE_H
