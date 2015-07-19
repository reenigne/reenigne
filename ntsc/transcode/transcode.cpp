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
            console.write("Usage: transcode <input picture name>\n");
            return;
        }

        String fileName = _arguments[1];

        PNGFileFormat<SRGB> png;
        Bitmap<SRGB> srgbInput = png.load(File(fileName, true));
        if (srgbInput.size().y != 240) {
            console.write("Input picture must have 240 rows\n");
            return;
        }

        int inputSize = 760*2;
        int outputSize = 760*2;
        int inputLeft = 450;

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

        fftwf_plan yBackward = fftwf_plan_dft_1d(inputSize,
            reinterpret_cast<fftwf_complex*>(&yFrequency[0]),
            reinterpret_cast<fftwf_complex*>(&yTime[0]),
            FFTW_BACKWARD, FFTW_MEASURE);
        fftwf_plan cBackward = fftwf_plan_dft_1d(inputSize,
            reinterpret_cast<fftwf_complex*>(&cFrequency[0]),
            reinterpret_cast<fftwf_complex*>(&cTime[0]),
            FFTW_BACKWARD, FFTW_MEASURE);


        Bitmap<SRGB> srgbOutput(srgbInput.size()*Vector(2, 1));

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
                float r = srgb.x / 255.0f;
                float g = srgb.y / 255.0f;
                float b = srgb.z / 255.0f;
                float i = 0.595716f*r - 0.274453f*g - 0.321263f*b;
                float q = 0.211456f*r - 0.522591f*g + 0.311135f*b;
                yTime[x] = 0.299f*r + 0.587f*g + 0.114f*b;
                cTime[x] = Complex<float>(i, q) * unit(x*192.0f/srgbInput.size().x + ((yy & 1) != 0 ? 0.0f : 0.5f));
            }

            fftwf_execute(yForward);
            fftwf_execute(cForward);

            for (int x = 0; x < 288; ++x)
                cFrequency[x] = 0;
            for (int x = 288; x < 480; ++x)
                yFrequency[x] = 0;
            for (int x = 480; x < inputSize; ++x)
                cFrequency[x] = 0;

            fftwf_execute(yBackward);
            fftwf_execute(cBackward);

            for (int x = 0; x < inputSize; ++x)
                cTime[x] *= unit(-x*192.0f/srgbInput.size().x + ((yy & 1) != 0 ? 0.0f : 0.5f));

            //fftwf_execute(cForward);

            //for (int x = 96; x < inputSize - 96; ++x)
            //    cFrequency[x] = 0;

            //fftwf_execute(cBackward);

            for (int x = 0; x < outputSize; ++x) {
                float y = yTime[x].x*255.0f/inputSize;
                Complex<float> c = cTime[x]*255.0f/(inputSize);
                SRGB o(
                    byteClamp(y + 0.9563*c.x + 0.6210*c.y),
                    byteClamp(y - 0.2721*c.x - 0.6474*c.y),
                    byteClamp(y - 1.1069*c.x + 1.7046*c.y));
                srgbOutput[Vector(x, yy)] = o;
            }
        }

        srgbOutput.save(png, File(fileName + "_transcoded.png", true));
    }
};
