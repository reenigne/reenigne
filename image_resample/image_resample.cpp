#include "unity/main.h"
#include "unity/file.h"
#include "unity/colour_space.h"
#include <stdio.h>
#include "unity/bitmap.h"
#include "unity/config_file.h"

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

        Bitmap<SRGB> input;
        input.load(File(config.get<String>("inputPicture")));
        Bitmap<Vector3<float> > linearInput;
        input.convert(&linearInput, ConvertSRGBToLinear());
        Array<Any> sizeArray = config.get<List<Any> >("outputSize");
        Vector size(sizeArray[0].value<int>(), sizeArray[1].value<int>());
        Bitmap<Vector3<float> > linearOutput(size);
        if (config.get<bool>("subpixels"))
            linearInput.subPixelResample(&linearOutput,
                config.get<bool>("tripleResolution"));
        else
            linearInput.resample(&linearOutput);
        Bitmap<SRGB> output;
        linearOutput.convert(&output, ConvertLinearToSRGB());
        output.save(File(config.get<String>("outputPicture")));
    }
private:
    class ConvertSRGBToLinear
    {
    public:
        ConvertSRGBToLinear() : _c(ColourSpace::rgb()) { }
        Vector3<float> convert(SRGB c)
        {
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
        }
    private:
        ColourSpace _c;
    };
};