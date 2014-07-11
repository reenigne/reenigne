#include "alfe/main.h"                             
#include "alfe/bitmap_png.h"
#include "alfe/complex.h"

static const SRGB inputPalette[16] = {
    SRGB(0x00, 0x00, 0x00), SRGB(0x00, 0x00, 0xaa),
    SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
    SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
    SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
    SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
    SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
    SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
    SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff)};

//Low = 0.070565, High = 0.727546
static Byte chromaData[256] = {
     65, 11, 62,  6, 121, 87, 63,  6,  60,  9,120, 65,  61, 59,129,  5,
    121,  6, 58, 58, 134, 65, 62,  6,  57,  9,108, 72, 126, 72,125, 77,
     60, 98,160,  6, 113,195,194,  8,  53, 94,218, 64,  56,152,225,  5,
    118, 90,147, 56, 115,154,156,  0,  52, 92,197, 73, 107,156,213, 62,
    119, 10, 97,122, 178, 77, 60, 87, 119, 12,174,205, 119, 58,135, 88,
    185,  6, 54,158, 194, 67, 57, 87, 114, 10,101,168, 181, 67,114,160,
     64,  8,156,109, 121, 73,177,122,  58,  8,244,207,  65, 58,251,137,
    127,  5,141,156, 126, 58,144, 97,  57,  7,189,168, 106, 55,201,162,
    163,124, 62, 10, 185,159, 59,  8, 135,104,128, 80, 119,142,140,  5,
    241,141, 59, 57, 210,160, 61,  5, 137,108,103, 61, 177,140,110, 65,
     59,107,124,  4, 180,201,122,  6,  52,104,194, 77,  55,159,197,  3,
    130,128,121, 51, 174,197,123,  3,  52,100,162, 62, 101,156,171, 51,
    173, 11, 60,113, 199, 93, 58, 77, 167, 11,118,196, 132, 63,129, 74,
    256,  9, 54,195, 192, 55, 59, 74, 183, 14,103,199, 206, 74,118,154,
    153,108,156,105, 255,202,188,123, 143,107,246,203, 164,208,250,129,
    209,103,148,157, 253,195,171,120, 163,106,196,207, 245,202,249,208};

static double intensity[4] = {
    0, 0.047932237386703491, 0.15110087022185326, 0.18384206667542458};

