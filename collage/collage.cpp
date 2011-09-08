#include "unity/main.h"
#include "unity/file.h"
#include "unity/colour_space.h"
#include <stdio.h>
#include "unity/bitmap.h"

enum Atom
{
    atomBoolean,
    atomInteger,
    atomString,
    atomEnumeration,
    atomEnumeratedValue,
    atomEnumeratedValueRecord,
    atomStructure,
    atomStructureEntry,
    atomTuple,

    atomValue,
    atomIdentifier,
    atomTrue,
    atomFalse,

    atomOption,

    atomLast
};

String atomToString(Atom atom)
{
    class LookupTable
    {
    public:
        LookupTable()
        {
            _table[atomBoolean] = String("Boolean");
            _table[atomInteger] = String("Integer");
            _table[atomString] = String("String");
            _table[atomEnumeration] = String("Enumeration");
            _table[atomEnumeratedValue] = String("EnumeratedValue");
            _table[atomEnumeratedValueRecord] =
                String("EnumeratedValueRecord");
            _table[atomStructure] = String("Structure");
            _table[atomStructureEntry] = String("StructureEntry");
            _table[atomTuple] = String("Tuple");

            _table[atomValue] = String("value");
            _table[atomIdentifier] = String("identifier");                           
            _table[atomTrue] = String("true");
            _table[atomFalse] = String("false");

            _table[atomOption] = String("option");
        }
        String lookUp(Atom atom) { return _table[atom]; }
    private:
        String _table[atomLast];
    };
    static LookupTable lookupTable;
    return lookupTable.lookUp(atom);
}

#include "unity/symbol.h"
#include "unity/config_file.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() == 1) {
            printf("Usage: collage <config file path>\n");
            return;
        }

        ConfigFile config;

        List<StructuredType::Member> vectorMembers;
        vectorMembers.add(StructuredType::Member(String("x"), IntegerType()));
        vectorMembers.add(StructuredType::Member(String("y"), IntegerType()));
        StructuredType vectorType(String("Vector"), vectorMembers);
        config.addType(vectorType);

        config.addOption("inputPicture", StringType());
        config.addOption("outputSize", vectorType);
        config.addOption("subpixels", BooleanType());
        config.addOption("tripleResolution", BooleanType());
        config.addOption("outputPicture", StringType());
        config.load(_arguments[1]);

        Bitmap<SRGB> input;
        input.load(File(config.getValue<String>("inputPicture")));
        Bitmap<Vector3<float> > linearInput;
        input.convert(&linearInput, ConvertSRGBToLinear());
        Array<Any> sizeArray = config.getValue<List<Any> >("outputSize");
        Vector size(sizeArray[0].value<int>(), sizeArray[1].value<int>());
        Bitmap<Vector3<float> > linearOutput(size);
        if (config.getValue<bool>("subpixels"))
            linearInput.subPixelResample(&linearOutput,
                config.getValue<bool>("tripleResolution"));
        else
            linearInput.resample(&linearOutput);
        Bitmap<SRGB> output;
        linearOutput.convert(&output, ConvertLinearToSRGB());
        output.save(File(config.getValue<String>("outputPicture")));
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