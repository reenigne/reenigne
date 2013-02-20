#include "alfe/main.h"

bool isSync(Byte* p) { return (*p) == 0; }

// This needs to be less sensitive for non-CGA images
int findHSync(Byte* p)
{
    for (int i = 0; i < 6; ++i) {
        if (isSync(p - 3))
            return i - 3;
        ++p;
    }
    return 3;
}

bool foundHSync(int h) { return h >= -2 && h <= 2; }

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

class Program : public ProgramBase
{
public:
    void run()
    {
        static const int inputSamples = 1024*450;
        static const int nominalSamplesPerLine = 1824;
        static const int outputSamplesPerLine = nominalSamplesPerLine*5/12;
        static const int lines = 240;
        double contrast = 240.0/(159 - 37);
        double brightness = -(37 * contrast);

        // Pass 0 - read the samples from the capture card

        AutoHandle h = File("\\\\.\\pipe\\vbicap", true).openPipe();
        h.write<int>(1);


        Array<Byte> buffer(inputSamples + 512);
        Byte* b = &buffer[0] + 256;
        for (int i = 0; i < 256; ++i) {
            *(b - 256 + i) = 0;
            *(b + inputSamples + i) = 0;
        }
        for (int i = 0; i < 450; ++i)
            h.read(&buffer[i*1024], 1024);


        // Pass 1 - find the hsync pulses and determine the approximate line
        // rate.

        int p;
        for (p = 0; p < inputSamples; ++p)
            if (!isSync(b + p))
                break;
        if (p == inputSamples)
            throw Exception("No data\n");
        // p points to the first non-sync sample
        for (; p < inputSamples; ++p)
            if (isSync(b + p))
                break;
        if (p == inputSamples)
            throw Exception("No sync\n");
        // p points to the first sync sample on the first line
        int firstHSync = p;

        int total = 0;
        int linesFound = 0;
        int lastHSync = p;
        do {
            if (lastHSync + nominalSamplesPerLine + 2 >= inputSamples)
                break;
            // Find the sync pulse on the next line
            int h = findHSync(b + lastHSync + nominalSamplesPerLine);
            if (!foundHSync(h)) {
                int i = 1;
                do {
                    if (linesFound == 0)
                        p = lastHSync + i*nominalSamplesPerLine;
                    else
                        p = lastHSync + (i*total)/linesFound;
                    if (p >= inputSamples)
                        break;
                    h = findHSync(b + p);
                    if (foundHSync(h)) {
                        lastHSync = p + h;
                        break;
                    }
                    ++i;
                } while (true);
                if (p >= inputSamples)
                    break;
            }
            else {
                ++linesFound;
                total += nominalSamplesPerLine + h;
                lastHSync += nominalSamplesPerLine + h;
            }
        } while (true);

        int i = 0;

        float inputSamplesPerLine = static_cast<float>(total)/linesFound;
        // int lines = (inputSamples - firstHSync)/inputSamplesPerLine;
        int outputSize = lines*outputSamplesPerLine;
        Array<Byte> outputBuffer(outputSize*3);
        float inputSize = lines*inputSamplesPerLine;
        float filterSize = lines*nominalSamplesPerLine/8;
        float inputSamplesPerOutputSample =
            inputSamplesPerLine/outputSamplesPerLine;
        float filterSamplesPerInputSample =
            nominalSamplesPerLine/(8*inputSamplesPerLine);
        float filterSamplesPerOutputSample =
            nominalSamplesPerLine/(8*outputSamplesPerLine);
        b += firstHSync;
        int z0 = 0;
        int kb = 256;  // inputSamplesPerLine/(nominalSamplesPerLine/8)
        float ct[8];
        float st[8];
        for (int i = 0; i < 8; ++i) {
            ct[i] = cos(i*M_PI*2/8);
            st[i] = sin(i*M_PI*2/8);
        }


        // Pass 2 - synchronize with the color burst to find the phase offset,
        // exact sample rate and wobble parameters.



        // Pass 3 - NTSC decode and rescale

        int o = 0;
        for (int line = 0; line < lines; ++line) {
            float i0 = 0;
            float q0 = 0;
            float dc = 0;
            for (int x = 140; x < 140 + 32; ++x) {
                int k = o*inputSize/outputSize + x;
                float z = (*(b + k))*contrast;
                double angle = (k - o*inputSize/outputSize + 2)/8;
                angle = (fmod(angle, 1.0) - 33.0/360)*M_PI*2;
                i0 += cos(angle)*z;
                q0 += sin(angle)*z;
                dc += z;
            }

            dc = (dc - 3989)/32;
            //float sat = 1.0/sqrt(i0*i0 + q0*q0);
            float sat = 0.00072;
            //printf("%f %f %f %f\n",i0,q0, atan2(q0,i0),sqrt(q0*q0+i0*i0));
            double contrast2 = (contrast/sqrt(i0*i0 + q0*q0))/.00072;

            for (int x = 0; x < outputSamplesPerLine; ++x) {
                float y = 0;
                float i = 0;
                float q = 0;
                float t = 0;
                int k = o*inputSize/outputSize;
                for (int j = -kb; j < kb; ++j) {
                    float s = lanczos(j*filterSamplesPerInputSample + z0);
                    float z = s*(*(b + j + k))*contrast;
                    //int octant = (j + k)&7;
                    double angle = static_cast<double>(j + k)*nominalSamplesPerLine/(inputSamplesPerLine*8);
                    angle = fmod(angle, 1.0)*M_PI*2;

                    y += z;
                    i += cos(angle)*z;  //ct[octant]*z;
                    q += sin(angle)*z;  //st[octant]*z;
                    t += s;
                }
                y /= t;
                i /= t;
                q /= t;

                float ic = (i*i0 + q*q0)*sat;
                float qc = (q*i0 - i*q0)*sat;

                y += brightness - dc;
                Byte r = clamp(0, static_cast<int>(y + 0.9563*ic + 0.6210*qc), 255);
                Byte g = clamp(0, static_cast<int>(y - 0.2721*ic - 0.6474*qc), 255);
                Byte b = clamp(0, static_cast<int>(y - 1.1069*ic + 1.7046*qc), 255);

                outputBuffer[o*3] = r;
                outputBuffer[o*3 + 1] = g;
                outputBuffer[o*3 + 2] = b;

                int k1 = (o + 1)*inputSize/outputSize;
                z0 += (k1 - k)*filterSamplesPerInputSample -
                    filterSamplesPerOutputSample;
                ++o;
            }
        }


        // Pass 4 - save to disk

        AutoHandle out = File("output.raw").openWrite();
        out.write(&outputBuffer[0], outputSize*3);
    }
};
