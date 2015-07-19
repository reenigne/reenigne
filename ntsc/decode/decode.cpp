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
            console.write("Usage: encode <input ntsc file name>\n");
            return;
        }

        String fileName = _arguments[1];

        Array<Byte> input;
        FileHandle inputHandle = File(fileName, true).openRead();
        int nn = inputHandle.size();
        input.allocate(nn);
        inputHandle.read(&input[0], nn);

        Array<Complex<float>> yTime(900);
        Array<Complex<float>> cTime(900);
        Array<Complex<float>> yFrequency(900);
        Array<Complex<float>> cFrequency(900);

        fftwf_plan yForward = fftwf_plan_dft_1d(900,
            reinterpret_cast<fftwf_complex*>(&yTime[0]),
            reinterpret_cast<fftwf_complex*>(&yFrequency[0]),
            FFTW_FORWARD, FFTW_MEASURE);
        fftwf_plan cForward = fftwf_plan_dft_1d(900,
            reinterpret_cast<fftwf_complex*>(&cTime[0]),
            reinterpret_cast<fftwf_complex*>(&cFrequency[0]),
            FFTW_FORWARD, FFTW_MEASURE);
        fftwf_plan yBackward = fftwf_plan_dft_1d(900,
            reinterpret_cast<fftwf_complex*>(&yFrequency[0]),
            reinterpret_cast<fftwf_complex*>(&yTime[0]),
            FFTW_BACKWARD, FFTW_MEASURE);
        fftwf_plan cBackward = fftwf_plan_dft_1d(900,
            reinterpret_cast<fftwf_complex*>(&cFrequency[0]),
            reinterpret_cast<fftwf_complex*>(&cTime[0]),
            FFTW_BACKWARD, FFTW_MEASURE);

        Bitmap<SRGB> output(Vector(900, 262));

        for (int y = 0; y < 262; ++y) {
            for (int x = 0; x < 900; ++x) {
                yTime[x] = input[y*904 + x];
                cTime[x] = static_cast<float>(input[y*904 + x])
                    *unit(x*227.5f/900.0f + ((y & 1) != 0 ? 0.5f : 0.0f) + 0.5f);
            }
            fftwf_execute(yForward);
            fftwf_execute(cForward);
            for (int x = 900 - 284; x <= 900 - 171; ++x)
                yFrequency[x] = 0;
            for (int x = 171; x <= 284; ++x)
                yFrequency[x] = 0;
            for (int x = 57; x <= 900 - 57; ++x)
                cFrequency[x] = 0;
            fftwf_execute(yBackward);
            fftwf_execute(cBackward);
            for (int x = 0; x < 900; ++x) {
                float yy = (yTime[x].x/900 - 17)*255/(50-17) + 128;
                Complex<float> c = cTime[x]*2*255/(900*(50-17));
                SRGB o(
                    byteClamp(yy + 0.9563*c.x + 0.6210*c.y),       // 255     255 = (0.9563+1.1069)*I  I = 123.59  
                    byteClamp(yy - 0.2721*c.x - 0.6474*c.y),       // 103
                    byteClamp(yy - 1.1069*c.x + 1.7046*c.y));      // 0       Y = 136.81
                output[Vector(x, y)] = o;
            }
        }

        PNGFileFormat<SRGB> png;
        output.save(png, File(fileName + ".png", true));
    }
};