class Program : public ProgramBase
{
public:
    void run()
    {
        for (int i = 0; i < 256; ++i)
            _chroma[i] = chromaData[i]*(0.727546-0.070565)/256.0+0.070565;

        String inputFileName;
        String outputFileName;
        bool gotInputFileName = false;
        bool gotOutputFileName = false;
        bool unknownArgument = false;
        _newCGA = false;
        _hue = 0;

        // capture_field settings:
        //_saturation = 0.87554481017589569;
        //_contrast = 1;
        //_brightness = 0;

        // Ideal settings
        _saturation = 0.819474;
        _contrast = 0.851458;
        _brightness = 0.074192;

        // DOSBox settings
        //_saturation = 0.439715;
        //_contrast = 1.489000;
        //_brightness = -0.244639;

        for (int i = 1; i < _arguments.count(); ++i) {
            String argument = _arguments[i];
            CharacterSource s(argument);
            if (s.get() == '-')
                if (s.get() == '-') {
                    // TODO
                    continue;                    
                }
            if (!gotInputFileName) {
                inputFileName = argument;
                gotInputFileName = true;
                continue;
            }
            if (!gotOutputFileName) {
                outputFileName = argument;
                gotOutputFileName = true;
                continue;
            }
            unknownArgument = true;
            break;
        }
        if (!gotInputFileName || !gotOutputFileName || unknownArgument) {
            console.write("Syntax: " + _arguments[0] +
                " <input file name.png> <output file name.png> [options]\n");
            console.write("Options are not implemented yet:\n");
            console.write("  --type=<type> - CGA card type (old or new)\n");
            console.write("  --hue=<value>\n");
            console.write("  --saturation=<value>\n");
            console.write("  --contrast=<value>\n");
            console.write("  --brightness=<value>\n");
        }
        BitmapFileFormat<SRGB> png = PNGFileFormat();
        Bitmap<SRGB> input = png.load(File(_arguments[1], true));
        Bitmap<SRGB> input2 = input;
        Vector size = input.size();
        if (size.x <= 456) {
            // Image is most likely 2bpp or LRES text mode with 1 pixel per
            // ldot. Rescale it to 1 pixel per hdot.
            input2 = Bitmap<SRGB>(size * 2);
            const Byte* inputRow = input.data();
            Byte* outputRow = input2.data();
            for (int y = 0; y < size.y; ++y) {
                SRGB* outputPixel = reinterpret_cast<SRGB*>(outputRow);
                const SRGB* inputPixel =
                    reinterpret_cast<const SRGB*>(inputRow);
                for (int x = 0; x < size.x; ++x) {
                    SRGB s = *inputPixel;
                    ++inputPixel;
                    *outputPixel = s;
                    ++outputPixel;
                    *outputPixel = s;
                    ++outputPixel;
                }
                outputRow += input2.stride();
                outputPixel = reinterpret_cast<SRGB*>(outputRow);
                inputPixel = reinterpret_cast<const SRGB*>(inputRow);
                for (int x = 0; x < size.x; ++x) {
                    SRGB s = *inputPixel;
                    ++inputPixel;
                    *outputPixel = s;
                    ++outputPixel;
                    *outputPixel = s;
                    ++outputPixel;
                }
                inputRow += input.stride();
                outputRow += input2.stride();
            }
            input = input2;
            size = input.size();
        }

        // Convert to RGBI indexes and add left and right borders.
        Bitmap<Byte> rgbi(size + Vector(14, 0));
        {
            rgbi.fill(0);
            const Byte* inputRow = input.data();
            Byte* rgbiRow = rgbi.data();
            for (int y = 0; y < size.y; ++y) {
                const SRGB* inputPixel =
                    reinterpret_cast<const SRGB*>(inputRow);
                Byte* rgbiPixel = rgbiRow + 7;
                for (int x = 0; x < size.x; ++x) {
                    SRGB s = *inputPixel;
                    ++inputPixel;
                    int bestDistance = 0x7fffffff;
                    Byte bestRGBI = 0;
                    for (int i = 0; i < 16; ++i) {
                        int distance =
                            (Vector3Cast<int>(inputPalette[i]) - 
                            Vector3Cast<int>(s)).modulus2();
                        if (distance < bestDistance) {
                            bestDistance = distance;
                            bestRGBI = i;
                            if (distance < 42*42)
                                break;
                        }
                    }
                    *rgbiPixel = bestRGBI;
                    ++rgbiPixel;
                }
                inputRow += input.stride();
                rgbiRow += rgbi.stride();
            }
        }

        // Convert to raw NTSC
        Bitmap<double> ntsc(size + Vector(13, 0));
        {
            const Byte* rgbiRow = rgbi.data();
            Byte* ntscRow = ntsc.data();
            for (int y = 0; y < size.y; ++y) {
                const Byte* rgbiPixel = rgbiRow;
                double* ntscPixel = reinterpret_cast<double*>(ntscRow);
                for (int x = 0; x < size.x + 13; ++x) {
                    int left = *rgbiPixel;
                    ++rgbiPixel;
                    int right = *rgbiPixel;
                    *ntscPixel = simulateCGA(left, right, (x+1)&3);
                    ++ntscPixel;
                }
                rgbiRow += rgbi.stride();
                ntscRow += ntsc.stride();
            }
        }

        // Find color burst
        double burst[4];
        for (int i = 0; i < 4; ++i)
            burst[i] = simulateCGA(6, 6, i);
        Complex<double> iq;
        iq.x = burst[0] - burst[2];
        iq.y = burst[1] - burst[3];
        _contrast /= 32.0;
        _saturation *= 2.0;
        Complex<double> iqAdjust =
            -iq.conjugate()*unit((33 + 90 + _hue)/360.0)*_saturation*_contrast/
            iq.modulus();

        // Decode NTSC
        Bitmap<SRGB> output(size + Vector(7, 0));
        {
            const Byte* ntscRow = ntsc.data();
            Byte* outputRow = output.data();
            for (int yy = 0; yy < size.y; ++yy) {
                const double* n = reinterpret_cast<const double*>(ntscRow);
                SRGB* outputPixel = reinterpret_cast<SRGB*>(outputRow);
                for (int x = 0; x < size.x + 7; ++x) {
                    if (x == 24 && yy == 24)
                        x = 24;
                    // Filter kernel must be a polynomial of (1,1,1,1) so that
                    // all phases contribute equally.
                    double y =
                        n[0] +n[1]*4 +n[2]*7 +n[3]*8 +n[4]*7 +n[5]*4 +n[6];
                    Complex<double> iq;
                    switch ((x + 1) & 3) {
                        case 0: 
                            iq.x =  n[0]   -n[2]*7 +n[4]*7 -n[6];
                            iq.y =  n[1]*4 -n[3]*8 +n[5]*4; 
                            break;
                        case 1:
                            iq.x = -n[1]*4 +n[3]*8 -n[5]*4;
                            iq.y =  n[0]   -n[2]*7 +n[4]*7 -n[6];
                            break;
                        case 2:
                            iq.x = -n[0]   +n[2]*7 -n[4]*7 +n[6];
                            iq.y = -n[1]*4 +n[3]*8 -n[5]*4; 
                            break;
                        case 3:
                            iq.x = +n[1]*4 -n[3]*8 +n[5]*4;
                            iq.y = -n[0]   +n[2]*7 -n[4]*7 +n[6];
                            break;
                    }
                    ++n;

                    y = y*_contrast + _brightness;
                    iq *= iqAdjust;

                    Colour nn(  // RGB NTSC
                        y + 0.9563*iq.x + 0.6210*iq.y,
                        y - 0.2721*iq.x - 0.6474*iq.y,
                        y - 1.1069*iq.x + 1.7046*iq.y);
                    Colour s(   // sRGB
                         1.5073*nn.x - 0.3725*nn.y - 0.0832*nn.z,
                        -0.0275*nn.x + 0.9350*nn.y + 0.0670*nn.z,
                        -0.0272*nn.x - 0.0401*nn.y + 1.1677*nn.z);
                    //s = s*256;
                    s = nn*255;

                    *outputPixel =
                        SRGB(byteClamp(s.x), byteClamp(s.y), byteClamp(s.z));
                    ++outputPixel;
                }
                ntscRow += ntsc.stride();
                outputRow += output.stride();
            }
        }
        output.save(png, File(outputFileName, true));
    }
private:
    double simulateCGA(int left, int right, int phase)
    {
        double c = _chroma[((left & 7) << 5) | ((right & 7) << 2) | phase];
        double i = intensity[(left >> 3) | ((right >> 2) & 2)];
        if (!_newCGA)
            return c+i;
        double r = intensity[((left >> 2) & 1) | ((right >> 1) & 2)];
        double g = intensity[((left >> 1) & 1) | (right & 2)];
        double b = intensity[(left & 1) | ((right << 1) & 1)];
        return (c/0.72)*0.29 + (i/0.28)*0.32 + (r/0.28)*0.1 + (g/0.28)*0.22 +
            (b/0.28)*0.07;
    }

    bool _newCGA;
    double _hue;
    double _saturation;
    double _contrast;
    double _brightness;
    double _chroma[256];
};
