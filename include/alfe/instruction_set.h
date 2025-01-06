#include "alfe/main.h"
#include "alfe/pattern_database.h"

#ifndef INCLUDED_INSTRUCTION_SET_H
#define INCLUDED_INSTRUCTION_SET_H

class InstructionSet
{
public:
    void setPatternDatabase(PatternDatabase* db) { _db = db; }
protected:
    PatternDatabase* _db;
};

class X86_16_InstructionSet : public InstructionSet
{
public:
    void initialize()
    {
        addRegisterClass("WordRegister",16,"ax(ah:al) cx(ch:cl) dx(dh:dl) bx(bh:bl) sp bp si di");
        addRegisterClass("ByteRegister",8,"al cl dl bl ah ch dh bh");
        addRegisterClass("DefaultDSDereferenceableRegister",16,"si=4 di=5 bx=7");
        addRegisterClass("SegmentRegister",16,"es cs ss ds");
        addCode(
            "Void updateFlags<@T>(T r, T d, T s)"
            "{"
            "}"

            "T load<@T>(Word offset, Word segment)"
            "{"
            "    return *ReinterpretCast<T far *>((StaticCast<DWord>(segment) << 16) + offset);"
            "}"

            "void store<@T>(Word offset, Word segment, T value)"
            "{"
            "    *ReinterpretCast<T far *>((StaticCast<DWord>(segment) << 16) + offset) = value;"
            "}");

        for (int alu = 0; alu < 8; ++alu) {
            for (int width = 0; width < 2; ++width) {
                String w = (width == 0 ? "Byte" : "Word");
                String a = (width == 0 ? "al" : "ax");
                for (int segment = 0; segment < 2; ++segment) {
                    for (int mode = 0; mode < 19; ++mode) {
                        if (mode == 18 && segment != 0)
                            continue;
                        String f = eaRMFields(mode) + (mode < 4 ? "" : ", ") + w + "Register reg";
                        String overRide;
                        if (segment == 1) {
                             if (mode == 18)
                                continue;
                            f += ", SegmentRegister seg";
                            overRide = "(6, 3), (seg, 2), (1, 3), ";
                        }
                        String rmReg = eaRMSource(mode, width, segment);

                        addPattern(f,
                            overRide + "(" + decimal(width + (alu << 3)) + ", 8), " + eaBits(mode, "reg")),

                            aluMnemonic(alu) + " " + eaAssembly(mode, width, segment) + ", $reg",

                            "d = " + rmReg + ";\n"
                            "s = $reg;\n"
                            + aluCode(alu, width)
                            + (alu == 7 ? "" : rmReg + " = r;\n")));

                        addPattern(f,
                            overRide + "(" + decimal(width + 2 + (alu << 3)) + ", 8), " + eaBits(mode, "reg")),

                            aluMnemonic(alu) + " $reg, " + eaAssembly(mode, width, segment),

                            "s = " + rmReg + ";\n"
                            "d = $reg;\n"
                            + aluCode(alu, width)
                            + (alu == 7 ? "" : " $reg = r;\n")));
                    }
                    addPattern(w + " imm",
                        "(" + decimal(width + (alu << 3) + 4) + ", 8), " + (width == 0 ? "(imm, 8)" : "(imm, 16)"),

                        aluMnemonic(alu) + " " + a + ", $imm"

                        "d = " + a + ";\n"
                        "s = $imm;\n"
                        + aluCode(alu, width)
                        + (alu == 7 ? "" : a + " = r;\n")));
                }
            }
        }
        addPattern("SegmentRegister segreg",
            "(6, 3), (segreg, 2), (0, 3)",

            "push $segreg",

            "sp -= 2;\n"
            "store(ss, sp, $segreg);\n");

        addPattern("SegmentRegister segreg",
            "(7, 3), (segreg, 2), (0, 3)",

            "pop $segreg",

            "$regreg = load<Word>(ss, sp);\n"
            "sp += 2;\n");

        addPattern("",
            "(0x27, 8)",

            "daa",

            "daa();\n");

        addPattern("",
            "(0x2f, 8)",

            "das",

            "das();\n");

        addPattern("",
            "(0x27, 8)",

            "aaa",

            "aaa();\n");

        addPattern("",
            "(0x2f, 8)",

            "aas",

            "aas();\n");




    }
