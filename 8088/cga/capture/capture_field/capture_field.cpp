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

class Program : public ProgramBase
{
public:
    void run()
    {
        AutoHandle h = File("\\\\.\\pipe\\vbicap", true).openPipe();
        h.write<int>(1);

        static const int totalSamples = 1024*450;
        static const int nominalSamplesPerLine = 1824;

        Array<Byte> buffer(totalSamples + 512);
        Byte* b = &buffer[0] + 256;
        for (int i = 0; i < 256; ++i) {
            *(b - 256 + i) = 0;
            *(b + totalSamples + i) = 0;
        }
        for (int i = 0; i < 450; ++i)
            h.read(&buffer[i*1024], 1024);

        int p;
        for (p = 0; p < totalSamples; ++p)
            if (!isSync(b + p))
                break;
        if (p == totalSamples)
            throw Exception("No data\n");
        // p points to the first non-sync sample
        for (; p < totalSamples; ++p)
            if (isSync(b + p))
                break;
        if (p == totalSamples)
            throw Exception("No sync\n");
        // p points to the first sync sample on the first line
        int firstHSync = p;

        int total = 0;
        int linesFound = 0;
        int lastHSync = p;
        do {
            if (lastHSync + nominalSamplesPerLine + 2 >= totalSamples)
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
                    if (p >= totalSamples)
                        break;
                    h = findHSync(b + p);
                    if (foundHSync(h)) {
                        lastHSync = p + h;
                        break;
                    }
                    ++i;
                } while (true);
                if (p >= totalSamples)
                    break;
            }
            else {
                ++linesFound;
                total += nominalSamplesPerLine + h;
                lastHSync += nominalSamplesPerLine + h;
            }
        } while (true);
        
        AutoHandle out = File("output.raw").openWrite();
        int i = 0;

        float scaleTarget;
        float scale;
        int outputSamplesPerLine = 1824*5/12;
        int targetTotal = linesFound*outputSamplesPerLine;
        float scaleFactor = 
            static_cast<float>(total)/static_cast<float>(targetTotal);
        if (targetTotal > total) {
            // Upsampling
            scaleTarget = scaleFactor;
            scale = 1.0f;
        }
        else {
            // Downsampling
            scaleTarget = 1.0f;
            scale = 1.0f/scaleFactor;
        }
        b += firstHSync;
        int outputBytes = static_cast<int>(scale*totalSamples);
        int outputLines = outputBytes/nominalSamplesPerLine;
        outputBytes = outputLines*nominalSamplesPerLine;
        Array<Byte> outputBuffer(outputBytes);
        float z0 = 0;
        for (int i = 0; i < outputBytes; ++i) {
            float t = 0;
            float a = 0;
            int k = static_cast<int>(i * scale); 
            for (int j = -256; j < 256; ++j) {
                float s = sinc((j*scale - z0)/(8*scaleFactor));
                t += s;
                a += s*(*(b + j + k));
            }
            outputBuffer[i] = clamp(0, static_cast<int>(a/t), 255);
            int k1 = static_cast<int>((i + 1) * scale);
            z0 += scaleTarget - (k1 - k)*scale;  // TODO: do this subtraction in integer coordinates
        }
        //do {
        //    int first = (i*total)/linesFound + firstHSync;
        //    int last = first + nominalSamplesPerLine;
        //    if (last > totalSamples)
        //        break;
        //    out.write(b + first, 1824);
        //    ++i;
        //} while (true);
        out.write(&outputBuffer[0], outputBytes);
    }
};