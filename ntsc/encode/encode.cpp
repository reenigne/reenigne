#include "alfe/main.h"
#include "alfe/file.h"
#include "alfe/colour_space.h"
#include "alfe/bitmap.h"
#include "alfe/bitmap_png.h"
#include "alfe/minimum_maximum.h"
#include "alfe/complex.h"
#include "fftw3.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() == 1) {
            console.write("Usage: encode <input picture name>\n");
            return;
        }

        PNGFileFormat<SRGB> png;
        Bitmap<SRGB> srgbInput = png.load(File(_arguments[1], true));
        if (srgbInput.size().y != 240) {
            console.write("Input picture must have 240 rows\n");
            return;
        }

        int outputSize = 900;
        int inputSize = srgbInput.size().x*910/768;
        int inputLeft = (inputSize - srgbInput.size().x)/2;

        Array<Complex<float>> yTime(inputSize);
        Array<Complex<float>> cTime(inputSize);
        Array<Complex<float>> yFrequency(inputSize);
        Array<Complex<float>> cFrequency(inputSize);
        Array<Complex<float>> oFrequency(outputSize);
        Array<Complex<float>> oTime(outputSize);

        fftwf_plan yForward = fftwf_plan_dft_1d(inputSize,
            reinterpret_cast<fftwf_complex*>(&yTime),
            reinterpret_cast<fftwf_complex*>(&yFrequency),
            FFTW_FORWARD, FFTW_MEASURE);
        fftwf_plan cForward = fftwf_plan_dft_1d(inputSize,
            reinterpret_cast<fftwf_complex*>(&cTime),
            reinterpret_cast<fftwf_complex*>(&cFrequency),
            FFTW_FORWARD, FFTW_MEASURE);
        fftwf_plan backward = fftwf_plan_dft_1d(outputSize,
            reinterpret_cast<fftwf_complex*>(&oFrequency),
            reinterpret_cast<fftwf_complex*>(&oTime),
            FFTW_BACKWARD, FFTW_MEASURE);

        Bitmap<SRGB> srgbSyncBurst = png.load(File("sync_burst_900_262.png"));

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
                cTime[x] = Complex<float>(i, q) * unit(x*192.0f/inputSize);
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
                if ((ox >= 3*outputSize/16 && ox < 5*outputSize/16) ||
                    (ox >= 11*outputSize/16 && ox < 13*outputSize/16))
                    oFrequency[ox] = cFrequency[ix];
                else
                    oFrequency[ox] = yFrequency[ix];
            }

            fftwf_execute(backward);
        }

    }
private:
    class ConvertSRGBToLinear
    {
    public:
        ConvertSRGBToLinear() : _c(ColourSpace::rgb()) { }
        Vector3<Complex<float>> convert(SRGB c)
        {
            //return Vector3<float>(pow(c.x/255.0, 1/2.2), pow(c.y/255.0, 1/2.2), pow(c.z/255.0, 1/2.2));
            return Vector3Cast<Complex<float>>(_c.fromSrgb(c));
        }
    private:
        ColourSpace _c;
    };
    class ConvertLinearToSRGB
    {
    public:
        ConvertLinearToSRGB() : _c(ColourSpace::rgb()) { }
        SRGB convert(Vector3<Complex<float>> c)
        {
            return _c.toSrgb24(Vector3<float>(c.x.x, c.y.x, c.z.x));
            //return SRGB(
            //    static_cast<Byte>(clamp(0, static_cast<int>(pow(static_cast<double>(c.x), 2.2)*255.0), 255)),
            //    static_cast<Byte>(clamp(0, static_cast<int>(pow(static_cast<double>(c.y), 2.2)*255.0), 255)),
            //    static_cast<Byte>(clamp(0, static_cast<int>(pow(static_cast<double>(c.z), 2.2)*255.0), 255)));
        }
    private:
        ColourSpace _c;
    };
};
