#include "alfe/main.h"
#include "alfe/complex.h"

float sinc(float z)
{
    if (z == 0.0f)
        return 1.0f;
    z *= M_PI;
    return sin(z)/z;
}

float lanczos(float z)
{
    if (z < -3 || z > 3)
        return 0;
    return sinc(z)*sinc(z/3);
}

template<class T> Byte checkClamp(T x)
{
    int y = static_cast<int>(x);
    //static bool clamped = false;
    //if (!clamped) {
    //    if (y < 0 || y > 0xff) {
    //        clamped = true;
    //        printf("Clamped!\n");
    //    }
    //}
    return clamp(0, y, 255);
}

Complex<float> rotor(float phase)
{
    static const float tau = 2*M_PI;
    float angle = phase*tau;
    return Complex<float>(cos(angle), sin(angle)); 
    // return exp(Complex<float>(0, phase*tau));
}

class NTSCDecoder
{
public:
    NTSCDecoder() { }

    void decode()
    {

    }
private:
    Byte* _input;
    Byte* _output;
};

class Program : public ProgramBase
{
public:
    void run()
    {
        // Settings

        static const int samples = 450*1024;
        static const int sampleSpaceBefore = 256;
        static const int sampleSpaceAfter = 256;
        static const int lines = 240;
        static const int nominalSamplesPerLine = 1820;
        static const int firstSyncSample = -130;  // Assumed position of previous hsync before our samples started
        static const int pixelsPerLine = 760;
        static const float kernelSize = 3;  // Lanczos parameter
        static const int nominalSamplesPerCycle = 8;
        static const int driftSamples = 40;
        static const float contrast = 240.0/(159 - 37);
        static const float brightness = -(37 * contrast);
        static const float saturation = 0.25;
        static const float hue = 0;
        static const int burstSamples = 40;
        static const int firstBurstSample = 192;
        static const float tau = 2*M_PI;
        static const int burstCenter = firstBurstSample + burstSamples/2;

        // Pass 0 - read data

        //String name = "q:\\input.raw";
        String name = "q:\\colour_bars.raw";
        AutoHandle h = File(name, true).openPipe();
//        AutoHandle h = File("\\\\.\\pipe\\vbicap", true).openPipe();
//        h.write<int>(1);

        Array<Byte> buffer(sampleSpaceBefore + samples + sampleSpaceAfter);
        Byte* b = &buffer[0] + sampleSpaceBefore;
        for (int i = 0; i < sampleSpaceBefore; ++i)
            b[i - sampleSpaceBefore] = 0;
        for (int i = 0; i < sampleSpaceAfter; ++i)
            b[i + samples] = 0;
        for (int i = 0; i < 450; ++i)
            h.read(&b[i*1024], 1024);


        // Pass 1 - find sync and burst pulses, compute wobble amplitude and phase

        float deltaSamplesPerCycle = 0;

        int syncPositions[lines + 1];
        int oldP = firstSyncSample - driftSamples;
        int p = oldP + nominalSamplesPerLine;
        float samplesPerLine = nominalSamplesPerLine;
        Complex<float> bursts[lines + 1];
        float burstDCs[lines + 1];
        for (int line = 0; line < lines + 1; ++line) {
            Complex<float> burst = 0;
            float burstDC = 0;
            for (int i = firstBurstSample; i < firstBurstSample + burstSamples; ++i) {
                int sample = b[oldP + i];
                float phase = ((oldP + i)&7)/8.0 + (33.0 + hue)/360;
                burst += rotor(phase)*sample;
                burstDC += sample;
            }
            bursts[line] = burst/burstSamples;
            burstDCs[line] = burstDC/burstSamples;

            syncPositions[line] = p;
            oldP = p;
            for (int i = 0; i < driftSamples*2; ++i) {
                if (b[p] == 0)
                    break;
                ++p;
            }
            p += nominalSamplesPerLine - driftSamples;

            samplesPerLine = (2*samplesPerLine + p - oldP)/3;
        }

        float deltaSamplesPerLine = samplesPerLine - nominalSamplesPerLine;


        // Pass 2 - render

        int pixels = lines*pixelsPerLine;
        Array<Byte> outputBuffer(pixels*3);
        Byte* output = &outputBuffer[0];

        float q = syncPositions[1] - samplesPerLine;
        Complex<float> burst = bursts[0];
        for (int line = 0; line < lines; ++line) {
            // Determine the phase, amplitude and DC offset of the color signal
            // from the color burst, which starts shortly after the horizontal
            // sync pulse ends. The color burst is 9 cycles long, and we look
            // at the middle 5 cycles.

            float contrast1 = contrast;
            int samplesPerLineInt = samplesPerLine;
            float phase = samplesPerLine - (samplesPerLineInt & ~7);
            Complex<float> expectedBurst = burst*rotor(phase/nominalSamplesPerCycle);
            Complex<float> actualBurst = bursts[line];
            burst = (expectedBurst*2 + actualBurst)/3;

            float phaseDifference = (actualBurst*(expectedBurst.conjugate())).argument()/tau;
            float adjust = -phaseDifference/pixelsPerLine;

            Complex<float> chromaAdjust = burst.conjugate()*contrast1*saturation;
            float brightness1 = brightness;

            // Resample the image data

            float samplesPerLine = nominalSamplesPerLine + deltaSamplesPerLine;
            for (int x = 0; x < pixelsPerLine; ++x) {
                float y = 0;
                Complex<float> c = 0;
                float t = 0;

                float kFrac0 = x*samplesPerLine/pixelsPerLine;
                float kFrac = q + kFrac0;
                int k = kFrac;
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
                    float z = s*b[i];
                    y += z;
                    c += rotor((i&7)/8.0)*z*saturation;
                    t += s;
                }

                float wobble = 1;

                y = y*contrast1*wobble/t + brightness1;
                c = c*chromaAdjust*rotor((x - burstCenter*pixelsPerLine/samplesPerLine)*adjust)*wobble/t;

                output[0] = checkClamp(y + 0.9563*c.x + 0.6210*c.y);
                output[1] = checkClamp(y - 0.2721*c.x - 0.6474*c.y);
                output[2] = checkClamp(y - 1.1069*c.x + 1.7046*c.y);
                output += 3;
            }

            int p = syncPositions[line + 1];
            int actualSamplesPerLine = p - syncPositions[line];
            samplesPerLine = (2*samplesPerLine + actualSamplesPerLine)/3;
            q += samplesPerLine;
            q = (10*q + p)/11;

//            printf("Line %i: q=%f, p=%i, samplesPerLine=%f, adjust = %f, cyclesPerLine = %f\n",line, q, p, samplesPerLine, adjust*pixelsPerLine, cyclesPerLine);
        }


        // Pass 3 - write data

        AutoHandle out = File("q:\\output.raw", true).openWrite();
        out.write(&outputBuffer[0], pixels*3);
    }
};