private:
    String aluMnemonic(int alu)
    {
        static const char* mnemonics[] = {"add", "or", "adc", "sbb", "and", "sub", "xor", "cmp");
        return mnemonics[alu];
    }
    String aluCode(int alu, int width)
    {
        String s = "Word r;\n";
        if (width == 0)
            return "Byte r;\n";
        switch (alu) {
            case 0: s += "r = d + s;";
            case 1: s += "r = d | s;";
            case 2: s += "r = d + s + (cf ? 1 : 0);";
            case 3: s += "r = d - s + (cf ? 1 : 0);");
            case 4: s += "r = d & s;";
            case 5: s += "r = d - s;";
            case 6: s += "r = d ^ s;";
            case 7: s += "r = d - s;";
        }
        return s + "\nupdateFlags(r, d, s);\n"
    }
    int eaDefaultSegment(int mode)
    {
        static const int r[] = {
            3, 3, 2, 2, 3, 3,
            3, 3, 2, 2, 3, 2,
            3, 3, 2, 2, 3, 2
            0};
        return r[mode];
    }
    String eaRMSource(int mode, int width, int segment)
    {
        String s;
        switch (mode) {
            case  0: s = "bx+si";
            case  1: s = "bx+di";
            case  2: s = "bp+si";
            case  3: s = "bp+di";
            case  4: s = "$rmReg";
            case  5: s = "$offset";
            case  6: s = "bx+si+$offset";
            case  7: s = "bx+di+$offset";
            case  8: s = "bp+si+$offset";
            case  9: s = "bp+di+$offset";
            case 10: s = "$rmReg+$offset";
            case 11: s = "bp+$offset";
            case 12: s = "bx+si+$offset";
            case 13: s = "bx+di+$offset";
            case 14: s = "bp+si+$offset";
            case 15: s = "bp+di+$offset";
            case 16: s = "$rmReg+$offset";
            case 17: s = "bp+$offset";
            default: return "$rmReg";
        }
        return (width == 0 ? "load<Byte>(" : "load<Word>(") + s + ", " + (segment == 0 ? (eaDefaultSegment(mode) == 3 ? "ds" : "ss") : "$seg") + ")";
    }
    String eaFields(int mode, int width)
    {
        switch (mode) {
            case  0: return "";
            case  1: return "";
            case  2: return "";
            case  3: return "";
            case  4: return "DefaultDSDereferenceableRegister rmReg";
            case  5: return "Word offset";
            case  6: return "SInt8 offset";
            case  7: return "SInt8 offset";
            case  8: return "SInt8 offset";
            case  9: return "SInt8 offset";
            case 10: return "DefaultDSDereferenceableRegister rmReg, SInt8 offset";
            case 11: return "SInt8 offset";
            case 12: return "Word offset";
            case 13: return "Word offset";
            case 14: return "Word offset";
            case 15: return "Word offset";
            case 16: return "DefaultDSDereferenceableRegister rmReg, Word offset";
            case 17: return "Word offset";
        }
        if (width == 0)
            return "ByteRegister rmReg";
        return "WordRegister rmReg";
    }
    String eaAssembly(int mode, int width, int segment)
    {
        String w = (width == 0 ? "b" : "w");
        String s;
        if (segment == 1)
            s = "$seg:";
        switch (mode) {
            case  0: return w + "[" + s + "bx+si]";
            case  1: return w + "[" + s + "bx+di]";
            case  2: return w + "[" + s + "bp+si]";
            case  3: return w + "[" + s + "bp+di]";
            case  4: return w + "[" + s + "$rmReg]";
            case  5: return w + "[" + s + "$offset]";
            case  6: return w + "[" + s + "bx+si+$offset]";
            case  7: return w + "[" + s + "bx+di+$offset]";
            case  8: return w + "[" + s + "bp+si+$offset]";
            case  9: return w + "[" + s + "bp+di+$offset]";
            case 10: return w + "[" + s + "$rmReg+$offset]";
            case 11: return w + "[" + s + "bp+$offset]";
            case 12: return w + "[" + s + "bx+si+$offset]";
            case 13: return w + "[" + s + "bx+di+$offset]";
            case 14: return w + "[" + s + "bp+si+$offset]";
            case 15: return w + "[" + s + "bp+di+$offset]";
            case 16: return w + "[" + s + "$rmReg+$offset]";
            case 17: return w + "[" + s + "bp+$offset]";
        }
        return "$rmReg";
    }
    String modRMByte(String rm, String reg, String mod)
    {
        return "(" + rm + ", 3), (" + rName + ", 3), (" + mod + ", 2)";
    }
    String eaBits(int mode, String rName)
    {
        switch (mode) {
            case  0: return modRMByte("0", rName, "0");
            case  1: return modRMByte("1", rName, "0");
            case  2: return modRMByte("2", rName, "0");
            case  3: return modRMByte("3", rName, "0");
            case  4: return modRMByte("rmReg", rName, "0");
            case  5: return modRMByte("6", rName, "0") + ", (offset, 16)";
            case  6: return modRMByte("0", rName, "1") + ", (offset, 8)";
            case  7: return modRMByte("1", rName, "1") + ", (offset, 8)";
            case  8: return modRMByte("2", rName, "1") + ", (offset, 8)";
            case  9: return modRMByte("3", rName, "1") + ", (offset, 8)";
            case 10: return modRMByte("rmReg", rName, "1") + ", (offset, 8)";
            case 11: return modRMByte("6", rName, "1") + ", (offset, 8)";
            case 12: return modRMByte("0", rName, "2") + ", (offset, 16)";
            case 13: return modRMByte("1", rName, "2") + ", (offset, 16)";
            case 14: return modRMByte("2", rName, "2") + ", (offset, 16)";
            case 15: return modRMByte("3", rName, "2") + ", (offset, 16)";
            case 16: return modRMByte("rmReg", rName, "2") + ", (offset, 16)";
            case 17: return modRMByte("0", rName, "2") + ", (offset, 16)";
        }
        return modRMByte("rmReg", rName, "3");
    }
};

#endif // INCLUDED_INSTRUCTION_SET_H
