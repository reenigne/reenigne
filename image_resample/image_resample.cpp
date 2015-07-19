#include "alfe/main.h"
#include "alfe/file.h"
#include "alfe/colour_space.h"
#include <stdio.h>
#include "alfe/bitmap.h"
#include "alfe/bitmap_png.h"
#include "alfe/config_file.h"
#include "alfe/minimum_maximum.h"
#include "alfe/complex.h"
#include "fftw3.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() == 1) {
            printf("Usage: image_resample <config file path>\n");
            return;
        }

        ConfigFile config;
        auto inputPicture = config.addOption<String>("inputPicture");
        auto outputSizeOption = config.addOption<Vector>("outputSize");
        auto subpixels = config.addDefaultOption<bool>("subpixels", false);
        auto outputPicture = config.addOption<String>("outputPicture");
        auto gammaOption = config.addDefaultOption<double>("gamma", -1);
        config.load(_arguments[1]);

        PNGFileFormat<SRGB> png;
        Bitmap<SRGB> srgbInput = png.load(File(inputPicture.get()));
        Vector inputSize = srgbInput.size()*2;
        Bitmap<Vector3<Complex<float>>> input(inputSize);
        Vector outputSize = outputSizeOption.get()*2;

        Bitmap<Vector3<Complex<float>>> inputFFT(inputSize);
        int n[2] = {inputSize.y, inputSize.x};
        int inembed[2] = {inputSize.y, input.stride() / sizeof(Vector3<Complex<float>>)};
        int onembed[2] = {inputSize.y, inputFFT.stride() / sizeof(Vector3<Complex<float>>)};
        fftwf_plan forward = fftwf_plan_many_dft(2, n, 3,
            reinterpret_cast<fftwf_complex*>(input.data()), inembed, 3, 1,
            reinterpret_cast<fftwf_complex*>(inputFFT.data()), onembed, 3, 1,
            FFTW_FORWARD, FFTW_MEASURE);

        Bitmap<Vector3<Complex<float>>> outputFFT(outputSize);
        Bitmap<Vector3<Complex<float>>> output(outputSize);
        n[0] = outputSize.y;
        n[1] = outputSize.x;
        inembed[0] = outputSize.y;
        inembed[1] = outputFFT.stride() / sizeof(Vector3<Complex<float>>);
        onembed[0] = outputSize.y;
        onembed[1] = output.stride() / sizeof(Vector3<Complex<float>>);
        fftwf_plan backward = fftwf_plan_many_dft(2, n, 3,
            reinterpret_cast<fftwf_complex*>(outputFFT.data()), inembed, 3, 1,
            reinterpret_cast<fftwf_complex*>(output.data()), onembed, 3, 1,
            FFTW_BACKWARD, FFTW_MEASURE);

        Vector tl = inputSize/4;
        Vector s = inputSize/2;
        double gamma = gammaOption.get();
        if (gamma < 0)
            srgbInput.convert(input.subBitmap(tl, s), ConvertSRGBToLinear());
        else
            srgbInput.convert(input.subBitmap(tl, s),
                ConvertSRGBToLinearWithGamma(gamma));

        int r = tl.x + s.x;
        for (int y = 0; y < tl.y; ++y) {
            int yy = tl.y*2 - 1 - y;
            for (int x = 0; x < tl.x; ++x)
                input[Vector(x, y)] = input[Vector(tl.x*2 - 1 - x, yy)];
            for (int x = tl.x; x < r; ++x)
                input[Vector(x, y)] = input[Vector(x, yy)];
            for (int x = r; x < s.x*2; ++x)
                input[Vector(x, y)] = input[Vector(r*2 - 1 - x, yy)];
        }
        for (int y = tl.y; y < tl.y + s.y; ++y) {
            for (int x = 0; x < tl.x; ++x)
                input[Vector(x, y)] = input[Vector(tl.x*2 - 1 - x, y)];
            for (int x = r; x < s.x*2; ++x)
                input[Vector(x, y)] = input[Vector(r*2 - 1 - x, y)];
        }
        for (int y = tl.y + s.y; y < s.y*2; ++y) {
            int yy = (tl.y + s.y)*2 - 1 - y;
            for (int x = 0; x < tl.x; ++x)
                input[Vector(x, y)] = input[Vector(tl.x*2 - 1 - x, yy)];
            for (int x = tl.x; x < r; ++x)
                input[Vector(x, y)] = input[Vector(x, yy)];
            for (int x = r; x < s.x*2; ++x)
                input[Vector(x, y)] = input[Vector(r*2 - 1 - x, yy)];
        }

        fftwf_execute(forward);
        outputFFT.fill(Vector3<Complex<float>>(0, 0, 0));

        bool sub = subpixels.get();
        int xCount = min(outputSize.x, inputSize.x);
        int yCount = min(outputSize.y, inputSize.y);
        int xMid = (xCount + 1)/2;
        int yMid = (yCount + 1)/2;
        for (int y = 0; y < yCount; ++y) {
            int iy = y;
            int oy = y;
            if (y > yMid) {
                iy += inputSize.y - yCount;
                oy += outputSize.y - yCount;
            }
            for (int x = 0; x < xCount; ++x) {
                int ix = x;
                int ox = x;
                if (x > xMid) {
                    ix += inputSize.x - xCount;
                    ox += outputSize.x - xCount;
                }
                outputFFT[Vector(ox, oy)] = inputFFT[Vector(ix, iy)];
                if (sub) {
                    float phase = x/(3.0f*outputSize.x);
                    if (x > xMid)
                        phase = (x - outputSize.x)/(3.0f*outputSize.x);
                    outputFFT[Vector(ox, oy)] *= Vector3<Complex<float>>(unit(-phase), 1, unit(phase));
                }
            }
        }

        fftwf_execute(backward);

        // float normalization = 1.0f/(outputSize.x*outputSize.y);
        float normalization = 1.0f/(inputSize.x*inputSize.y);
        for (int y = 0; y < outputSize.y; ++y)
            for (int x = 0; x < outputSize.x; ++x) {
                Vector v(x, y);
                Vector3<Complex<float>> p = output[v] * normalization;
                p.x.x = clamp(0.0f, p.x.x, 1.0f);
                p.y.x = clamp(0.0f, p.y.x, 1.0f);
                p.z.x = clamp(0.0f, p.z.x, 1.0f);
                output[v] = p;
            }

        Bitmap<SRGB> srgbOutput(outputSize/2);
        if (gamma < 0)
            output.subBitmap(outputSize/4, outputSize/2).
                convert(srgbOutput, ConvertLinearToSRGB());
        else
            output.subBitmap(outputSize/4, outputSize/2).
                convert(srgbOutput, ConvertLinearToSRGBWithGamma(gamma));
        //Bitmap<SRGB> srgbOutput(outputSize);
        //if (gamma < 0)
        //    output.convert(srgbOutput, ConvertLinearToSRGB());
        //else
        //    output.convert(srgbOutput, ConvertLinearToSRGBWithGamma(gamma));
        srgbOutput.save(png, File(outputPicture.get()));
    }
private:
    class ConvertSRGBToLinear
    {
    public:
        ConvertSRGBToLinear() : _c(ColourSpace::rgb()) { }
        Vector3<Complex<float>> convert(SRGB c)
        {
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
        }
    private:
        ColourSpace _c;
    };
    class ConvertSRGBToLinearWithGamma
    {
    public:
        ConvertSRGBToLinearWithGamma(double gamma) : _gamma(1/gamma) { }
        Vector3<Complex<float>> convert(SRGB c)
        {
            return Vector3<Complex<float>>(
                pow(c.x/255.0, _gamma), pow(c.y/255.0, _gamma),
                pow(c.z/255.0, _gamma));
        }
    private:
        double _gamma;
    };
    class ConvertLinearToSRGBWithGamma
    {
    public:
        ConvertLinearToSRGBWithGamma(double gamma) : _gamma(gamma) { }
        SRGB convert(Vector3<Complex<float>> c)
        {
            return SRGB(helper(c.x), helper(c.y), helper(c.z));
        }
    private:
        Byte helper(Complex<float> c)
        {
            return byteClamp(pow(static_cast<double>(c.x), _gamma)*255.0);
        }
        double _gamma;
    };
};
