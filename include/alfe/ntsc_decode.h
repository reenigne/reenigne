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
        _yScale = 1;
        _doDecode = true;
        _chromaSamples = 8;

        for (int i = 8; i < 40; ++i)
            _burstWeights[i] = 1;
        for (int i = 0; i < 8; ++i) {
            _burstWeights[i] = 1 - (cos(tau*i/16) + 1)/2;
            _burstWeights[i + 40] = (cos(tau*i/16) + 1)/2;
        }
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
    void setInputBuffer(Byte* input) { _input = input; }
    void setOutputBuffer(Bitmap<T> output)  { _output = output; }
    void setContrast(float contrast) { _contrast = contrast; }
    void setBrightness(float brightness) { _brightness = brightness; }
    void setSaturation(float saturation) { _saturation = saturation; }
    void setHue(float hue) { _hue = hue; }
    void setYScale(int yscale) { _yScale = yscale; }
    void setDoDecode(bool doDecode) { _doDecode = doDecode; }
    void setChromaSamples(float samples) { _chromaSamples = samples; }

    void decode()
    {
        if (!_doDecode) {
            outputRaw();
            return;
        }
        // Settings

        static const int lines = 240;
        static const int nominalSamplesPerLine = 1820;
        static const int firstSyncSample = -40;  // Assumed position of previous hsync before our samples started (was -130)
        static const float kernelSize = lobes;  // Lanczos parameter
        static const int nominalSamplesPerCycle = 8;
        static const int driftSamples = 40;
        static const int burstSamples = 48;  // Central 6 of 8-10 cycles
        static const int firstBurstSample = 32 + driftSamples;      // == 72
        static const int burstCenter = firstBurstSample + burstSamples/2;

        Byte* b = _input;


        // Pass 1 - find sync and burst pulses, compute wobble amplitude and phase

        float deltaSamplesPerCycle = 0;

        int syncPositions[lines + 1];
        int fracSyncPositions[lines + 1];
        int oldP = firstSyncSample - driftSamples;                  // == -80
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
            float t = 0;
            for (int i = firstBurstSample; i < firstBurstSample + burstSamples; ++i) {
                int j = oldP + i;                                   // == -8
                int sample = b[j];
                float phase = (j&7)/8.0f;
                float w = 1; //_burstWeights[ i - firstBurstSample];
                burst += rotor(phase)*sample*w;
                t += w;
                burstDC += sample;
            }

            float burstAmplitude = burst.modulus()/burstSamples;
            bursts[line] = burst*hueRotor/burstSamples;
            burstDC /= t;
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
            rotorTable[i] = rotor(i/8.0).x;
        Complex<float> expectedBurst = burst;
        int oldActualSamplesPerLine = nominalSamplesPerLine;
        float contrast1 = _contrast;
        float saturation1 = _saturation*100;
        for (int line = 0; line < lines; ++line) {
            // Determine the phase, amplitude and DC offset of the color signal
            // from the color burst, which starts shortly after the horizontal
            // sync pulse ends. The color burst is 9 cycles long, and we look
            // at the middle 5 cycles.

            Complex<float> actualBurst = bursts[line];
            burst = (expectedBurst*2 + actualBurst)/3;

            float phaseDifference = (actualBurst*(expectedBurst.conjugate())).argument()/tau;
            float adjust = -phaseDifference/_outputPixelsPerLine;

            float bm2 = burst.modulus2();
            Complex<float> chromaAdjust;
            // TODO: Implement proper colour-killer logic (100 scanlines hysterisis?)
            if (bm2 < 100)
                chromaAdjust = 0;
            else
                chromaAdjust = burst.conjugate()*contrast1*saturation1 / bm2;
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
                float z0 = -kFrac/_chromaSamples;             // TODO: is this correct?
                int firstInput = -kernelSize*_chromaSamples + kFrac;
                int lastInput = kernelSize*_chromaSamples + kFrac;

                for (int j = firstInput; j <= lastInput; ++j) {
                    // The input sample corresponding to the output pixel is k+kFrac
                    // The sample we're looking at in this iteration is j+k
                    // The difference is j-kFrac
                    // So the value we pass to lanczos() is (j-kFrac)/_chromaSamples
                    // So z0 = -kFrac/_chromaSamples;

                    float s = lanczos(j/_chromaSamples + z0);
                    int i = j + k;
                    float z = s*b[i];
                    //y += z;
                    c.x += rotorTable[i & 7]*z;
                    c.y += rotorTable[(i + 6) & 7]*z;
                    //c += rotor((i&7)/8.0)*z*saturation;
                    t += s;
                }
                c /= t;

                //float lumaSamples = samplesPerLine/_outputPixelsPerLine;
                float lumaSamples = samplesPerCycle/2;  // i.e. 7.16MHz
                firstInput = -kernelSize*lumaSamples + kFrac;
                lastInput = kernelSize*lumaSamples + kFrac;

                Complex<float> cc = c*2;

                t = 0;
                z0 = -kFrac/lumaSamples;
                for (int j = firstInput; j <= lastInput; ++j) {
                    // The input sample corresponding to the output pixel is k+kFrac
                    // The sample we're looking at in this iteration is j+k
                    // The difference is j-kFrac
                    // So the value we pass to lanczos() is (j-kFrac)/lumaSamples
                    // So z0 = -kFrac/lumaSamples;

                    float s = lanczos(j/lumaSamples + z0);
                    int i = j + k;
                    float z = s*(b[i] - (cc.x*rotorTable[i & 7] + cc.y*rotorTable[(i + 6)&7]));
                    //float z = s*(cc.x*rotorTable[i & 7] + cc.y*rotorTable[(i + 6)&7]);
                    y += z;
                    //c += rotor((i&7)/8.0)*z*saturation;
                    t += s;
                }

                y = y*contrast1/t + brightness1;
                c = c*chromaAdjust*rotor((x - burstCenter*_outputPixelsPerLine/samplesPerLine)*adjust);

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

            Byte* outputRow2 = outputRow + _output.stride();
            for (int yy = 1; yy < _yScale; ++yy) {
                T* output = reinterpret_cast<T*>(outputRow2);
                T* input = reinterpret_cast<T*>(outputRow);
                for (int x = 0; x < _output.size().x; ++x) {
                    *output = *input;
                    ++output;
                    ++input;
                }
                outputRow2 += _output.stride();
            }
            outputRow = outputRow2;
        }
    }
private:
    void outputRaw()
    {
        Byte* outputRow = _output.data();
        Byte* b = _input;
        for (int y = 0; y < 252; ++y) {
            T* output = reinterpret_cast<T*>(outputRow);
            for (int x = 0; x < 1824; ++x) {
                setOutput(output, SRGB(*b, *b, *b));
                ++output;
                ++b;
            }
            outputRow += _output.stride();
        }
        T* output = reinterpret_cast<T*>(outputRow);
        for (int x = 0; x < 450*1024-252*1824; ++x) {
            setOutput(output, SRGB(*b, *b, *b));
            ++output;
            ++b;
        }
    }


    void setOutput(SRGB* output, SRGB x) { *output = x; }
    void setOutput(UInt32* output, SRGB x)
    {
        *output = (x.x << 16) | (x.y << 8) | x.z;
    }
    void setOutput(DWORD* output, SRGB x)
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
    int _yScale;
    bool _doDecode;
    float _chromaSamples;
    float _burstWeights[48];
};

#endif // INCLUDED_NTSC_DECODE_H
