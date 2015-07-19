#include "alfe/main.h"
#include "alfe/file.h"
#include "alfe/colour_space.h"
#include "alfe/bitmap.h"
#include "alfe/bitmap_png.h"
#include "alfe/minimum_maximum.h"
#include "alfe/complex.h"
#include "fftw3.h"

static const int peakChroma   = 0x3cc;
static const int white        = 0x320;
static const int peakBurst    = 0x160;
static const int black        = 0x118;
static const int blank        = 0x0f0;
static const int troughBurst  = 0x080;
static const int troughChroma = 0x068;
static const int sync         = 0x010;

static const int syncBurst[] = {
    // Sync              
    0x0f0,                      // 768-782
    0x0e9, 0x0a4, 0x044, 0x011, // 783-786
    0x010,                      // 787-849
    0x017, 0x05c, 0x0bc, 0x0ef, // 850-853
    0x0f0, 0x0f0, 0x0f0, 0x0f0, // 854-857
    // Burst fields 1, 3
    0x0f4, 0x0dc, 0x0d6, 0x12c, 0x123, 0x096, // 858-863
    0x0b3, 0x14e, 0x12d, 0x092, // 864-867
    0x0b3, 0x14e, 0x12d, 0x092, // 868-871
    0x0b3, 0x14e, 0x12d, 0x092, // 872-875
    0x0b3, 0x14e, 0x12d, 0x092, // 876-879
    0x0b3, 0x14e, 0x12d, 0x092, // 880-883
    0x0b3, 0x14e, 0x12d, 0x092, // 884-887
    0x0b3, 0x14e, 0x12d, 0x092, // 888-891
    0x0b3, 0x14e, 0x129, 0x0a6, // 892-895
    0x0cd, 0x112, 0x0fa, 0x0ec, // 896-899
    0x0f0,                      // 900-909
    // Burst field 2, 4
    0x0ec, 0x104, 0x10a, 0x0b4, 0x0bd, 0x14a, // 868-863
    0x12d, 0x092, 0x0b3, 0x14e, // 864-867
    0x12d, 0x092, 0x0b3, 0x14e, // 868-871
    0x12d, 0x092, 0x0b3, 0x14e, // 872-875
    0x12d, 0x092, 0x0b3, 0x14e, // 876-879
    0x12d, 0x092, 0x0b3, 0x14e, // 880-883
    0x12d, 0x092, 0x0b3, 0x14e, // 884-887
    0x12d, 0x092, 0x0b3, 0x14e, // 888-891
    0x12d, 0x092, 0x0b7, 0x13a, // 892-895
    0x113, 0x0ce, 0x0e6, 0x0f4, // 896-899
    0x0f0,                      // 900-909
    //                             equalizing        serration
    // VBI fields                  1, 3     2, 4     1, 3     2, 4
    0x0f0,                      // 768-782  313-327  782      327
    0x0e9, 0x0a4, 0x044, 0x011, // 783-786  328-331  783-786  328-331
    0x010,                      // 787-815  332-360  787-260  332-715
    0x017, 0x05c, 0x0bc, 0x0ef, // 816-819  361-364  261-264  716-719
    0x0f0,                      // 820-327  365-782  265-327  720-782
    0x0e9, 0x0a4, 0x044, 0x011, // 328-331  783-786  328-331  783-786
    0x010,                      // 332-360  787-815  332-715  787-260
    0x017, 0x05c, 0x0bc, 0x0ef, // 361-364  816-819  716-719  261-264
    0x0f0                       // 365-782  820-327  720-782  265-327
};

void rise(Bitmap<int> b, int x, int y)
{
    for (int xx = 0; xx < 4; ++xx)
        b[Vector(x + xx, y)] = syncBurst[xx + 6];
}

void fall(Bitmap<int> b, int x, int y)
{
    for (int xx = 0; xx < 4; ++xx)
        b[Vector(x + xx, y)] = syncBurst[xx + 1];
}

