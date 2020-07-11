#include "alfe/main.h"
#include "alfe/bitmap.h"
#include "alfe/avi.h"

int sampleFromIRE(float IRE) { return static_cast<int>(IRE * 1.4 + 60.49999); }

float sinewave(int sample, float yIRE, float cIRE, float phase_degrees)
{
    return yIRE + cIRE * static_cast<float>(
        sin(((sample & 3) + (phase_degrees + 147) / 90) * M_PI / 2)) / 2;
}

float lirp(float ire1, float ire2, float f)
{
    return ire1 + (ire2 - ire1) * f;
}

float black() { return 7.5; }
float blank() { return 0; }
float sync() { return -40; }

float colorCarrier(int sample) { return sinewave(sample, 0, 40, 180); }

float horizontalBlanking(int sample)
{
    int x = sample % 910;
    if (x <= 782 || x >= 900 || (x >= 853 && x <= 857))
        return blank();
    if (x >= 786 && x <= 849)
        return sync();
    if (x >= 863 && x <= 893)
        return colorCarrier(sample);
    switch (x) {
        case 783: return lirp(blank(), sync(), 2.0f / 56);
        case 784: return lirp(blank(), sync(), 19.0f / 56);
        case 785: return lirp(blank(), sync(), 43.0f / 56);

        case 850: return lirp(sync(), blank(), 2.0f / 56);
        case 851: return lirp(sync(), blank(), 19.0f / 56);
        case 852: return lirp(sync(), blank(), 43.0f / 56);

        case 858: return lirp(blank(), colorCarrier(sample), 23.0f / 345);
        case 859: return lirp(blank(), colorCarrier(sample), 75.0f / 345);
        case 860: return lirp(blank(), colorCarrier(sample), 138.0f / 345);
        case 861: return lirp(blank(), colorCarrier(sample), 225.0f / 345);
        case 862: return lirp(blank(), colorCarrier(sample), 299.0f / 345);

        case 894: return lirp(colorCarrier(sample), blank(), 23.0f / 345);
        case 895: return lirp(colorCarrier(sample), blank(), 75.0f / 345);
        case 896: return lirp(colorCarrier(sample), blank(), 138.0f / 345);
        case 897: return lirp(colorCarrier(sample), blank(), 225.0f / 345);
        case 898: return lirp(colorCarrier(sample), blank(), 276.0f / 345);
        case 899: return lirp(colorCarrier(sample), blank(), 330.0f / 345);
    }
    return 0;
}

Vector3<float> yiqFromRGB(Vector3<float> rgb)
{
    return Vector3<float>(0.299f * rgb.x + 0.587f * rgb.y + 0.114f * rgb.z,
        0.595716f * rgb.x - 0.274453f * rgb.y - 0.321263f * rgb.z,
        0.211456f * rgb.x - 0.522591f * rgb.y + 0.311135f * rgb.z);
}

float ireFromC(float c)
{
    return c * 95.5f + 7.5f;
}

class NTSCEncoder : Uncopyable
{
public:
    NTSCEncoder() : _frame(0) { }
    void encode(Bitmap<SRGB> bitmap, Array<Byte>* ntsc)
    {
        int n = 910 * 525 / 2;
        ntsc->ensure(n);

        int sample = n * _frame;

        Byte* p = &(*ntsc)[0];
        for (int i = 0; i < n; ++i) {
            *p = sampleFromIRE(image(&bitmap, sample));
            ++p;
            ++sample;
        }

        _frame = (_frame + 1) & 3;
    }
private:
    float image(Bitmap<SRGB>* bitmap, int sample)
    {
        int sampleInField = (sample + 20 * 910 + 142) % (455 * 525);
        if (sampleInField >= 20 * 910) {
            int xx = sample % 910;
            if (xx < 768) {
                int yInFrame = (sample % (525 * 910)) / 910;
                int yy;
                if (yInFrame < 245)
                    yy = yInFrame * 2 + 1;
                else
                    yy = (yInFrame - 262) * 2;
                SRGB rgb = (*bitmap)[Vector(xx, yy)];
                Vector3<float> yiq = yiqFromRGB(Vector3Cast<float>(rgb)/255);
                float c = 0;
                switch (sample & 3) {
                    case 0: c = yiq.x - yiq.y; break;
                    case 1: c = yiq.x - yiq.z; break;
                    case 2: c = yiq.x + yiq.y; break;
                    case 3: c = yiq.x + yiq.z; break;
                }
                return ireFromC(c);
            }
            return horizontalBlanking(sample);
        }
    
        // Vertical blanking
        if (sampleInField < 9 * 910) {
            // Vertical sync
            int sampleInHalfLine = (sampleInField + 455 - 142) % 455;
            if (sampleInField >= 3 * 910 && sampleInField < 6 * 910) {
                // Serration
                if (sampleInHalfLine <= 260 || sampleInHalfLine >= 331)
                    return sync();
                if (sampleInHalfLine >= 264 && sampleInHalfLine <= 327)
                    return blank();
                switch (sampleInHalfLine) {
                    case 261:
                        return lirp(sync(), blank(), 2.0f / 56); // 0x06;
                    case 262:
                        return lirp(sync(), blank(), 19.0f / 56); // 0x17;
                    case 263:
                        return lirp(sync(), blank(), 43.0f / 56); // 0x2f;
                }
                // Samples 328-330 are the same for equalization and serration
            }
            // Equalization
            if (sampleInHalfLine <= 327 || sampleInHalfLine >= 364)
                return blank();
            if (sampleInHalfLine >= 331 && sampleInHalfLine <= 360)
                return sync();
            switch (sampleInHalfLine) {
                case 361: return lirp(sync(), blank(), 2.0f / 56); // 0x06;
                case 362: return lirp(sync(), blank(), 19.0f / 56); // 0x17;
                case 363: return lirp(sync(), blank(), 43.0f / 56); // 0x2f;
                case 328: return lirp(blank(), sync(), 2.0f / 56); // 0x3a;
                case 329: return lirp(blank(), sync(), 19.0f / 56); // 0x29;
                case 330: return lirp(blank(), sync(), 43.0f / 56); // 0x11;
            }
            return 0;
        }
        return horizontalBlanking(sample);
    }
    
    int _frame;
};

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] +
                " <input file name>\n");
            return;
        }
        COMInitializer comInitializer;
        AVIFileInitializer aviFileInitializer;
        AVIFile aviFile(_arguments[1]);
        AVIStream aviStream(&aviFile);
        GetFrame getFrame(&aviStream);
        
        File outputFile = _arguments[1] + ".ntsc";
        FileStream stream = outputFile.openWrite();
        Bitmap<SRGB> bitmap(Vector(768, 525));
        Array<Byte> ntsc;
        NTSCEncoder encoder;
        while (!getFrame.atEnd()) {
            bitmap = getFrame.getFrame(bitmap);
            encoder.encode(bitmap, &ntsc);
            stream.write(ntsc);
        }
    }
};