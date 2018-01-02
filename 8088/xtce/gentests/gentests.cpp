#include "alfe/main.h"
#include "alfe/space.h"

class Instruction
{
public:
    Instruction() { }
    Instruction(Byte opcode, Byte modrm = 0, Word offset = 0,
        Word immediate = 0)
      : _opcode(opcode), _modrm(modrm), _offset(offset), _immediate(immediate)
    {
    }
    bool hasModrm()
    {
        static const Byte hasModrmTable[] = {
            1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
            1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
            1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
            1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1};
        return hasModrmTable[_opcode] != 0;
    }
    int immediateBytes()
    {
        static const Byte immediateBytesTable[] = {
            0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0,
            0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0,
            0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0,
            0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0, 1, 2, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0,
            2, 2, 2, 2, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 0,
            1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 0, 2, 0, 1, 1, 0, 0, 2, 0, 2, 0, 0, 1, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 4, 1, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        return immediateBytesTable[_opcode];
    }
    int modRMLength()
    {
        static const Byte modRMLengthsTable[] = {
            0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 
            0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 
            0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 
            0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        if (hasModrm())
            return 1 + modRMLengthsTable[_modrm];
        return 0;
    }
    int length()
    {
        return 1 + modRMLength() + immediateBytes();
    }
    int defaultSegment()
    {
        if (hasModrm() && _opcode != 0x8d) {
            static const signed char defaultSegmentTable[] = {
                3, 3, 2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 3, 3, 
                3, 3, 2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 3, 3, 
                3, 3, 2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 3, 3, 
                3, 3, 2, 2, 3, 3, 3, 3, 3, 3, 2, 2, 3, 3, 3, 3, 
                3, 3, 2, 2, 3, 3, 2, 3, 3, 3, 2, 2, 3, 3, 2, 3, 
                3, 3, 2, 2, 3, 3, 2, 3, 3, 3, 2, 2, 3, 3, 2, 3, 
                3, 3, 2, 2, 3, 3, 2, 3, 3, 3, 2, 2, 3, 3, 2, 3, 
                3, 3, 2, 2, 3, 3, 2, 3, 3, 3, 2, 2, 3, 3, 2, 3, 
                3, 3, 2, 2, 3, 3, 2, 3, 3, 3, 2, 2, 3, 3, 2, 3, 
                3, 3, 2, 2, 3, 3, 2, 3, 3, 3, 2, 2, 3, 3, 2, 3, 
                3, 3, 2, 2, 3, 3, 2, 3, 3, 3, 2, 2, 3, 3, 2, 3, 
                3, 3, 2, 2, 3, 3, 2, 3, 3, 3, 2, 2, 3, 3, 2, 3, 
               -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
               -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
               -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
               -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
            return defaultSegmentTable[_modrm];
        }
        int o = _opcode & 0xfe;
        if (o == 0xa0 || o == 0xa2 || o == 0xa4 || o == 0xa6 || o == 0xac ||
            _opcode == 0xd7)
            return 3;
        return -1;

    }
    void output(Byte* p)
    {
        *p = _opcode;
        ++p;
        if (hasModrm()) {
            *p = _modrm;
            ++p;
            int l = modRMLength();
            if (l > 0) {
                *p = (_offset & 0xff);
                if (l > 1)
                    p[1] = _offset >> 8;
                p += l;
            }
        }
        int l = immediateBytes();
        if (l > 0) {
            *p = _immediate;
            if (l > 1) {
                p[1] = _immediate >> 8;
                if (l == 4) {
                    p[2] = _immediate >> 16;
                    p[3] = _immediate >> 24;
                }
            }
        }
    }
    bool isGroup()
    {
        int o = _opcode & 0xfe;
        return o == 0xf6 || o == 0xfe || o == 0x80 || o == 0x82;
    }  
    void write()
    {
        console.write("{" + hex(_opcode, 2) + ", " + hex(_modrm, 2) + ", " +
            hex(_offset, 4) + ", " + hex(_immediate) + "}");
    }
    
private:
    Byte _opcode;
    Byte _modrm;
    Word _offset;
    DWord _immediate;
};

class Test
{
public:
    Test(int queueFiller = 0, int nops = 0)
      : _queueFiller(queueFiller), _nops(nops) { }
    void addInstruction(Instruction instruction)
    {
        _instructions.append(instruction);
    }
    int length()
    {
        int l = 0;
        for (auto i : _instructions)
            l += i.length();
        return l;
    }
    void output(Byte* p)
    {
        *p = (_queueFiller << 5) + _nops;
        ++p;
        *p = length();
        ++p;
        outputBytes(p);
    }
    Byte* outputBytes(Byte* p)
    {
        for (auto i : _instructions) {
            int l = i.length();
            i.output(p);
            p += l;
        }
        return p;
    }
    Byte* outputCode(Byte* p)
    {
        switch (_queueFiller) {
            case 0:
                p[0] = 0xb0;
                p[1] = 0x00;
                p[2] = 0xf6;
                p[3] = 0xe0;
                p += 4;
                break;
            default:
                throw Exception("Unknown queue filler.");
        }
        for (int i = 0; i < _nops; ++i) {
            *p = 0x90;
            ++p;
        }
        p = outputBytes(p);
        p[0] = 0xeb;
        p[1] = 0x00;
        p[2] = 0xcd;
        p[3] = 0xff;
        return p + 4;
    }
    void write()
    {
        bool first = true;
        console.write("{{");
        for (auto i : _instructions) {
            if (!first)
                console.write(",\n");
            else
                console.write("  ");
            i.write();
            first = false;
        }
        console.write("}, " + decimal((_queueFiller << 5) + _nops) + "}\n");
    }
private:
    int _queueFiller;
    int _nops;
    AppendableArray<Instruction> _instructions;
};

class Disassembler
{
public:
    Disassembler() : _byteCount(0) { }
    String disassemble(Byte byte, bool firstByte)
    {
        String bytes;
        if (firstByte) {
            if (_byteCount != 0)
                bytes = "!a";
            _byteCount = 0;
        }
        _code[_byteCount] = byte;
        ++_byteCount;
        _lastOffset = 0;
        String instruction = disassembleInstruction();
        if (_lastOffset >= _byteCount)
            return bytes;  // We don't have the complete instruction yet
        _byteCount = 0;
        for (int i = 0; i <= _lastOffset; ++i)
            bytes += hex(_code[i], 2, false);
        return bytes.alignLeft(12) + " " + instruction;
    }
private:
    String disassembleInstruction()
    {
        _wordSize = (opcode() & 1) != 0;
        _doubleWord = false;
        _offset = 1;
        if ((opcode() & 0xc4) == 0)
            return alu(op1()) + regMemPair();
        if ((opcode() & 0xc6) == 4)
            return alu(op1()) + accum() + ", " + imm();
        if ((opcode() & 0xe7) == 6)
            return "PUSH " + segreg(op1());
        if ((opcode() & 0xe7) == 7)
            return "POP " + segreg(op1());
        if ((opcode() & 0xe7) == 0x26)
            return segreg(op1() & 3) + ":";
        if ((opcode() & 0xf8) == 0x40)
            return "INC " + rwo();
        if ((opcode() & 0xf8) == 0x48)
            return "DEC " + rwo();
        if ((opcode() & 0xf8) == 0x50)
            return "PUSH " + rwo();
        if ((opcode() & 0xf8) == 0x58)
            return "POP " + rwo();
        if ((opcode() & 0xfc) == 0x80)
            return alu(reg()) + ea() + ", " +
            (opcode() == 0x81 ? iw(true) : sb(true));
        if ((opcode() & 0xfc) == 0x88)
            return "MOV " + regMemPair();
        if ((opcode() & 0xf8) == 0x90)
            if (opcode() == 0x90)
                return "NOP";
            else
                return "XCHG AX, " + rwo();
        if ((opcode() & 0xf8) == 0xb0)
            return "MOV " + rbo() + ", " + ib();
        if ((opcode() & 0xf8) == 0xb8)
            return "MOV " + rwo() + ", " + iw();
        if ((opcode() & 0xfc) == 0xd0) {
            static String shifts[8] = {
                "ROL", "ROR", "RCL", "RCR", "SHL", "SHR", "SHL", "SAR"};
            return shifts[reg()] + " " + ea() + ", " +
                ((op0() & 2) == 0 ? String("1") : byteRegs(1));
        }
        if ((opcode() & 0xf8) == 0xd8) {
            _wordSize = false;
            _doubleWord = true;
            return String("ESC ") + op0() + ", " + reg() + ", " + ea();
        }
        if ((opcode() & 0xf6) == 0xe4)
            return "IN " + accum() + ", " + port();
        if ((opcode() & 0xf6) == 0xe6)
            return "OUT " + port() + ", " + accum();
        if ((opcode() & 0xe0) == 0x60) {
            static String conds[16] = {
                "O", "NO", "B", "AE", "E", "NE", "BE", "A",
                "S", "NS", "P", "NP", "L", "GE", "LE", "G"};
            return "J" + conds[opcode() & 0xf] + " " + cb();
        }
        switch (opcode()) {
        case 0x27: return "DAA";
        case 0x2f: return "DAS";
        case 0x37: return "AAA";
        case 0x3f: return "AAS";
        case 0x84:
        case 0x85: return "TEST " + regMemPair();
        case 0x86:
        case 0x87: return "XCHG " + regMemPair();
        case 0x8c:
            _wordSize = true;
            return "MOV " + ea() + ", " + segreg(reg());
        case 0x8d:
            _doubleWord = true;
            _wordSize = false;
            return "LEA " + rw() + ", " + ea();
        case 0x8e:
            _wordSize = true;
            return "MOV " + segreg(reg()) + ", " + ea();
        case 0x8f: return "POP " + ea();
        case 0x98: return "CBW";
        case 0x99: return "CWD";
        case 0x9a: return "CALL " + cp();
        case 0x9b: return "WAIT";
        case 0x9c: return "PUSHF";
        case 0x9d: return "POPF";
        case 0x9e: return "SAHF";
        case 0x9f: return "LAHF";
        case 0xa0:
        case 0xa1: return "MOV " + accum() + ", " + size() + "[" + iw() + "]";
        case 0xa2:
        case 0xa3: return "MOV " + size() + "[" + iw() + "], " + accum();
        case 0xa4:
        case 0xa5: return "MOVS" + size();
        case 0xa6:
        case 0xa7: return "CMPS" + size();
        case 0xa8:
        case 0xa9: return "TEST " + accum() + ", " + imm();
        case 0xaa:
        case 0xab: return "STOS" + size();
        case 0xac:
        case 0xad: return "LODS" + size();
        case 0xae:
        case 0xaf: return "SCAS" + size();
        case 0xc0:
        case 0xc2: return "RET " + iw();
        case 0xc1:
        case 0xc3: return "RET";
        case 0xc4: _doubleWord = true; return "LDS " + rw() + ", " + ea();
        case 0xc5:
            _doubleWord = true;
            _wordSize = false;
            return "LES " + rw() + ", " + ea();
        case 0xc6:
        case 0xc7: return "MOV " + ea() + ", " + imm(true);
        case 0xc8:
        case 0xca: return "RETF " + iw();
        case 0xc9:
        case 0xcb: return "RETF";
        case 0xcc: return "INT 3";
        case 0xcd: return "INT " + ib();
        case 0xce: return "INTO";
        case 0xcf: return "IRET";
        case 0xd4: return "AAM " + ib();
        case 0xd5: return "AAD " + ib();
        case 0xd6: return "SALC";
        case 0xd7: return "XLATB";
        case 0xe0: return "LOOPNE " + cb();
        case 0xe1: return "LOOPE " + cb();
        case 0xe2: return "LOOP " + cb();
        case 0xe3: return "JCXZ " + cb();
        case 0xe8: return "CALL " + cw();
        case 0xe9: return "JMP " + cw();
        case 0xea: return "JMP " + cp();
        case 0xeb: return "JMP " + cb();
        case 0xf0:
        case 0xf1: return "LOCK";
        case 0xf2: return "REPNE ";
        case 0xf3: return "REP ";
        case 0xf4: return "HLT";
        case 0xf5: return "CMC";
        case 0xf6:
        case 0xf7:
            switch (reg()) {
            case 0:
            case 1: return "TEST " + ea() + ", " + imm(true);
            case 2: return "NOT " + ea();
            case 3: return "NEG " + ea();
            case 4: return "MUL " + ea();
            case 5: return "IMUL " + ea();
            case 6: return "DIV " + ea();
            case 7: return "IDIV " + ea();
            }
        case 0xf8: return "CLC";
        case 0xf9: return "STC";
        case 0xfa: return "CLI";
        case 0xfb: return "STI";
        case 0xfc: return "CLD";
        case 0xfd: return "STD";
        case 0xfe:
        case 0xff:
            switch (reg()) {
            case 0: return "INC " + ea();
            case 1: return "DEC " + ea();
            case 2: return "CALL " + ea();
            case 3: _doubleWord = true; return "CALL " + ea();
            case 4: return "JMP " + ea();
            case 5: _doubleWord = true; return "JMP " + ea();
            case 6: return "PUSH " + ea();
            case 7: return "??? " + ea();
            }
        }
        return "!b";
    }
    UInt8 getByte(int offset)
    {
        _lastOffset = max(_lastOffset, offset);
        return _code[offset];
    }
    UInt16 getWord(int offset)
    {
        return getByte(offset) | (getByte(offset + 1) << 8);
    }
    String regMemPair()
    {
        if ((op0() & 2) == 0)
            return ea() + ", " + r();
        return r() + ", " + ea();
    }
    String r() { return !_wordSize ? rb() : rw(); }
    String rb() { return byteRegs(reg()); }
    String rw() { return wordRegs(reg()); }
    String rbo() { return byteRegs(op0()); }
    String rwo() { return wordRegs(op0()); }
    String byteRegs(int r)
    {
        static String b[8] = {"AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"};
        return b[r];
    }
    String wordRegs(int r)
    {
        static String w[8] = {"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};
        return w[r];
    }
    String ea()
    {
        String s;
        switch (mod()) {
        case 0: s = disp(); break;
        case 1: s = disp() + sb(); _offset = 3; break;
        case 2: s = disp() + "+" + iw(); _offset = 4; break;
        case 3: return !_wordSize ? byteRegs(rm()) : wordRegs(rm());
        }
        return size() + "[" + s + "]";
    }
    String size()
    {
        if (!_doubleWord)
            return (!_wordSize ? "B" : "W");
        else
            return (!_wordSize ? "" : "D");
    }
    String disp()
    {
        static String d[8] = {
            "BX+SI", "BX+DI", "BP+SI", "BP+DI", "SI", "DI", "BP", "BX"};
        if (mod() == 0 && rm() == 6) {
            String s = iw();
            _offset = 4;
            return s;
        }
        return d[rm()];
    }
    String alu(int op)
    {
        static String o[8] = {
            "ADD ", "OR ", "ADC ", "SBB ", "AND ", "SUB ", "XOR ", "CMP "};
        return o[op];
    }
    Byte opcode() { return getByte(0); }
    int op0() { return opcode() & 7; }
    int op1() { return (opcode() >> 3) & 7; }
    Byte modRM() { _offset = 2; return getByte(1); }
    int mod() { return modRM() >> 6; }
    int reg() { return (modRM() >> 3) & 7; }
    int rm() { return modRM() & 7; }
    String imm(bool m = false) { return !_wordSize ? ib(m) : iw(m); }
    String iw(bool m = false)
    {
        if (m)
            ea();
        return hex(getWord(_offset), 4, false);
    }
    String ib(bool m = false)
    {
        if (m)
            ea();
        return hex(getByte(_offset), 2, false);
    }
    String sb(bool m = false)
    {
        if (m)
            ea();
        UInt8 byte = getByte(_offset);
        if ((byte & 0x80) == 0)
            return "+" + hex(byte, 2, false);
        return "-" + hex(-byte, 2, false);
    }
    String accum() { return !_wordSize ? "AL" : "AX"; }
    String segreg(int r)
    {
        static String sr[8] = {"ES", "CS", "SS", "DS", "??", "??", "??", "??"};
        return sr[r];
    }
    String cb()
    {
        return "IP" + sb();
        //hex(_address + static_cast<SInt8>(getByte(_offset)), 4, false);
    }
    String cw()
    {
        return "IP+" + iw();
        //return hex(_address + getWord(_offset), 4, false);
    }
    String cp()
    {
        return hex(getWord(_offset + 2), 4, false) + ":" +
            hex(getWord(_offset), 4, false);
    }
    String port() { return ((op1() & 1) == 0 ? ib() : wordRegs(2)); }

    UInt16 _ip;
    UInt8 _code[6];
    int _byteCount;
    bool _wordSize;
    bool _doubleWord;
    int _offset;
    int _lastOffset;
};

class Emulator
{
public:
    Emulator() : _logging(false), _ram(0xa0000)
    {
        static String b[8] = {"AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"};
        static String w[8] = {"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};
        static String s[8] = {"ES", "CS", "SS", "DS", "??", "??", "??", "??"};
        for (int i = 0; i < 8; ++i) {
            _wordRegisters[i].init(w[i], &_registerData[i]);
            _byteRegisters[i].init(b[i], reinterpret_cast<UInt8*>(
                &_registerData[i & 3]) + (i >= 4 ? 1 : 0));
            _segmentRegisters[i].init(s[i], &_segmentRegisterData[i]);
        }
        _flags.init("F", &_flagsData);
    }
    String log(Test test)
    {
        _logging = true;
        _test = test;
        run();
        return _log;
    }
    int expected(Test test)
    {
        _test = test;
        run();
        return _cycles * 2;
    }
private:
    void run()
    {
        UInt16 testSegment = 0x10a8;

        _cycles = 0;
        Byte* stopP = _test.outputCode(&_ram[testSegment << 4]);
        _stopIP = stopP - &_ram[(testSegment << 4) + 2];
        ax() = 0;
        cx() = 0;
        dx() = 0;
        bx() = 0;
        sp() = 0;
        bp() = 0;
        si() = 0;
        di() = 0;
        es() = testSegment;
        cs() = testSegment;
        ss() = testSegment;
        ds() = testSegment;
        _state = stateBegin;
        _busState = t1;

        int t = 0;  // 0 = Tidle, 1 = T1, 2 = T2, 3 = T3, 4 = T4, 5 = Tw
        int d = -1;  // -1 = SI, 0 = S0, 1 = S1, 2 = S2, 3 = S3, 4 = S4, 5 = SW
        Byte queue[4];
        int queueLength = 0;
        int lastS = 0;
        _pitCycle = 0;
        _ready = true;

        do {
            simulateCycleAction();

            switch (_busState) {
                case t1:
                    break;
                case t2:
                    _cpu_ad = _busAddress;
                    _bus_address = _cpu_ad;
                    break;
                case t3:
                    _cpu_ad &= 0xcffff;
                    switch (_busSegment) {
                        case 0:  // ES
                            break;
                        case 1:  // CS or none
                            _cpu_ad |= 0x20000;
                            break;
                        case 2:  // SS
                            _cpu_ad |= 0x10000;
                            break;
                        case 3:  // DS
                            _cpu_ad |= 0x30000;
                            break;
                    }

                    if (_ioInProgress == ioWrite) {
                        _cpu_ad = (_cpu_ad & 0xfff00) | _busData;
                        _bus_data = _busData;
                    }
                    break;
                case t4:
                    if (_ioInProgress != ioWrite) {
                        _cpu_ad = (_cpu_ad & 0xfff00) | _busData;
                        _bus_data = _busData;
                    }
                    break;


                        
            }
            if (_busState != t2)
                _cpu_ad = (_cpu_ad & 0x3ffff) | (intf() ? 0x40000 : 0);

            _cpu_rqgt0 = false;  // Used by 8087 for bus mastering, NYI
            _cpu_ready = true;   // Used for DMA and wait states, NYI
            _cpu_test = false;   // Used by 8087 for synchronization, NYI
            _bus_dma = 0;        // NYI
            _bus_irq = 0xfc;     // NYI
            _bus_pit = _pitCycle < 2 ? 4 : 5;
            _bus_iochrdy = true; // Used for wait states, NYI
            _bus_aen = false;    // Used for DMA, NYI
            _bus_tc = false;     // Used for DMA, NYI

            char qsc[] = ".IES";
            char sc[] = "ARWHCrwp";
            String line = String(hex(_cpu_ad, 5, false)) + " " +
                codePoint(qsc[_cpu_qs]) + codePoint(sc[_cpu_s]) +
                (_cpu_rqgt0 ? "G" : ".") + (_cpu_ready ? "." : "z") +
                (_cpu_test ? "T" : ".") +
                "  " + hex(_bus_address, 5, false) + " " +
                hex(_bus_data, 2, false) + " " + hex(_bus_dma, 2, false) +
                " " + hex(_bus_irq, 2, false) + " " +
                hex(_bus_pit, 1, false) + " " + (_bus_ior ? "R" : ".") +
                (_bus_iow ? "W" : ".") + (_bus_memr ? "r" : ".") +
                (_bus_memw ? "w" : ".") + (_bus_iochrdy ? "." : "z") +
                (_bus_aen ? "D" : ".") +
                (_bus_tc ? "T" : ".");
            line += "  ";
            if (_cpu_s != 7 && _cpu_s != 3)
                switch (t) {
                    case 0:
                    case 4:
                        // T1 state occurs after transition out of passive
                        t = 1;
                        break;
                    case 1:
                        t = 2;
                        break;
                    case 2:
                        t = 3;
                        break;
                    case 3:
                        t = 5;
                        break;
                }
            else
                switch (t) {
                    case 4:
                        d = -1;
                    case 0:
                        t = 0;
                        break;
                    case 1:
                    case 2:
                        t = 6;
                        break;
                    case 3:
                    case 5:
                        d = -1;
                        t = 4;
                        break;
                }
            switch (t) {
                case 0: line += "  "; break;
                case 1: line += "T1"; break;
                case 2: line += "T2"; break;
                case 3: line += "T3"; break;
                case 4: line += "T4"; break;
                case 5: line += "Tw"; break;
                default: line += "!c"; t = 0; break;
            }
            line += " ";
            if (_bus_aen)
                switch (d) {
                    // This is a bit of a hack since we don't have access
                    // to the right lines to determine the DMA state
                    // properly. This probably breaks for memory-to-memory
                    // copies.
                    case -1: d = 0; break;
                    case 0: d = 1; break;
                    case 1: d = 2; break;
                    case 2: d = 3; break;
                    case 3:
                    case 5:
                        if ((_bus_iow && _bus_memr) || (_bus_ior && _bus_memw))
                            d = 4;
                        else
                            d = 5;
                        break;
                    case 4:
                        d = -1;
                }
            switch (d) {
                case -1: line += "  "; break;
                case 0: line += "S0"; break;
                case 1: line += "S1"; break;
                case 2: line += "S2"; break;
                case 3: line += "S3"; break;
                case 4: line += "S4"; break;
                case 5: line += "SW"; break;
                default: line += "!d"; t = 0; break;
            }
            line += " ";
            String instruction;
            if (_cpu_qs != 0) {
                if (_cpu_qs == 2)
                    queueLength = 0;
                else {
                    Byte b = queue[0];
                    for (int i = 0; i < 3; ++i)
                        queue[i] = queue[i + 1];
                    --queueLength;
                    if (queueLength < 0) {
                        line += "!g";
                        queueLength = 0;
                    }
                    instruction = _disassembler.disassemble(b, _cpu_qs == 1);
                }
            }
            if (t == 4 || d == 4) {
                if (t == 4 && d == 4)
                    line += "!e";
                String seg;
                switch (_cpu_ad & 0x30000) {
                    case 0x00000: seg = "ES "; break;
                    case 0x10000: seg = "SS "; break;
                    case 0x20000: seg = "CS "; break;
                    case 0x30000: seg = "DS "; break;
                }
                String type = "-";
                if (lastS == 0)
                    line += hex(_bus_data, 2, false) + " <-i           ";
                else {
                    if (lastS == 4) {
                        type = "f";
                        seg = "   ";
                    }
                    if (d == 4) {
                        type = "d";
                        seg = "   ";
                    }
                    line += hex(_bus_data, 2, false) + " ";
                    if (_bus_ior || _bus_memr)
                        line += "<-" + type + " ";
                    else
                        line += type + "-> ";
                    if (_bus_memr || _bus_memw)
                        line += "[" + seg + hex(_bus_address, 5, false) + "]";
                    else
                        line += "port[" + hex(_bus_address, 4, false) + "]";
                    if (lastS == 4 && d != 4) {
                        if (queueLength >= 4)
                            line += "!f";
                        else {
                            queue[queueLength] = _bus_data;
                            ++queueLength;
                        }
                    }
                }
                line += " ";
            }
            else
                line += "                  ";
            if (_cpu_qs != 0)
                line += codePoint(qsc[_cpu_qs]);
            else
                line += " ";
            line += " " + instruction + "\n";
            lastS = _cpu_s;
            console.write(line);

            //++_cycles;
            _pitCycle = (_pitCycle + 1) & 3;
        } while (_ip != _stopIP);
    }
    void simulateCycleAction()
    {
        _cpu_qs = 0;
        bool busDone;
        do {
            busDone = true;
            switch (_busState) {
                case t1:
                    if (_ioInProgress == ioInstructionFetch) {
                        _busAddress = physicalAddress(1, _prefetchAddress);
                        //_bus->setAddressReadMemory(_tick, _busAddress);
                    }
                    else {
                        int segment = _segment;
                        if (_segmentOverride != -1)
                            segment = _segmentOverride;
                        _busSegment = segment;
                        _busAddress = physicalAddress(segment, _address);
                        //if (_usePortSpace) {
                        //    if (_ioInProgress == ioWrite)
                        //        _bus->setAddressWriteIO(_tick, _busAddress);
                        //    else
                        //        _bus->setAddressReadIO(_tick, _busAddress);
                        //}
                        //else {
                        //    if (_ioInProgress == ioWrite) {
                        //        _bus->setAddressWriteMemory(_tick,
                        //            _busAddress);
                        //    }
                        //    else
                        //        _bus->setAddressReadMemory(_tick, _busAddress);
                        //}
                    }
                    _busState = t2;
                    _bus_ior = false;
                    _bus_iow = false;
                    _bus_memr = false;
                    _bus_memw = false;
                    break;
                case t2:
                    if (_ioInProgress == ioWrite) {
                        _ioRequested = ioNone;
                        switch (_byte) {
                            case ioSingleByte:
                                _busData = _data;
                                _state = _afterIO;
                                break;
                            case ioWordFirst:
                                _busData = _data;
                                _ioInProgress = ioWrite;
                                _byte = ioWordSecond;
                                ++_address;
                                break;
                            case ioWordSecond:
                                _busData = _data >> 8;
                                _state = _afterIO;
                                _byte = ioSingleByte;
                                break;
                        }
                        //if (_usePortSpace)
                        //    _bus->writeIO(_tick, _busData);
                        //else
                        //    _bus->writeMemory(_tick, _busData);
                        if (!_usePortSpace)
                            _ram[_busAddress] = _busData;
                    }
                    _busState = t3;
                    if (_usePortSpace) {
                        if (_ioInProgress == ioWrite)
                            _bus_iow = true;
                        else
                            _bus_ior = true;
                    }
                    else {
                        if (_ioInProgress == ioWrite)
                            _bus_memw = true;
                        else
                            _bus_memr = true;
                    }
                    break;
                case t3:
                    _busState = tWait;
                    busDone = false;
                    break;
                case tWait:
                    if (!_ready)
                        break;
                    _busState = t4;
                    _cpu_s = 7;
                    if (_ioInProgress == ioInstructionFetch) {
                        if (_abandonFetch)
                            break;
                        _busData = _ram[_busAddress];
                            //_bus->readMemory(_tick);
                        _prefetchQueue[(_prefetchOffset + _prefetched) & 3] =
                            _busData;
                        ++_prefetched;
                        ++_prefetchAddress;
                        completeInstructionFetch();
                        break;
                    }
                    if (_ioInProgress == ioWrite)
                        break;
                    if (_ioInProgress == ioInterruptAcknowledge)
                        _data = 8; //_pic->readAcknowledgeByte(_tick);
                    else
                        if (_usePortSpace)
                            _busData = 0xff; //_bus->readIO(_tick);
                        else
                            _busData = _ram[_busAddress]; //_bus->readMemory(_tick);
                    _ioRequested = ioNone;
                    switch (_byte) {
                        case ioSingleByte:
                            _data = _busData;
                            _state = _afterIO;
                            break;
                        case ioWordFirst:
                            _data = _busData;
                            _ioInProgress = ioRead;
                            _byte = ioWordSecond;
                            ++_address;
                            break;
                        case ioWordSecond:
                            _data |= static_cast<UInt16>(_busData) << 8;
                            _state = _afterIO;
                            _byte = ioSingleByte;
                            break;
                    }
                    break;
                case t4:
                    _busState = tIdle;
                    busDone = false;
                    break;
                case tIdle:
                    if (_byte == ioWordSecond)
                        _busState = t1;
                    else
                        _ioInProgress = ioNone;
                    _abandonFetch = false;
                    if (_ioInProgress == ioNone && _ioRequested != ioNone) {
                        _ioInProgress = _ioRequested;
                        _busState = t1;
                    }
                    if (_ioInProgress == ioNone && _prefetched < 4) {
                        _ioInProgress = ioInstructionFetch;
                        _busState = t1;
                    }
                    if (_busState == t1) {
                        switch (_ioInProgress) {
                            case ioRead:
                                _cpu_s = _usePortSpace ? 1 : 5;
                                break;
                            case ioWrite:
                                _cpu_s = _usePortSpace ? 2 : 6;
                                break;
                            case ioInterruptAcknowledge:
                                _cpu_s = 0;
                                break;
                            case ioInstructionFetch:
                                _cpu_s = 4;
                                break;
                        }
                    }
                    busDone = true;
                    _bus_ior = false;
                    _bus_iow = false;
                    _bus_memr = false;
                    _bus_memw = false;
                    break;
            }
        } while (!busDone);
        do {
            if (_wait > 0) {
                --_wait;
                return;
            }
            switch (_state) {
                case stateWaitingForBIU:
                    return;

                case stateBegin: fetch(stateDecodeOpcode, false); break;
                case stateDecodeOpcode:
                    {
                        _cpu_qs = 1;
                        _opcode = _data;
                        _wordSize = ((_opcode & 1) != 0);
                        _sourceIsRM = ((_opcode & 2) != 0);
                        static State stateForOpcode[0x100] = {
                            stateALU,          stateALU,          stateALU,          stateALU,
                            stateALUAccumImm,  stateALUAccumImm,  statePushSegReg,   statePopSegReg,
                            stateALU,          stateALU,          stateALU,          stateALU,
                            stateALUAccumImm,  stateALUAccumImm,  statePushSegReg,   statePopSegReg,
                            stateALU,          stateALU,          stateALU,          stateALU,
                            stateALUAccumImm,  stateALUAccumImm,  statePushSegReg,   statePopSegReg,
                            stateALU,          stateALU,          stateALU,          stateALU,
                            stateALUAccumImm,  stateALUAccumImm,  statePushSegReg,   statePopSegReg,
                            stateALU,          stateALU,          stateALU,          stateALU,
                            stateALUAccumImm,  stateALUAccumImm,  stateSegOverride,  stateDAA,
                            stateALU,          stateALU,          stateALU,          stateALU,
                            stateALUAccumImm,  stateALUAccumImm,  stateSegOverride,  stateDAS,
                            stateALU,          stateALU,          stateALU,          stateALU,
                            stateALUAccumImm,  stateALUAccumImm,  stateSegOverride,  stateAAA,
                            stateALU,          stateALU,          stateALU,          stateALU,
                            stateALUAccumImm,  stateALUAccumImm,  stateSegOverride,  stateAAS,
                            stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,
                            stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,
                            stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,
                            stateIncDecRW,     stateIncDecRW,     stateIncDecRW,     stateIncDecRW,
                            statePushRW,       statePushRW,       statePushRW,       statePushRW,
                            statePushRW,       statePushRW,       statePushRW,       statePushRW,
                            statePopRW,        statePopRW,        statePopRW,        statePopRW,
                            statePopRW,        statePopRW,        statePopRW,        statePopRW,
                            stateJCond,        stateJCond,        stateJCond,        stateJCond,
                            stateJCond,        stateJCond,        stateJCond,        stateJCond,
                            stateJCond,        stateJCond,        stateJCond,        stateJCond,
                            stateJCond,        stateJCond,        stateJCond,        stateJCond,
                            stateJCond,        stateJCond,        stateJCond,        stateJCond,
                            stateJCond,        stateJCond,        stateJCond,        stateJCond,
                            stateJCond,        stateJCond,        stateJCond,        stateJCond,
                            stateJCond,        stateJCond,        stateJCond,        stateJCond,
                            stateALURMImm,     stateALURMImm,     stateALURMImm,     stateALURMImm,
                            stateTestRMReg,    stateTestRMReg,    stateXchgRMReg,    stateXchgRMReg,
                            stateMovRMReg,     stateMovRMReg,     stateMovRegRM,     stateMovRegRM,
                            stateMovRMWSegReg, stateLEA,          stateMovSegRegRMW, statePopMW,
                            stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,
                            stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,     stateXchgAxRW,
                            stateCBW,          stateCWD,          stateCallCP,       stateWait,
                            statePushF,        statePopF,         stateSAHF,         stateLAHF,
                            stateMovAccumInd,  stateMovAccumInd,  stateMovIndAccum,  stateMovIndAccum,
                            stateMovS,         stateMovS,         stateCmpS,         stateCmpS,
                            stateTestAccumImm, stateTestAccumImm, stateStoS,         stateStoS,
                            stateLodS,         stateLodS,         stateScaS,         stateScaS,
                            stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,
                            stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,
                            stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,
                            stateMovRegImm,    stateMovRegImm,    stateMovRegImm,    stateMovRegImm,
                            stateInvalid,      stateInvalid,      stateRet,          stateRet,
                            stateLoadFar,      stateLoadFar,      stateMovRMImm,     stateMovRMImm,
                            stateInvalid,      stateInvalid,      stateRet,          stateRet,
                            stateInt3,         stateInt,          stateIntO,         stateIRet,
                            stateShift,        stateShift,        stateShift,        stateShift,
                            stateAAM,          stateAAD,          stateSALC,         stateXlatB,
                            stateEscape,       stateEscape,       stateEscape,       stateEscape,
                            stateEscape,       stateEscape,       stateEscape,       stateEscape,
                            stateLoop,         stateLoop,         stateLoop,         stateLoop,
                            stateInOut,        stateInOut,        stateInOut,        stateInOut,
                            stateCallCW,       stateJmpCW,        stateJmpCP,        stateJmpCB,
                            stateInOut,        stateInOut,        stateInOut,        stateInOut,
                            stateLock,         stateInvalid,      stateRep,          stateRep,
                            stateHlt,          stateCmC,          stateMath,         stateMath,
                            stateLoadC,        stateLoadC,        stateLoadI,        stateLoadI,
                            stateLoadD,        stateLoadD,        stateMisc,         stateMisc};
                        _state = stateForOpcode[_opcode];
                    }
                    break;
                case stateEndInstruction:
                    _segmentOverride = -1;
                    _rep = 0;
                    _usePortSpace = false;
                    _newInstruction = true;
                    _newIP = _ip;
                    _afterCheckInt = stateBegin;
                    _state = stateCheckInt;
                    break;

                case stateBeginModRM: fetch(stateDecodeModRM, false); break;
                case stateDecodeModRM:
                    _modRM = _data;
                    if ((_modRM & 0xc0) == 0xc0) {
                        _useMemory = false;
                        _address = _modRM & 7;
                        _state = _afterEA;
                        break;
                    }
                    _useMemory = true;
                    switch (_modRM & 7) {
                        case 0: _wait = 7; _address = bx() + si(); break;
                        case 1: _wait = 8; _address = bx() + di(); break;
                        case 2: _wait = 8; _address = bp() + si(); break;
                        case 3: _wait = 7; _address = bp() + di(); break;
                        case 4: _wait = 5; _address =        si(); break;
                        case 5: _wait = 5; _address =        di(); break;
                        case 6: _wait = 5; _address = bp();        break;
                        case 7: _wait = 5; _address = bx();        break;
                    }
                    switch (_modRM & 0xc0) {
                        case 0x00:
                            if ((_modRM & 7) == 6)
                                fetch(stateEAOffset, true);
                            else
                                _state = stateEASetSegment;
                            break;
                        case 0x40: fetch(stateEAByte, false); break;
                        case 0x80: fetch(stateEAWord, true); break;
                    }
                    break;
                case stateEAOffset:
                    _address = _data;
                    _segment = 3;
                    _wait = 6;
                    _state = _afterEA;
                    break;
                case stateEAByte:
                    _address += signExtend(_data);
                    _wait += 4;
                    _state = stateEASetSegment;
                    break;
                case stateEAWord:
                    _address += _data;
                    _wait += 4;
                    _state = stateEASetSegment;
                    break;
                case stateEASetSegment:
                    {
                        static int segments[8] = {3, 3, 2, 2, 3, 3, 2, 3};
                        _segment = segments[_modRM & 7];
                    }
                    _state = _afterEA;
                    break;

                case stateEAIO:
                    if (_useMemory)
                        initIO(_afterEAIO, _ioType, _wordSize);
                    else {
                        if (!_wordSize)
                            if (_ioType == ioRead)
                                _data = _byteRegisters[_address];
                            else
                                _byteRegisters[_address] = _data;
                        else
                            if (_ioType == ioRead)
                                _data = _wordRegisters[_address];
                            else
                                _wordRegisters[_address] = _data;
                        _state = _afterEAIO;
                    }
                    break;

                case statePush:
                    sp() -= 2;
                case statePush2:
                    _address = sp();
                    _segment = 2;
                    initIO(_afterIO, ioWrite, true);
                    break;
                case statePop:
                    _address = sp();
                    sp() += 2;
                    _segment = 2;
                    initIO(_afterIO, ioRead, true);
                    break;

                case stateALU: readEA(stateALU2); break;
                case stateALU2:
                    _wait = 5;
                    if (!_sourceIsRM) {
                        _destination = _data;
                        _source = getReg();
                        _wait += 3;
                    }
                    else {
                        _destination = getReg();
                        _source = _data;
                    }
                    if (!_useMemory)
                        _wait = 3;
                    _aluOperation = (_opcode >> 3) & 7;
                    _state = stateALU3;
                    break;
                case stateALU3:
                    doALUOperation();
                    if (_aluOperation != 7) {
                        if (!_sourceIsRM)
                            writeEA(_data, 0);
                        else {
                            setReg(_data);
                            _state = stateEndInstruction;
                        }
                    }
                    else
                        _state = stateEndInstruction;
                    break;

                case stateALUAccumImm:
                    fetch(stateALUAccumImm2, _wordSize);
                    break;
                case stateALUAccumImm2:
                    _destination = getAccum();
                    _source = _data;
                    _aluOperation = (_opcode >> 3) & 7;
                    doALUOperation();
                    if (_aluOperation != 7)
                        setAccum();
                    end(4);
                    break;

                case statePushSegReg:
                    push(_segmentRegisters[_opcode >> 3]);
                    break;

                case statePopSegReg: pop(statePopSegReg2); break;
                case statePopSegReg2:
                    _segmentRegisters[_opcode >> 3] = _data;
                    end(4);
                    break;

                case stateSegOverride:
                    _segmentOverride = (_opcode >> 3) & 3;
                    _state = stateBegin;
                    _wait = 2;
                    break;

                case stateDAA:
                    if (af() || (al() & 0x0f) > 9) {
                        _data = al() + 6;
                        al() = _data;
                        setAF(true);
                        if ((_data & 0x100) != 0)
                            setCF(true);
                    }
                    if (cf() || al() > 0x9f) {
                        al() += 0x60;
                        setCF(true);
                    }
                    _state = stateDA;
                    break;
                case stateDAS:
                    {
                        UInt8 t = al();
                        if (af() || ((al() & 0xf) > 9)) {
                            _data = al() - 6;
                            al() = _data;
                            setAF(true);
                            if ((_data & 0x100) != 0)
                                setCF(true);
                        }
                        if (cf() || t > 0x9f) {
                            al() -= 0x60;
                            setCF(true);
                        }
                    }
                    _state = stateDA;
                    break;
                case stateDA:
                    _wordSize = false;
                    _data = al();
                    setPZS();
                    end(4);
                    break;
                case stateAAA:
                    if (af() || ((al() & 0xf) > 9)) {
                        al() += 6;
                        ++ah();
                        setCA();
                    }
                    else
                        clearCA();
                    _state = stateAA;
                    break;
                case stateAAS:
                    if (af() || ((al() & 0xf) > 9)) {
                        al() -= 6;
                        --ah();
                        setCA();
                    }
                    else
                        clearCA();
                    _state = stateAA;
                    break;
                case stateAA:
                    al() &= 0x0f;
                    end(4);
                    break;

                case stateIncDecRW:
                    _destination = rw();
                    _source = 1;
                    _wordSize = true;
                    if ((_opcode & 8) == 0) {
                        _data = _destination + _source;
                        setOFAdd();
                    }
                    else {
                        _data = _destination - _source;
                        setOFSub();
                    }
                    doAF();
                    setPZS();
                    rw() = _data;
                    end(2);
                    break;
                case statePushRW:
                    sp() -= 2;
                    _data = rw();
                    _afterIO = stateEndInstruction;
                    _state = statePush2;
                    _wait += 7;
                    break;
                case statePopRW: pop(statePopRW2); break;
                case statePopRW2: rw() = _data; end(4); break;

                case stateInvalid: end(0); break;

                case stateJCond: fetch(stateJCond2, false); break;
                case stateJCond2:
                    {
                        bool jump;
                        switch (_opcode & 0x0e) {
                        case 0x00: jump =  of(); break;
                        case 0x02: jump =  cf(); break;
                        case 0x04: jump =  zf(); break;
                        case 0x06: jump =  cf() || zf(); break;
                        case 0x08: jump =  sf(); break;
                        case 0x0a: jump =  pf(); break;
                        case 0x0c: jump = (sf() != of()); break;
                        case 0x0e: jump = (sf() != of()) || zf(); break;
                        }
                        if ((_opcode & 1) != 0)
                            jump = !jump;
                        if (jump)
                            jumpShort();
                        end(jump ? 16 : 4);
                    }
                break;

                case stateALURMImm: readEA(stateALURMImm2); break;
                case stateALURMImm2:
                    _destination = _data;
                    fetch(stateALURMImm3, _opcode == 0x81);
                    break;
                case stateALURMImm3:
                    if (_opcode != 0x83)
                        _source = _data;
                    else
                        _source = signExtend(_data);
                    _aluOperation = modRMReg();
                    _wait = 9;
                    if (_aluOperation == 7)
                        _wait = 6;
                    if (!_useMemory)
                        _wait = 4;
                    doALUOperation();
                    if (_aluOperation != 7)
                        writeEA(_data, _wait);
                    else
                        _state = stateEndInstruction;
                    break;

                case stateTestRMReg: readEA(stateTestRMReg2); break;
                case stateTestRMReg2:
                    test(_data, getReg());
                    end(_useMemory ? 5 : 3);
                    break;
                case stateXchgRMReg: readEA(stateXchgRMReg2); break;
                case stateXchgRMReg2:
                    _source = getReg();
                    setReg(_data);
                    writeEA(_source, _useMemory ? 9 : 4);
                    break;
                case stateMovRMReg: loadEA(stateMovRMReg2); break;
                case stateMovRMReg2:
                    writeEA(getReg(), _useMemory ? 5 : 2);
                    break;
                case stateMovRegRM: readEA(stateMovRegRM2); break;
                case stateMovRegRM2:
                    setReg(_data);
                    end(_useMemory ? 4 : 2);
                    break;
                case stateMovRMWSegReg:
                    _wordSize = true;
                    loadEA(stateMovRMWSegReg2);
                    break;
                case stateMovRMWSegReg2:
                    writeEA(_segmentRegisters[modRMReg() & 3],
                        _useMemory ? 5 : 2);
                    break;
                case stateLEA: loadEA(stateLEA2); break;
                case stateLEA2: setReg(_address); end(2); break;
                case stateMovSegRegRMW:
                    _wordSize = true;
                    readEA(stateMovSegRegRMW2);
                    break;
                case stateMovSegRegRMW2:
                    _segmentRegisters[modRMReg() & 3] = _data;
                    end(_useMemory ? 4 : 2);
                    break;
                case statePopMW:
                    _afterIO = statePopMW2;
                    loadEA(statePop);
                    break;
                case statePopMW2: writeEA(_data, _useMemory ? 9 : 12); break;
                case stateXchgAxRW:
                    _data = rw();
                    rw() = ax();
                    ax() = _data;
                    end(3);
                    break;
                case stateCBW: ax() = signExtend(al()); end(2); break;
                case stateCWD:
                    dx() = ((ax() & 0x8000) == 0 ? 0x0000 : 0xffff);
                    end(5);
                    break;
                case stateCallCP: fetch(stateCallCP2, true); break;
                case stateCallCP2:
                    _savedIP = _data;
                    fetch(stateCallCP3, true);
                    break;
                case stateCallCP3:
                    _savedCS = _data;
                    _wait = 2;
                    push(cs(), stateCallCP4);
                    break;
                case stateCallCP4: push(_ip, stateJmpCP4); break;
                case stateWait: end(4); break;
                case statePushF: push((_flags & 0x0fd7) | 0xf000); break;
                case statePopF: pop(statePopF2); break;
                case statePopF2: _flags = _data | 2; end(4); break;
                case stateSAHF:
                    _flags = (_flags & 0xff02) | ah();
                    end(4);
                    break;
                case stateLAHF: ah() = _flags & 0xd7; end(4); break;

                case stateMovAccumInd: fetch(stateMovAccumInd2, true); break;
                case stateMovAccumInd2:
                    _address = _data;
                    initIO(stateMovAccumInd3, ioRead, _wordSize);
                    break;
                case stateMovAccumInd3: setAccum(); end(6); break;
                case stateMovIndAccum: fetch(stateMovIndAccum2, true); break;
                case stateMovIndAccum2:
                    _address = _data;
                    _data = getAccum();
                    _wait = 6;
                    initIO(stateEndInstruction, ioWrite, _wordSize);
                    break;

                case stateMovS:
                    _wait = (_rep != 0 ? 9 : 10);
                    if (repCheck())
                        lodS(stateMovS2);
                    break;
                case stateMovS2:
                    _afterRep = stateMovS;
                    stoS(stateRepAction);
                    break;
                case stateRepAction:
                    _state = stateEndInstruction;
                    if (_rep != 0 && cx() != 0) {
                        --cx();
                        _afterCheckInt = _afterRep;
                        if (cx() == 0)
                            _afterCheckInt = stateEndInstruction;
                        if ((_afterRep == stateCmpS || _afterRep == stateScaS)
                            && (zf() == (_rep == 1)))
                            _afterCheckInt = stateEndInstruction;
                        _state = stateCheckInt;
                    }
                    break;
                case stateCmpS:
                    _wait = 14;
                    if (repCheck())
                        lodS(stateCmpS2);
                    break;
                case stateCmpS2:
                    _destination = _data;
                    lodDIS(stateCmpS3);
                    break;
                case stateCmpS3:
                    _source = _data;
                    sub();
                    _afterRep = stateCmpS;
                    _state = stateRepAction;
                    break;

                case stateTestAccumImm:
                    fetch(stateTestAccumImm2, _wordSize);
                    break;
                case stateTestAccumImm2:
                    test(getAccum(), _data);
                    end(4);
                    break;

                case stateStoS:
                    _wait = (_rep != 0 ? 6 : 7);
                    if (repCheck()) {
                        _data = getAccum();
                        _afterRep = stateStoS;
                        stoS(stateRepAction);
                    }
                    break;
                case stateLodS:
                    _wait = (_rep != 0 ? 9 : 8);
                    if (repCheck())
                        lodS(stateLodS2);
                    break;
                case stateLodS2:
                    setAccum();
                    _afterRep = stateLodS;
                    _state = stateRepAction;
                    break;
                case stateScaS:
                    _wait = 11;
                    if (repCheck())
                        lodDIS(stateScaS2);
                    break;
                case stateScaS2:
                    _destination = getAccum();
                    _source = _data;
                    sub();
                    _afterRep = stateScaS;
                    _state = stateRepAction;
                    break;

                case stateMovRegImm:
                    _wordSize = ((_opcode & 8) != 0);
                    fetch(stateMovRegImm2, _wordSize);
                    break;
                case stateMovRegImm2:
                    if (_wordSize)
                        rw() = _data;
                    else
                        rb() = _data;
                    end(4);
                    break;

                case stateRet: pop(stateRet2); break;
                case stateRet2:
                    _savedIP = _data;
                    if ((_opcode & 8) == 0) {
                        _savedCS = _segmentRegisters[1];
                        _state = stateRet4;
                        _wait = (!_wordSize ? 16 : 12);
                    }
                    else {
                        pop(stateRet3);
                        _wait = (!_wordSize ? 17 : 18);
                    }
                    break;
                case stateRet3:
                    _savedCS = _data;
                    _state = stateRet4;
                    break;
                case stateRet4:
                    if (!_wordSize)
                        fetch(stateRet5, true);
                    else
                        _state = stateRet6;
                    break;
                case stateRet5:
                    sp() += _data;
                    _state = stateRet6;
                    break;
                case stateRet6:
                    _segmentRegisters[1] = _savedCS;
                    setIP(_savedIP);
                    end(0);
                    break;

                case stateLoadFar: readEA(stateLoadFar2); break;
                case stateLoadFar2:
                    setReg(_data);
                    _afterIO = stateLoadFar3;
                    _state = stateLoadFar3;
                    break;
                case stateLoadFar3:
                    _segmentRegisters[!_wordSize ? 0 : 3] = _data;
                    end(8);
                    break;

                case stateMovRMImm: loadEA(stateMovRMImm2); break;
                case stateMovRMImm2: fetch(stateMovRMImm3, _wordSize); break;
                case stateMovRMImm3: writeEA(_data, _useMemory ? 6 : 4); break;

                case stateCheckInt:
                    _state = _afterCheckInt;
                    if (_nmiRequested) {
                        _nmiRequested = false;
                        interrupt(2);
                    }
                    if (intf() && _interruptRequested) {
                        initIO(stateHardwareInt, ioInterruptAcknowledge,
                            false);
                        _interruptRequested = false;
                        _wait = 1;
                    }
                    break;
                case stateHardwareInt:
                    _wait = 1;
                    _state = stateHardwareInt2;
                    break;
                case stateHardwareInt2:
                    initIO(stateIntAction, ioInterruptAcknowledge, false);
                    break;
                case stateInt3:
                    interrupt(3);
                    _wait = 1;
                    break;
                case stateInt:
                    fetch(stateIntAction, false);
                    break;
                case stateIntAction:
                    _source = _data;
                    push(_flags & 0x0fd7, stateIntAction2);
                    break;
                case stateIntAction2:
                    setIF(false);
                    setTF(false);
                    push(cs(), stateIntAction3);
                    break;
                case stateIntAction3: push(_ip, stateIntAction4); break;
                case stateIntAction4:
                    _address = _source << 2;
                    _segment = 1;
                    cs() = 0;
                    initIO(stateIntAction5, ioRead, true);
                    break;
                case stateIntAction5:
                    _savedIP = _data;
                    _address++;
                    initIO(stateIntAction6, ioRead, true);
                    break;
                case stateIntAction6:
                    cs() = _data;
                    setIP(_savedIP);
                    _wait = 13;
                    _state = stateEndInstruction;
                    break;
                case stateIntO:
                    if (of()) {
                        interrupt(4);
                        _wait = 2;
                    }
                    else
                        end(4);
                    break;
                case stateIRet: pop(stateIRet2); break;
                case stateIRet2: _savedIP = _data; pop(stateIRet3); break;
                case stateIRet3: _savedCS = _data; pop(stateIRet4); break;
                case stateIRet4:
                    _flags = _data | 2;
                    cs() = _savedCS;
                    setIP(_savedIP);
                    end(20);
                    break;

                case stateShift: readEA(stateShift2); break;
                case stateShift2:
                    if ((_opcode & 2) == 0) {
                        _source = 1;
                        _wait = (_useMemory ? 7 : 2);
                    }
                    else {
                        _source = cl();
                        _wait = (_useMemory ? 12 : 8);
                    }
                    _state = stateShift3;
                    break;
                case stateShift3:
                    if (_source == 0) {
                        writeEA(_data, 0);
                        break;
                    }
                    _destination = _data;
                    switch (modRMReg()) {
                    case 0:  // ROL
                        _data <<= 1;
                        doCF();
                        _data |= (cf() ? 1 : 0);
                        setOFRotate();
                        break;
                    case 1:  // ROR
                        setCF((_data & 1) != 0);
                        _data >>= 1;
                        if (cf())
                            _data |= (!_wordSize ? 0x80 : 0x8000);
                        setOFRotate();
                        break;
                    case 2:  // RCL
                        _data = (_data << 1) | (cf() ? 1 : 0);
                        doCF();
                        setOFRotate();
                        break;
                    case 3:  // RCR
                        _data >>= 1;
                        if (cf())
                            _data |= (!_wordSize ? 0x80 : 0x8000);
                        setCF((_destination & 1) != 0);
                        setOFRotate();
                        break;
                    case 4:  // SHL
                    case 6:
                        _data <<= 1;
                        doCF();
                        setOFRotate();
                        setPZS();
                        break;
                    case 5:  // SHR
                        setCF((_data & 1) != 0);
                        _data >>= 1;
                        setOFRotate();
                        setAF(true);
                        setPZS();
                        break;
                    case 7:  // SAR
                        setCF((_data & 1) != 0);
                        _data >>= 1;
                        if (!_wordSize)
                            _data |= (_destination & 0x80);
                        else
                            _data |= (_destination & 0x8000);
                        setOFRotate();
                        setAF(true);
                        setPZS();
                        break;
                    }
                    if ((_opcode & 2) != 0)
                        _wait = 4;
                    --_source;
                    break;

                case stateAAM: fetch(stateAAM2, false); break;
                case stateAAM2:
                    if (_data == 0)
                        interrupt(0);
                    else {
                        ah() = al() / _data;
                        al() %= _data;
                        _wordSize = true;
                        setPZS();
                        end(83);
                    }
                    break;
                case stateAAD: fetch(stateAAD2, false); break;
                case stateAAD2:
                    al() += ah()*_data;
                    ah() = 0;
                    setPZS();
                    end(60);
                    break;

                case stateSALC: al() = (cf() ? 0xff : 0x00); end(4); break;

                case stateXlatB:
                    _address = bx() + al();
                    initIO(stateXlatB2, ioRead, false);
                    break;
                case stateXlatB2:
                    al() = _data;
                    end(7);
                    break;

                case stateEscape: loadEA(stateEscape2); break;
                case stateEscape2: end(2); break;

                case stateLoop: fetch(stateLoop2, false); break;
                case stateLoop2:
                {
                    bool jump;
                    if (_opcode != 0xe3) {
                        _wait = 5;
                        --cx();
                        jump = (cx() != 0);
                        switch (_opcode) {
                        case 0xe0:
                            if (zf())
                                jump = false;
                            _wait = 6;
                            break;
                        case 0xe1: if (!zf()) jump = false; break;
                        }
                    }
                    else {
                        _wait = 6;
                        jump = (cx() == 0);
                    }
                    if (jump) {
                        jumpShort();
                        _wait = _opcode == 0xe0 ? 19 :
                            _opcode == 0xe2 ? 17 : 18;
                    }
                }
                end(_wait);
                break;

                case stateInOut:
                    if ((_opcode & 8) == 0) {
                        fetch(stateInOut2, false);
                        _wait = 2;
                    }
                    else {
                        _data = dx();
                        _state = stateInOut2;
                    }
                    break;
                case stateInOut2:
                    _usePortSpace = true;
                    _ioType = ((_opcode & 2) == 0 ? ioRead : ioWrite);
                    _segment = 7;
                    _address = _data;
                    if (_ioType == ioWrite)
                        _data = getAccum();
                    initIO(stateInOut3, _ioType, _wordSize);
                    break;
                case stateInOut3:
                    if (_ioType == ioRead)
                        setAccum();
                    end(4);
                    break;

                case stateCallCW: fetch(stateCallCW2, true); break;
                case stateCallCW2:
                    _savedIP = _data + _ip;
                    _wait = 3;
                    _state = stateCallCW3;
                    break;
                case stateCallCW3: push(_ip, stateJmpCW3); break;

                case stateJmpCW: fetch(stateJmpCW2, true); break;
                case stateJmpCW2:
                    _savedIP = _data + _ip;
                    _wait = 9;
                    _state = stateJmpCW3;
                    break;
                case stateJmpCW3: setIP(_savedIP); end(6); break;

                case stateJmpCP: fetch(stateJmpCP2, true); break;
                case stateJmpCP2:
                    _savedIP = _data;
                    fetch(stateJmpCP3, true);
                    break;
                case stateJmpCP3:
                    _savedCS = _data;
                    _wait = 9;
                    _state = stateJmpCP4;
                    break;
                case stateJmpCP4: cs() = _savedCS; _state = stateJmpCW3; break;

                case stateJmpCB: fetch(stateJmpCB2, false); break;
                case stateJmpCB2: jumpShort(); end(15); break;

                case stateLock: end(2); break;
                case stateRep:
                    _rep = (_opcode == 0xf2 ? 1 : 2);
                    _wait = 9;
                    _state = stateBegin;
                    break;
                case stateHlt:
                    _wait = 1;
                    _cpu_s = 3;
                    _state = stateHalted;
                    break;
                case stateHalted:
                    _wait = 1;
                    _state = stateCheckInt;
                    _afterCheckInt = stateHalted;
                    break;
                case stateCmC: _flags ^= 1; end(2); break;

                case stateMath: readEA(stateMath2); break;
                case stateMath2:
                    if ((modRMReg() & 6) == 0) {
                        _destination = _data;
                        fetch(stateMath3, _wordSize);
                    }
                    else
                        _state = stateMath3;
                    break;
                case stateMath3:
                    switch (modRMReg()) {
                    case 0:
                    case 1:  // TEST
                        test(_destination, _data);
                        end(_useMemory ? 7 : 5);
                        break;
                    case 2:  // NOT
                        writeEA(~_data, _useMemory ? 8 : 3);
                        break;
                    case 3:  // NEG
                        _source = _data;
                        _destination = 0;
                        sub();
                        writeEA(_data, _useMemory ? 8 : 3);
                        break;
                    case 4:  // MUL
                    case 5:  // IMUL
                        _source = _data;
                        _destination = getAccum();
                        _data = _destination;
                        setSF();
                        setPF();
                        _data *= _source;
                        ax() = _data;
                        if (!_wordSize)
                            if (modRMReg() == 4) {
                                setCF(ah() != 0);
                                _wait = 70;
                            }
                            else {
                                if ((_source & 0x80) != 0)
                                    ah() -= _destination;
                                if ((_destination & 0x80) != 0)
                                    ah() -= _source;
                                setCF(ah() ==
                                    ((al() & 0x80) == 0 ? 0 : 0xff));
                                _wait = 80;
                            }
                        else
                            if (modRMReg() == 4) {
                                dx() = _data >> 16;
                                _data |= dx();
                                setCF(dx() != 0);
                                _wait = 118;
                            }
                            else {
                                dx() = _data >> 16;
                                if ((_source & 0x8000) != 0)
                                    dx() -= _destination;
                                if ((_destination & 0x8000) != 0)
                                    dx() -= _source;
                                _data |= dx();
                                setCF(dx() ==
                                    ((ax() & 0x8000) == 0 ? 0 : 0xffff));
                                _wait = 128;
                            }
                            setZF();
                            setOF(cf());
                            if (_useMemory)
                                _wait += 2;
                            _state = stateEndInstruction;
                            break;
                    case 6:  // DIV
                    case 7:  // IDIV
                        _source = _data;
                        if (_source == 0) {
                            interrupt(0);
                            break;
                        }
                        if (!_wordSize) {
                            _destination = ax();
                            if (modRMReg() == 6) {
                                div();
                                if (_data > 0xff) {
                                    interrupt(0);
                                    break;
                                }
                                _wait = 80;
                            }
                            else {
                                _destination = ax();
                                if ((_destination & 0x8000) != 0)
                                    _destination |= 0xffff0000;
                                _source = signExtend(_source);
                                div();
                                if (_data > 0x7f && _data < 0xffffff80) {
                                    interrupt(0);
                                    break;
                                }
                                _wait = 101;
                            }
                            ah() = _remainder;
                            al() = _data;
                        }
                        else {
                            _destination = (dx() << 16) + ax();
                            div();
                            if (modRMReg() == 6) {
                                if (_data > 0xffff) {
                                    interrupt(0);
                                    break;
                                }
                                _wait = 144;
                            }
                            else {
                                if (_data > 0x7fff && _data <  0xffff8000) {
                                    interrupt(0);
                                    break;
                                }
                                _wait = 165;
                            }
                            dx() = _remainder;
                            ax() = _data;
                        }
                        if (_useMemory)
                            _wait += 2;
                        _state = stateEndInstruction;
                        break;
                    }
                    break;

                case stateLoadC: setCF(_wordSize); end(2); break;
                case stateLoadI: setIF(_wordSize); end(2); break;
                case stateLoadD: setDF(_wordSize); end(2); break;

                case stateMisc: readEA(stateMisc2); break;
                case stateMisc2:
                    _savedIP = _data;
                    switch (modRMReg()) {
                        case 0:
                        case 1:
                            _destination = _data;
                            _source = 1;
                            if (modRMReg() == 0) {
                                _data = _destination + _source;
                                setOFAdd();
                            }
                            else {
                                _data = _destination - _source;
                                setOFSub();
                            }
                            doAF();
                            setPZS();
                            writeEA(_data, _useMemory ? 7 : 3);
                            break;
                        case 2:
                            _wait = (_useMemory ? 1 : 0);
                            _state = stateCallCW3;
                            break;
                        case 3:
                            if (_useMemory) {
                                _address += 2;
                                _wait = 1;
                                readEA(stateCallCP3);
                            }
                            else
                                end(0);
                            break;
                        case 4:
                            _wait = (_useMemory ? 8 : 5);
                            _state = stateJmpCW3;
                            break;
                        case 5:
                            if (_useMemory) {
                                _address += 2;
                                _wait = 1;
                                readEA(stateJmpCP3);
                            }
                            else
                                end(0);
                            break;
                        case 6:
                        case 7:
                            push(_data, stateEndInstruction);
                            _wait += (_useMemory ? 2 : 1);
                            break;
                    }
                    break;
            }
        } while (true);
    }

    bool _logging;
    Test _test;
    String _log;
    int _cycles;
    Array<Byte> _ram;
    int _stopIP;

    enum IOType
    {
        ioNone,
        ioRead,
        ioWrite,
        ioInstructionFetch,
        ioInterruptAcknowledge
    };
    enum State
    {
        stateWaitingForBIU,
        stateBegin, stateDecodeOpcode,
        stateEndInstruction,
        stateBeginModRM, stateDecodeModRM,
        stateEAOffset,
        stateEARegisters,
        stateEAByte,
        stateEAWord,
        stateEASetSegment,
        stateEAIO,
        statePush, statePush2,
        statePop,

        stateALU, stateALU2, stateALU3,
        stateALUAccumImm, stateALUAccumImm2,
        statePushSegReg,
        statePopSegReg, statePopSegReg2,
        stateSegOverride,
        stateDAA, stateDAS, stateDA, stateAAA, stateAAS, stateAA,
        stateIncDecRW, statePushRW, statePopRW, statePopRW2,
        stateInvalid,
        stateJCond, stateJCond2,
        stateALURMImm, stateALURMImm2, stateALURMImm3,
        stateTestRMReg, stateTestRMReg2,
        stateXchgRMReg, stateXchgRMReg2,
        stateMovRMReg, stateMovRMReg2,
        stateMovRegRM, stateMovRegRM2,
        stateMovRMWSegReg, stateMovRMWSegReg2,
        stateLEA, stateLEA2,
        stateMovSegRegRMW, stateMovSegRegRMW2,
        statePopMW, statePopMW2,
        stateXchgAxRW,
        stateCBW, stateCWD,
        stateCallCP, stateCallCP2, stateCallCP3, stateCallCP4, stateCallCP5,
        stateWait,
        statePushF, statePopF, statePopF2,
        stateSAHF, stateLAHF,
        stateMovAccumInd, stateMovAccumInd2, stateMovAccumInd3,
        stateMovIndAccum, stateMovIndAccum2,
        stateMovS, stateMovS2, stateRepAction,
        stateCmpS, stateCmpS2, stateCmpS3,
        stateTestAccumImm, stateTestAccumImm2,
        stateStoS,
        stateLodS, stateLodS2,
        stateScaS, stateScaS2,
        stateMovRegImm, stateMovRegImm2,
        stateRet, stateRet2, stateRet3, stateRet4, stateRet5, stateRet6,
        stateLoadFar, stateLoadFar2, stateLoadFar3,
        stateMovRMImm, stateMovRMImm2, stateMovRMImm3,
        stateCheckInt, stateHardwareInt, stateHardwareInt2,
        stateInt, stateInt3, stateIntAction, stateIntAction2, stateIntAction3,
        stateIntAction4, stateIntAction5, stateIntAction6,
        stateIntO,
        stateIRet, stateIRet2, stateIRet3, stateIRet4,
        stateShift, stateShift2, stateShift3,
        stateAAM, stateAAM2,
        stateAAD, stateAAD2,
        stateSALC,
        stateXlatB, stateXlatB2,
        stateEscape, stateEscape2,
        stateLoop, stateLoop2,
        stateInOut, stateInOut2, stateInOut3,
        stateCallCW, stateCallCW2, stateCallCW3, stateCallCW4,
        stateJmpCW, stateJmpCW2, stateJmpCW3,
        stateJmpCP, stateJmpCP2, stateJmpCP3, stateJmpCP4,
        stateJmpCB, stateJmpCB2,
        stateLock, stateRep, stateHlt, stateHalted, stateCmC,
        stateMath, stateMath2, stateMath3,
        stateLoadC, stateLoadI, stateLoadD,
        stateMisc, stateMisc2
    };
    enum BusState
    {
        t1,
        t2,
        t3,
        tWait,
        t4,
        tIdle
    };
    enum IOByte
    {
        ioSingleByte,
        ioWordFirst,
        ioWordSecond
    };
    void div()
    {
        bool negative = false;
        bool dividendNegative = false;
        if (modRMReg() == 7) {
            if ((_destination & 0x80000000) != 0) {
                _destination =
                    static_cast<UInt32>(-static_cast<SInt32>(_destination));
                negative = !negative;
                dividendNegative = true;
            }
            if ((_source & 0x8000) != 0) {
                _source = (static_cast<UInt32>(-static_cast<SInt32>(_source)))
                    & 0xffff;
                negative = !negative;
            }
        }
        _data = _destination / _source;
        UInt32 product = _data * _source;
        // ISO C++ 2003 does not specify a rounding mode, but the x86 always
        // rounds towards zero.
        if (product > _destination) {
            --_data;
            product -= _source;
        }
        _remainder = _destination - product;
        if (negative)
            _data = static_cast<UInt32>(-static_cast<SInt32>(_data));
        if (dividendNegative)
            _remainder = static_cast<UInt32>(-static_cast<SInt32>(_remainder));
    }
    void jumpShort() { setIP(_ip + signExtend(_data)); }
    void interrupt(UInt8 number)
    {
        _data = number;
        _state = stateIntAction;
    }
    void test(UInt16 destination, UInt16 source)
    {
        _destination = destination;
        _source = source;
        bitwise(_destination & _source);
    }
    int stringIncrement()
    {
        int r = (_wordSize ? 2 : 1);
        return !df() ? r : -r;
    }
    void lodS(State state)
    {
        _address = si();
        si() += stringIncrement();
        _segment = 3;
        initIO(state, ioRead, _wordSize);
    }
    void lodDIS(State state)
    {
        _address = di();
        di() += stringIncrement();
        _segment = 0;
        initIO(state, ioRead, _wordSize);
    }
    void stoS(State state)
    {
        _address = di();
        di() += stringIncrement();
        _segment = 0;
        initIO(state, ioWrite, _wordSize);
    }
    bool repCheck()
    {
        if (_rep != 0 && cx() == 0) {
            _state = stateRepAction;
            return false;
        }
        return true;
    }
    void end(int wait) { _wait = wait; _state = stateEndInstruction; }
    void push(UInt16 data, State state = stateEndInstruction)
    {
        _data = data;
        _afterIO = state;
        _state = statePush;
        _wait += 6;
    }
    void pop(State state) { _afterIO = state; _state = statePop; }
    void loadEA(State state) { _afterEA = state; _state = stateBeginModRM; }
    void readEA(State state)
    {
        _afterEAIO = state;
        _ioType = ioRead;
        loadEA(stateEAIO);
    }
    void fetch(State state, bool wordSize)
    {
        initIO(state, ioInstructionFetch, wordSize);
    }
    void writeEA(UInt16 data, int wait)
    {
        _data = data;
        _wait = wait;
        _afterEAIO = stateEndInstruction;
        _ioType = ioWrite;
        _state = stateEAIO;
    }
    void setCA() { setCF(true); setAF(true); }
    void clearCA() { setCF(false); setAF(false); }
    void clearCAO() { clearCA(); setOF(false); }
    void setPZS() { setPF(); setZF(); setSF(); }
    void bitwise(UInt16 data) { _data = data; clearCAO(); setPZS(); }
    void doAF() { setAF(((_data ^ _source ^ _destination) & 0x10) != 0); }
    void doCF() { setCF((_data & (!_wordSize ? 0x100 : 0x10000)) != 0); }
    void setCAPZS() { setPZS(); doAF(); doCF(); }
    void setOFAdd()
    {
        UInt16 t = (_data ^ _source) & (_data ^ _destination);
        setOF((t & (!_wordSize ? 0x80 : 0x8000)) != 0);
    }
    void add() { _data = _destination + _source; setCAPZS(); setOFAdd(); }
    void setOFSub()
    {
        UInt16 t = (_destination ^ _source) & (_data ^ _destination);
        setOF((t & (!_wordSize ? 0x80 : 0x8000)) != 0);
    }
    void sub() { _data = _destination - _source; setCAPZS(); setOFSub(); }
    void setOFRotate()
    {
        setOF(((_data ^ _destination) & (!_wordSize ? 0x80 : 0x8000)) != 0);
    }

    void doALUOperation()
    {
        switch (_aluOperation) {
            case 0: add(); break;
            case 1: bitwise(_destination | _source); break;
            case 2: _source += cf() ? 1 : 0; add(); break;
            case 3: _source += cf() ? 1 : 0; sub(); break;
            case 4: test(_destination, _source); break;
            case 5:
            case 7: sub(); break;
            case 6: bitwise(_destination ^ _source); break;
        }
    }
    UInt16 signExtend(UInt8 data) { return data + (data < 0x80 ? 0 : 0xff00); }
    template<class U> class Register
    {
    public:
        void init(String name, U* data)
        {
            _name = name;
            _data = data;
            _read = false;
            _written = false;
        }
        const U& operator=(U value)
        {
            *_data = value;
            _writtenValue = value;
            _written = true;
            return *_data;
        }
        operator U()
        {
            if (!_read && !_written) {
                _read = true;
                _readValue = *_data;
            }
            return *_data;
        }
        const U& operator+=(U value)
        {
            return operator=(operator U() + value);
        }
        const U& operator-=(U value)
        {
            return operator=(operator U() - value);
        }
        const U& operator%=(U value)
        {
            return operator=(operator U() % value);
        }
        const U& operator&=(U value)
        {
            return operator=(operator U() & value);
        }
        const U& operator^=(U value)
        {
            return operator=(operator U() ^ value);
        }
        U operator-() const { return -operator U(); }
        void operator++() { operator=(operator U() + 1); }
        void operator--() { operator=(operator U() - 1); }
        String text()
        {
            String text;
            if (_read) {
                text = hex(_readValue, sizeof(U)==1 ? 2 : 4, false) + "<-" +
                    _name + "  ";
                _read = false;
            }
            if (_written) {
                text += _name + "<-" +
                    hex(_writtenValue, sizeof(U)==1 ? 2 : 4, false) + "  ";
                _written = false;
            }
            return text;
        }
    protected:
        String _name;
        U* _data;
        bool _read;
        bool _written;
        U _readValue;
        U _writtenValue;
    };
    class FlagsRegister : public Register<UInt16>
    {
    public:
        UInt32 operator=(UInt32 value)
        {
            Register<UInt16>::operator=(value);
            return Register<UInt16>::operator UInt16();
        }
        String text()
        {
            String text;
            if (this->_read) {
                text = flags(this->_readValue) + "<-" + this->_name + "  ";
                this->_read = false;
            }
            if (this->_written) {
                text += this->_name + "<-" + flags(this->_writtenValue) + "  ";
                this->_written = false;
            }
            return text;
        }
    private:
        String flags(UInt16 value) const
        {
            return String(value & 0x800 ? "O" : "p") +
                String(value & 0x400 ? "D" : "d") +
                String(value & 0x200 ? "I" : "i") +
                String(value & 0x100 ? "T" : "t") +
                String(value & 0x80 ? "S" : "s") +
                String(value & 0x40 ? "Z" : "z") +
                String(value & 0x10 ? "A" : "a") +
                String(value & 4 ? "P" : "p") +
                String(value & 1 ? "C" : "c");
        }
    };

    Register<UInt16>& rw() { return _wordRegisters[_opcode & 7]; }
    Register<UInt16>& ax() { return _wordRegisters[0]; }
    Register<UInt16>& cx() { return _wordRegisters[1]; }
    Register<UInt16>& dx() { return _wordRegisters[2]; }
    Register<UInt16>& bx() { return _wordRegisters[3]; }
    Register<UInt16>& sp() { return _wordRegisters[4]; }
    Register<UInt16>& bp() { return _wordRegisters[5]; }
    Register<UInt16>& si() { return _wordRegisters[6]; }
    Register<UInt16>& di() { return _wordRegisters[7]; }
    Register<UInt8>& rb() { return _byteRegisters[_opcode & 7]; }
    Register<UInt8>& al() { return _byteRegisters[0]; }
    Register<UInt8>& cl() { return _byteRegisters[1]; }
    Register<UInt8>& ah() { return _byteRegisters[4]; }
    Register<UInt16>& es() { return _segmentRegisters[0]; }
    Register<UInt16>& cs() { return _segmentRegisters[1]; }
    Register<UInt16>& ss() { return _segmentRegisters[2]; }
    Register<UInt16>& ds() { return _segmentRegisters[3]; }
    UInt16& csQuiet() { return _segmentRegisterData[1]; }
    bool cf() { return (_flags & 1) != 0; }
    void setCF(bool cf) { _flags = (_flags & ~1) | (cf ? 1 : 0); }
    bool pf() { return (_flags & 4) != 0; }
    void setPF()
    {
        static UInt8 table[0x100] = {
            4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
            0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
            0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
            4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
            0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
            4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
            4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
            0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
            0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
            4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
            4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
            0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
            4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
            0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
            0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
            4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4};
        _flags = (_flags & ~4) | table[_data & 0xff];
    }
    bool af() { return (_flags & 0x10) != 0; }
    void setAF(bool af) { _flags = (_flags & ~0x10) | (af ? 0x10 : 0); }
    bool zf() { return (_flags & 0x40) != 0; }
    void setZF()
    {
        _flags = (_flags & ~0x40) |
            ((_data & (!_wordSize ? 0xff : 0xffff)) == 0 ? 0x40 : 0);
    }
    bool sf() { return (_flags & 0x80) != 0; }
    void setSF()
    {
        _flags = (_flags & ~0x80) |
            ((_data & (!_wordSize ? 0x80 : 0x8000)) != 0 ? 0x80 : 0);
    }
    bool tf() { return (_flags & 0x100) != 0; }
    void setTF(bool tf) { _flags = (_flags & ~0x100) | (tf ? 0x100 : 0); }
    bool intf() { return (_flags & 0x200) != 0; }
    void setIF(bool intf) { _flags = (_flags & ~0x200) | (intf ? 0x200 : 0); }
    bool df() { return (_flags & 0x400) != 0; }
    void setDF(bool df) { _flags = (_flags & ~0x400) | (df ? 0x400 : 0); }
    bool of() { return (_flags & 0x800) != 0; }
    void setOF(bool of) { _flags = (_flags & ~0x800) | (of ? 0x800 : 0); }
    int modRMReg() { return (_modRM >> 3) & 7; }
    Register<UInt16>& modRMRW() { return _wordRegisters[modRMReg()]; }
    Register<UInt8>& modRMRB() { return _byteRegisters[modRMReg()]; }
    UInt16 getReg()
    {
        if (!_wordSize)
            return static_cast<UInt8>(modRMRB());
        return modRMRW();
    }
    UInt16 getAccum()
    {
        if (!_wordSize)
            return static_cast<UInt8>(al());
        return ax();
    }
    void setAccum() { if (!_wordSize) al() = _data; else ax() = _data; }
    void setReg(UInt16 value)
    {
        if (!_wordSize)
            modRMRB() = static_cast<UInt8>(value);
        else
            modRMRW() = value;
    }
    void initIO(State nextState, IOType ioType, bool wordSize)
    {
        _state = stateWaitingForBIU;
        _afterIO = nextState;
        _ioRequested = ioType;
        _byte = (!wordSize ? ioSingleByte : ioWordFirst);
        if (ioType == ioInstructionFetch)
            completeInstructionFetch();
    }
    UInt16 getIP() { return _ip; }
    void setIP(UInt16 value)
    {
        _ip = value;
        _abandonFetch = true;
        _prefetched = 0;
        _cpu_qs = 2;
        _prefetchAddress = _ip;
    }
    UInt32 physicalAddress(UInt16 segment, UInt16 offset)
    {
        return ((_segmentRegisterData[segment] << 4) + offset) & 0xfffff;
    }
    UInt8 getInstructionByte()
    {
        UInt8 byte = _prefetchQueue[_prefetchOffset & 3];
        _prefetchOffset = (_prefetchOffset + 1) & 3;
        --_prefetched;
        _cpu_qs = 3;
        return byte;
    }
    void completeInstructionFetch()
    {
        if (_ioRequested != ioInstructionFetch)
            return;
        if (_byte == ioSingleByte) {
            if (_prefetched > 0) {
                _ioRequested = ioNone;
                _data = getInstructionByte();
                _state = _afterIO;
                ++_ip;
            }
        }
        else {
            if (_prefetched > 1) {
                _data = getInstructionByte();
                _data |= static_cast<UInt16>(getInstructionByte()) << 8;
                _ioRequested = ioNone;
                _state = _afterIO;
                _ip += 2;
            }
        }
    }
    UInt32 codeAddress(UInt16 offset) { return physicalAddress(1, offset); }

    Register<UInt16> _wordRegisters[8];
    Register<UInt8> _byteRegisters[8];
    Register<UInt16> _segmentRegisters[8];
    UInt16 _registerData[8];      /* AX CX DX BX SP BP SI DI */
    UInt16 _segmentRegisterData[8];
    UInt16 _flagsData;
    //   0: CF = unsigned overflow?
    //   1:  1
    //   2: PF = parity: even number of 1 bits in result?
    //   3:  0
    //   4: AF = unsigned overflow for low nybble
    //   5:  0
    //   6: ZF = zero result?
    //   7: SF = result is negative?
    //   8: TF = interrupt after every instruction?
    //   9: IF = interrupts enabled?
    //  10: DF = SI/DI decrement in string operations
    //  11: OF = signed overflow?
    FlagsRegister _flags;
    UInt16 _ip;
    UInt8 _prefetchQueue[4];
    UInt8 _prefetchOffset;
    UInt8 _prefetched;
    int _segment;
    int _segmentOverride;
    int _busSegment;
    UInt16 _prefetchAddress;
    IOType _ioType;
    IOType _ioRequested;
    IOType _ioInProgress;
    BusState _busState;
    IOByte _byte;
    bool _abandonFetch;
    int _wait;
    State _state;
    UInt8 _opcode;
    UInt8 _modRM;
    UInt32 _data;
    UInt32 _source;
    UInt32 _destination;
    UInt32 _remainder;
    UInt16 _address;
    bool _useMemory;
    bool _wordSize;
    int _aluOperation;
    State _afterEA;
    State _afterIO;
    State _afterEAIO;
    State _afterRep;
    State _afterCheckInt;
    bool _sourceIsRM;
    UInt16 _savedCS;
    UInt16 _savedIP;
    int _rep;
    bool _usePortSpace;
    bool _halted;
    bool _newInstruction;
    UInt16 _newIP;
    int _cycle;
    int _stopAtCycle;
    UInt32 _busAddress;
    UInt8 _busData;
    bool _nmiRequested;
    bool _interruptRequested;
    bool _ready;
    Array<bool> _visited;

    Disassembler _disassembler;
    // These represent the CPU and ISA bus pins used to create the sniffer
    // logs.
    UInt32 _cpu_ad;
    // A19/S6        O ADDRESS/STATUS: During T1, these are the four most significant address lines for memory operations. During I/O operations, these lines are LOW. During memory and I/O operations, status information is available on these lines during T2, T3, Tw, and T4. S6 is always low.
    // A18/S5        O The status of the interrupt enable flag bit (S5) is updated at the beginning of each clock cycle.
    // A17/S4        O  S4*2+S3 0 = Alternate Data, 1 = Stack, 2 = Code or None, 3 = Data
    // A16/S3        O
    // A15..A8       O ADDRESS BUS: These lines provide address bits 8 through 15 for the entire bus cycle (T1±T4). These lines do not have to be latched by ALE to remain valid. A15±A8 are active HIGH and float to 3-state OFF during interrupt acknowledge and local bus ``hold acknowledge''.
    // AD7..AD0     IO ADDRESS DATA BUS: These lines constitute the time multiplexed memory/IO address (T1) and data (T2, T3, Tw, T4) bus. These lines are active HIGH and float to 3-state OFF during interrupt acknowledge and local bus ``hold acknowledge''.
    UInt8 _cpu_qs;
    // QS0           O QUEUE STATUS: provide status to allow external tracking of the internal 8088 instruction queue. The queue status is valid during the CLK cycle after which the queue operation is performed.
    // QS1           0 = No operation, 1 = First Byte of Opcode from Queue, 2 = Empty the Queue, 3 = Subsequent Byte from Queue
    UInt8 _cpu_s;
    // -S0           O STATUS: is active during clock high of T4, T1, and T2, and is returned to the passive state (1,1,1) during T3 or during Tw when READY is HIGH. This status is used by the 8288 bus controller to generate all memory and I/O access control signals. Any change by S2, S1, or S0 during T4 is used to indicate the beginning of a bus cycle, and the return to the passive state in T3 and Tw is used to indicate the end of a bus cycle. These signals float to 3-state OFF during ``hold acknowledge''. During the first clock cycle after RESET becomes active, these signals are active HIGH. After this first clock, they float to 3-state OFF.
    // -S1           0 = Interrupt Acknowledge, 1 = Read I/O Port, 2 = Write I/O Port, 3 = Halt, 4 = Code Access, 5 = Read Memory, 6 = Write Memory, 7 = Passive
    // -S2
    bool _cpu_rqgt0;    // -RQ/-GT0 !87 IO REQUEST/GRANT: pins are used by other local bus masters to force the processor to release the local bus at the end of the processor's current bus cycle. Each pin is bidirectional with RQ/GT0 having higher priority than RQ/GT1. RQ/GT has an internal pull-up resistor, so may be left unconnected.
    bool _cpu_ready;    // READY        I  READY: is the acknowledgement from the addressed memory or I/O device that it will complete the data transfer. The RDY signal from memory or I/O is synchronized by the 8284 clock generator to form READY. This signal is active HIGH. The 8088 READY input is not synchronized. Correct operation is not guaranteed if the set up and hold times are not met.
    bool _cpu_test;     // -TEST        I  TEST: input is examined by the ``wait for test'' instruction. If the TEST input is LOW, execution continues, otherwise the processor waits in an ``idle'' state. This input is synchronized internally during each clock cycle on the leading edge of CLK.
    UInt32 _bus_address;
    // +A19..+A0      O Address bits: These lines are used to address memory and I/O devices within the system. These lines are generated by either the processor or DMA controller.
    UInt8 _bus_data;
    // +D7..+D0      IO Data bits: These lines provide data bus bits 0 to 7 for the processor, memory, and I/O devices.
    UInt8 _bus_dma;
    // +DRQ0 JP6/1 == U28.19 == U73.9
    // +DRQ1..+DRQ3  I  DMA Request: These lines are asynchronous channel requests used by peripheral devices to gain DMA service. They are prioritized with DRQ3 being the lowest and DRQl being the highest. A request is generated by bringing a DRQ line to an active level (high). A DRQ line must be held high until the corresponding DACK line goes active.
    // -DACK0..-DACK3 O -DMA Acknowledge: These lines are used to acknowledge DMA requests (DRQ1-DRQ3) and to refresh system dynamic memory (DACK0). They are active low.
    UInt8 _bus_irq;
    // +IRQ2..+IRQ7  I  Interrupt Request lines: These lines are used to signal the processor that an I/O device requires attention. An Interrupt Request is generated by raising an IRQ line (low to high) and holding it high until it is acknowledged by the processor (interrupt service routine).
    UInt8 _bus_pit;     // clock, gate, output
    bool _bus_ior;      // -IOR         O -I/O Read Command: This command line instructs an I/O device to drive its data onto the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
    bool _bus_iow;      // -IOW         O -I/O Write Command: This command line instructs an I/O device to read the data on the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
    bool _bus_memr;     // -MEMR        O Memory Read Command: This command line instructs the memory to drive its data onto the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
    bool _bus_memw;     // -MEMW        O Memory Write Command: This command line instructs the memory to store the data present on the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
    bool _bus_iochrdy;  // +I/O CH RDY I  I/O Channel Ready: This line, normally high (ready), is pulled low (not ready) by a memory or I/O device to lengthen I/O or memory cycles. It allows slower devices to attach to the I/O channel with a minimum of difficulty. Any slow device using this line should drive it low immediately upon detecting a valid address and a read or write command. This line should never be held low longer than 10 clock cycles. Machine cycles (I/O or memory) are extended by an integral number of CLK cycles (210 ns).
    bool _bus_aen;      // +AEN         O Address Enable: This line is used to de-gate the processor and other devices from the I/O channel to allow DMA transfers to take place. When this line is active (high), the DMA controller has control of the address bus, data bus, read command lines (memory and I/O), and the write command lines (memory and I/O).
    bool _bus_tc;       // +T/C         O Terminal Count: This line provides a pulse when the terminal count for any DMA channel is reached. This signal is active high.
    int _pitCycle;
};

class Program : public ProgramBase
{
public:
    void run()
    {
        Array<Byte> testProgram;
        File("runtests.bin").readIntoArray(&testProgram);

        for (int i = 0; i < 0x100; ++i) {
            Instruction instruction(i);
            if (instruction.hasModrm()) {
                static const Byte modrms[] = {
                    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
                    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0xc0};
                int jj = 1;
                if (instruction.isGroup())
                    jj = 8;
                for (int m = 0; m < 25; ++m) {
                    for (int j = 0; j < jj; ++j)
                        addTest(Instruction(i, modrms[m] + (j << 3)));
                }
            }
            else
                addTest(instruction);
        }

        int nextTest = 0;
        int availableLength = 0xff00 - testProgram.count();
        do {
            int totalLength = 0;
            int newNextTest = _tests.count();
            for (int i = nextTest; i < _tests.count(); ++i) {
                int nl = totalLength + _tests[i].length() + 3;
                if (nl > availableLength) {
                    newNextTest = i;
                    break;
                }
                totalLength = nl;
            }
            Array<Byte> output(totalLength + 2);
            Byte* p = &output[0];
            *p = totalLength;
            p[1] = totalLength >> 8;
            p += 2;
            for (int i = nextTest; i < newNextTest; ++i) {
                Emulator emulator;
                int cycles = emulator.expected(_tests[i]);
                *p = cycles;
                ++p;
                _tests[i].output(p);
                p += _tests[i].length();
            }

            {
                auto h = File("runtest.bin").openWrite();
                h.write(testProgram);
                h.write(output);
            }
            NullTerminatedWideString data(String("cmd /c run.bat"));

            {
                PROCESS_INFORMATION pi;
                ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

                STARTUPINFO si;
                ZeroMemory(&si, sizeof(STARTUPINFO));
                si.cb = sizeof(STARTUPINFO);

                IF_FALSE_THROW(CreateProcess(NULL, data, NULL, NULL, FALSE,
                    0, NULL, NULL, &si, &pi) != 0);
                CloseHandle(pi.hThread);
                WindowsHandle hProcess = pi.hProcess;
                IF_FALSE_THROW(WaitForSingleObject(hProcess, 3*60*1000) ==
                    WAIT_OBJECT_0);
            }

            String result = File("runtests.output").contents();
            CharacterSource s(result);
            do {
                if (parse(&s, "FAIL")) {
                    Rational result;
                    Space::parse(&s);
                    if (!Space::parseNumber(&s, &result))
                        throw Exception("Cannot parse number of failing test");
                    int n = result.floor() + nextTest;

                    console.write(decimal(n) + "\n");
                    _tests[n].write();

                    CharacterSource s2(s);
                    CharacterSource oldS2(s2);
                    do {
                        if (parse(&s2, "Program ended normally."))
                            break;
                        int c = s2.get();
                        if (c == -1)
                            throw Exception("runtests didn't end properly");
                        oldS2 = s2;
                    } while (true);
                    String observed = s.subString(s.offset(), oldS2.offset());
                    File("observed.txt").openWrite().write(observed);
                    Emulator emulator;
                    String expected = emulator.log(_tests[n + nextTest]);
                    File("expected.txt").openWrite().write(expected);

                    PROCESS_INFORMATION pi;
                    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

                    STARTUPINFO si;
                    ZeroMemory(&si, sizeof(STARTUPINFO));
                    si.cb = sizeof(STARTUPINFO);

                    NullTerminatedWideString data(
                        String("windiff observed.txt expected.txt"));

                    IF_FALSE_THROW(CreateProcess(NULL, data, NULL, NULL, FALSE, 0,
                        NULL, NULL, &si, &pi) != 0);
                    break;
                }
                if (parse(&s, "PASS"))
                    break;
                int c = s.get();
                if (c == -1)
                    throw Exception("Test was inconclusive");
            } while (true);

            nextTest = newNextTest;
            if (nextTest == _tests.count())
                break;

        } while (true);
    }
private:
    void addTest(Instruction i)
    {
        Test t;
        t.addInstruction(i);
        _tests.append(t);
    }
    bool parse(CharacterSource* s, String m)
    {
        CharacterSource ss = *s;
        CharacterSource ms(m);
        do {
            int mc = ms.get();
            if (mc == -1) {
                *s = ss;
                return true;
            }
            int sc = ss.get();
            if (sc != mc)
                return false;
        } while (true);
    }

    AppendableArray<Test> _tests;
};