Bitmap<int> createSyncBurstBitmap()
{
    Bitmap<int> b(Vector(910, 262));
    b.fill(blank);
    for (int y = 0; y < 262; ++y) {
        // Horizontal sync
        fall(b, 783, y);
        for (int x = 787; x < 850; ++x)
            b[Vector(x, y)] = sync;
        rise(b, 850, y);

        // Burst
        for (int x = 858; x < 900; ++x)
            if ((y & 1) == 0)
                b[Vector(x, y)] = syncBurst[14 + x-858];
            else
                b[Vector(x, y)] = syncBurst[57 + x-858];
    }
    // Vertical sync
    for (int y = 241; y < 244; ++y) {
        // Equalization
        rise(b, 816, y);
        for (int x = 820; x < 910; ++x)
            b[Vector(x, y)] = blank;
        fall(b, 328, y + 1);
        for (int x = 332; x < 361; ++x)
            b[Vector(x, y + 1)] = sync;
        rise(b, 361, y + 1);

        // Serration
        for (int x = 850; x < 910; ++x)
            b[Vector(x, y + 3)] = sync;
        for (int x = 0; x < 261; ++x)
            b[Vector(x, y + 4)] = sync;
        rise(b, 261, y + 4);
        fall(b, 328, y + 4);
        for (int x = 332; x < 716; ++x)
            b[Vector(x, y + 4)] = sync;
        rise(b, 716, y + 4);

        // Equalization
        rise(b, 816, y + 6);
        for (int x = 820; x < 910; ++x)
            b[Vector(x, y + 6)] = blank;
        fall(b, 328, y + 7);
        for (int x = 332; x < 361; ++x)
            b[Vector(x, y + 7)] = sync;
        rise(b, 361, y + 7);
    }
    return b;
}

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() == 1) {
            console.write("Usage: encode <input picture name>\n");
            return;
        }

        String fileName = _arguments[1];

        PNGFileFormat<SRGB> png;
        Bitmap<SRGB> srgbInput = png.load(File(fileName, true));
        if (srgbInput.size().y != 240) {
            console.write("Input picture must have 240 rows\n");
            return;
        }

        int outputSize = 900;  // If increased over 910, need to modify syncburst to upsample
        //int outputSize = 910;

        Bitmap<int> syncBurstOrig = createSyncBurstBitmap();
        Array<Complex<float>> syncBurst910(910*262);
        for (int y = 0; y < 262; ++y)
            for (int x = 0; x < 910; ++x) {
                int v = syncBurstOrig[Vector(x, y)];
                syncBurst910[((y + 14)*910 + x + 60)%(910*262)] = v;
            }
        Array<Complex<float>> syncBurstFFT(max(910, outputSize));
        fftwf_plan sbForward = fftwf_plan_dft_1d(910,
            reinterpret_cast<fftwf_complex*>(&syncBurstFFT[0]),
            reinterpret_cast<fftwf_complex*>(&syncBurstFFT[0]),
            FFTW_FORWARD, FFTW_MEASURE);
        fftwf_plan sbBackward = fftwf_plan_dft_1d(outputSize,
            reinterpret_cast<fftwf_complex*>(&syncBurstFFT[0]),
            reinterpret_cast<fftwf_complex*>(&syncBurstFFT[0]),
            FFTW_BACKWARD, FFTW_MEASURE);

        Array<Complex<float>> syncBurst(outputSize*262);
        for (int y = 0; y < 262; ++y) {
            for (int x = 0; x < 910; ++x)
                syncBurstFFT[x] = syncBurst910[y*910 + x];
            fftwf_execute(sbForward);
            for (int x = outputSize/2; x < outputSize; ++x)
                syncBurstFFT[x] = syncBurstFFT[x + 910 - outputSize];
            fftwf_execute(sbBackward);
            for (int x = 0; x < outputSize; ++x)
                syncBurst[y*outputSize + x] = syncBurstFFT[x]/910/4;
        }

        int inputSize = srgbInput.size().x*910/768;
        int inputLeft = static_cast<int>(0.5f +
            58*srgbInput.size().x*910.0f/(outputSize*768));

        Array<Complex<float>> yTime(inputSize);
        Array<Complex<float>> cTime(inputSize);
        Array<Complex<float>> yFrequency(inputSize);
        Array<Complex<float>> cFrequency(inputSize);
        Array<Complex<float>> oFrequency(outputSize);
        Array<Complex<float>> oTime(outputSize);

        fftwf_plan yForward = fftwf_plan_dft_1d(inputSize,
            reinterpret_cast<fftwf_complex*>(&yTime[0]),
            reinterpret_cast<fftwf_complex*>(&yFrequency[0]),
            FFTW_FORWARD, FFTW_MEASURE);
        fftwf_plan cForward = fftwf_plan_dft_1d(inputSize,
            reinterpret_cast<fftwf_complex*>(&cTime[0]),
            reinterpret_cast<fftwf_complex*>(&cFrequency[0]),
            FFTW_FORWARD, FFTW_MEASURE);
        fftwf_plan backward = fftwf_plan_dft_1d(outputSize,
            reinterpret_cast<fftwf_complex*>(&oFrequency[0]),
            reinterpret_cast<fftwf_complex*>(&oTime[0]),
            FFTW_BACKWARD, FFTW_MEASURE);

        Array<float> output(outputSize*262);

        for (int yy = 0; yy < 262; ++yy)
            for (int x = 0; x < outputSize; ++x)
                output[yy*outputSize + x] = syncBurst[yy*outputSize + x].x;

        for (int yy = 0; yy < 240; ++yy) {
            for (int x = 0; x < inputSize; ++x) {
                int xx;
                if (x < inputLeft)
                    xx = inputLeft - 1 - x;
                else
                    if (x < inputLeft + srgbInput.size().x)
                        xx = x - inputLeft;
                    else
                        xx = inputLeft + 2*srgbInput.size().x - 1 - x;
                SRGB srgb = srgbInput[Vector(xx, yy)];
                float r = srgb.x / 255.0;
                float g = srgb.y / 255.0;
                float b = srgb.z / 255.0;
                float i = 0.595716f*r - 0.274453f*g - 0.321263f*b;
                float q = 0.211456f*r - 0.522591f*g + 0.311135f*b;
                yTime[x] = 0.299f*r + 0.587f*g + 0.114f*b;
                cTime[x] = Complex<float>(i, q) *
                    unit(-x*227.5f/inputSize + ((yy & 1) != 0 ? 0.0f : 0.5f));
            }

            fftwf_execute(yForward);
            fftwf_execute(cForward);

            int xCount = min(outputSize, inputSize);
            int xMid = (xCount + 1)/2;
            for (int x = 0; x < xCount; ++x) {
                int ix = x;
                int ox = x;
                if (x > xMid) {
                    ix += inputSize - xCount;
                    ox += outputSize - xCount;
                }
                if ((ox >= outputSize - 284 && ox <= outputSize - 171) ||
                    (ox >= 171 && ox <= 284))
                    oFrequency[ox] = cFrequency[ix];
                else
                    oFrequency[ox] = yFrequency[ix];
            }

            fftwf_execute(backward);

            double gain = (200.0 - 71.0)/inputSize;
            double offset = 71.0;
            for (int x = 0; x < 768*outputSize/910; ++x)
                output[(yy + 14)*outputSize + x + 58] =
                    oTime[x + 58].x*gain + offset;
        }

        int paddedOutputSize = (outputSize + 7)/8*8;
        Array<Byte> byteOutput(paddedOutputSize*262);

        float low = 255;
        float high = 0;
        for (int y = 0; y < 262; ++y)
            for (int x = 0; x < outputSize; ++x) {
                float v = output[y*outputSize + x];
                low = min(low, v);
                high = max(high, v);
            }
        for (int y = 0; y < 262; ++y)
            for (int x = 0; x < outputSize; ++x) {
                float v = output[y*outputSize + x];
                byteOutput[y*paddedOutputSize + x] = clamp(0, static_cast<int>((v - low)*64/(high - low)), 63);
            }

        FileHandle h = File(fileName + ".raw", true).openWrite();
        h.write(byteOutput);
    }
};
