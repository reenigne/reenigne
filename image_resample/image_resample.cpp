#include "alfe/main.h"
#include "alfe/file.h"
#include "alfe/colour_space.h"
#include <stdio.h>
#include "alfe/bitmap.h"
#include "alfe/bitmap_png.h"
#include "alfe/config_file.h"
#include "alfe/minimum_maximum.h"

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

        List<StructuredType::Member> vectorMembers;
        vectorMembers.add(StructuredType::Member(String("x"), Type::integer));
        vectorMembers.add(StructuredType::Member(String("y"), Type::integer));
        StructuredType vectorType(String("Vector"), vectorMembers);
        config.addType(vectorType);

        config.addOption("inputPicture", Type::string);
        config.addOption("outputSize", vectorType);
        config.addOption("subpixels", Type::boolean);
        config.addOption("tripleResolution", Type::boolean);
        config.addOption("outputPicture", Type::string);
        config.load(_arguments[1]);

        PNGFileFormat png;
        Bitmap<SRGB> input =
            png.load(File(config.get<String>("inputPicture")));

        //input.save(RawFileFormat(Vector(0, 0)), File("../../input.raw"));

        Bitmap<Vector3<float> > linearInput(input.size());
        input.convert(linearInput, ConvertSRGBToLinear());
        Array<Any> sizeArray = config.get<List<Any> >("outputSize");
        Vector size(sizeArray[0].value<int>(), sizeArray[1].value<int>());
        Bitmap<Vector3<float> > linearOutput(size);
        if (config.get<bool>("subpixels"))
            linearInput.subPixelResample(linearOutput,
                config.get<bool>("tripleResolution"));
        else
            linearInput.resample(linearOutput);
        Bitmap<SRGB> output(linearOutput.size());
        linearOutput.convert(output, ConvertLinearToSRGB());
        output.save(png, File(config.get<String>("outputPicture")));
    }
private:
    class ConvertSRGBToLinear
    {
    public:
        ConvertSRGBToLinear() : _c(ColourSpace::rgb()) { }
        Vector3<float> convert(SRGB c)
        {
            //return Vector3<float>(pow(c.x/255.0, 1/2.2), pow(c.y/255.0, 1/2.2), pow(c.z/255.0, 1/2.2));
            return Vector3Cast<float>(_c.fromSrgb(c));
        }
    private:
        ColourSpace _c;
    };
    class ConvertLinearToSRGB
    {
    public:
        ConvertLinearToSRGB() : _c(ColourSpace::rgb()) { }
        SRGB convert(Vector3<float> c)
        {
            return _c.toSrgb24(c);
            //return SRGB(
            //    static_cast<Byte>(clamp(0, static_cast<int>(pow(static_cast<double>(c.x), 2.2)*255.0), 255)),
            //    static_cast<Byte>(clamp(0, static_cast<int>(pow(static_cast<double>(c.y), 2.2)*255.0), 255)),
            //    static_cast<Byte>(clamp(0, static_cast<int>(pow(static_cast<double>(c.z), 2.2)*255.0), 255)));
        }
    private:
        ColourSpace _c;
    };
};
