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
        ConfigFile config;

        SymbolList vectorComponents;
        vectorComponents.add(
            Symbol(atomStructureEntry, Symbol(atomInteger), String("x")));
        vectorComponents.add(
            Symbol(atomStructureEntry, Symbol(atomInteger), String("y")));
        Symbol vectorType(atomStructure, String("Vector"),
            SymbolArray(vectorComponents));
        config.addType(vectorType);

        config.addOption("inputPicture", Symbol(atomString));
        config.addOption("outputSize", vectorType);
        config.addOption("subpixels", Symbol(atomBoolean));
        config.addOption("tripleResolution", Symbol(atomBoolean));
        config.addOption("outputPicture", Symbol(atomString));
        config.load(_arguments[1]);

        Bitmap<SRGB> input;
        input.load(File(config.getString("inputPicture")));
        Bitmap<Vector3<float> > linearInput;
        input.convert(&linearInput, ConvertSRGBToLinear());
        Symbol sizeSymbol = config.getSymbol("outputSize");
        Vector size(sizeSymbol[1].array()[0].integer(),
            sizeSymbol[1].array()[1].integer());
        Bitmap<Vector3<float> > linearOutput(size);
        if (config.getBoolean("subpixels"))
            linearInput.resample(&linearOutput);
        else
            linearInput.subPixelResample(&linearOutput,
                config.getBoolean("tripleResolution"));
        Bitmap<SRGB> output;
        linearOutput.convert(&output, ConvertLinearToSRGB());
        output.save(File(config.getString("outputPicture")));
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