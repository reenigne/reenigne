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
        return 0; //_cycles * 2;
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
        _busState = t1;
        _queueCycle = 0;

        do {
            executeOneInstruction();
        } while (_ip != _stopIP && _cycles < 2048);
    }
    void wait(int cycles)
    {
        do {
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
                            _queueCycle = 3;
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
            if (_queueCycle > 0) {
                --_queueCycle;
                if (_queueCycle == 0) {
                    ++_prefetched;
                    ++_prefetchAddress;
                    completeInstructionFetch();
                }
            }

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
            _bus_pit = (_cycles & 3) < 2 ? 4 : 5;
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
            if (s != 7 && s != 3)
                switch (tNext) {
                    case 0:
                    case 4:
                        // T1 state occurs after transition out of passive
                        tNext = 1;
                        break;
                    case 1:
                        tNext = 2;
                        break;
                    case 2:
                        tNext = 3;
                        break;
                    case 3:
                        tNext = 5;
                        break;
                }
            else
                switch (t) {
                    case 4:
                        d = -1;
                    case 0:
                        tNext = 0;
                        break;
                    case 1:
                    case 2:
                        tNext = 6;
                        break;
                    case 3:
                    case 5:
                        d = -1;
                        tNext = 4;
                        break;
                }
            switch (t) {
                case 0: line += "  "; break;
                case 1: line += "T1"; break;
                case 2: line += "T2"; break;
                case 3: line += "T3"; break;
                case 4: line += "T4"; break;
                case 5: line += "Tw"; break;
                default: line += "!c"; tNext = 0; break;
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
            if (tNext == 4 || d == 4) {
                if (tNext == 4 && d == 4)
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
            t = tNext;
            console.write(line);

            //++_cycles;
            _pitCycle = (_pitCycle + 1) & 3;
            --cycles;
        } while (cycles > 0);
    }
    Byte fetchInstructionByte()
    {
        do
            wait(1);
        while (_prefetched == 0);
        UInt8 byte = _prefetchQueue[_prefetchOffset & 3];
        _prefetchOffset = (_prefetchOffset + 1) & 3;
        --_prefetched;
        _cpu_qs = 3;
        return byte;
    }

    void executeOneInstruction()
    {
        Byte opcode = fetchInstructionByte();
        switch (opcode) {
            case 0x00: case 0x01: case 0x02: case 0x03:
            case 0x08: case 0x09: case 0x0a: case 0x0b: 
            case 0x10: case 0x11: case 0x12: case 0x13:
            case 0x18: case 0x19: case 0x1a: case 0x1b:
            case 0x20: case 0x21: case 0x22: case 0x23:
            case 0x28: case 0x29: case 0x2a: case 0x2b: 
            case 0x30: case 0x31: case 0x32: case 0x33:
            case 0x38: case 0x39: case 0x3a: case 0x3b: // alu rm, r / r, rm
                readEA();
                wait(3);
                if (!_sourceIsRM) {
                    _destination = _data;
                    _source = getReg();
                    wait(3);
                }
                else {
                    _destination = getReg();
                    _source = _data;
                }
                if (_useMemory)
                    wait(2);
                _aluOperation = (opcode >> 3) & 7;
                doALUOperation();
                if (_aluOperation != 7) {
                    if (!_sourceIsRM)
                        writeEA(_data, 0);
                    else
                        setReg(_data);
                }
                break;
            case 0x04: case 0x05: case 0x0c: case 0x0d:
            case 0x14: case 0x15: case 0x1c: case 0x1d:
            case 0x24: case 0x25: case 0x2c: case 0x2d:
            case 0x34: case 0x35: case 0x3c: case 0x3d: // alu A, imm
                // TODO
                break;
            case 0x06: case 0x0e: case 0x16: case 0x1e: // POP segreg
                // TODO
                break;
            case 0x07: case 0x0f: case 0x17: case 0x1f: // PUSH segreg
                // TODO
                break;
            case 0x26: case 0x2e: case 0x36: case 0x3e: // segreg:
                // TODO
                break;
            case 0x27: // DAA
                // TODO
                break;
            case 0x2f: // DAS
                // TODO
                break;
            case 0x37: // AAA
                // TODO
                break;
            case 0x3f: // AAS
                // TODO
                break;
            case 0x40: case 0x41: case 0x42: case 0x43:
            case 0x44: case 0x45: case 0x46: case 0x47:
            case 0x48: case 0x49: case 0x4a: case 0x4b:
            case 0x4c: case 0x4d: case 0x4e: case 0x4f: // INCDEC rw
                // TODO
                break;
            case 0x50: case 0x51: case 0x52: case 0x53:
            case 0x54: case 0x55: case 0x56: case 0x57: // PUSH rw
                // TODO
                break;
            case 0x58: case 0x59: case 0x5a: case 0x5b:
            case 0x5c: case 0x5d: case 0x5e: case 0x5f: // POP rw
                // TODO
                break;
            case 0x60: case 0x61: case 0x62: case 0x63:
            case 0x64: case 0x65: case 0x66: case 0x67:
            case 0x68: case 0x69: case 0x6a: case 0x6b:
            case 0x6c: case 0x6d: case 0x6e: case 0x6f:
            case 0x70: case 0x71: case 0x72: case 0x73:
            case 0x74: case 0x75: case 0x76: case 0x77:
            case 0x78: case 0x79: case 0x7a: case 0x7b:
            case 0x7c: case 0x7d: case 0x7e: case 0x7f: // Jcond cb
                // TODO
                break;
            case 0x80: case 0x81: case 0x82: case 0x83: // alu rm, imm
                // TODO
                break;
            case 0x84: case 0x85: // TEST rm, reg
                // TODO
                break;
            case 0x86: case 0x87: // XCHG rm, reg
                // TODO
                break;
            case 0x88: case 0x89: case 0x8a: case 0x8b: // MOV rm, reg
                // TODO
                break;
            case 0x8c: // MOV rmw, segreg
                // TODO
                break;
            case 0x8d: // LEA rw, rmw
                // TODO
                break;
            case 0x8e: // MOV segreg, rmw
                // TODO
                break;
            case 0x8f: // POP rmw
                // TODO
                break;
            case 0x90: case 0x91: case 0x92: case 0x93:
            case 0x94: case 0x95: case 0x96: case 0x97: // XCHG AX, rw
                // TODO
                break;
            case 0x98: // CBW
                // TODO
                break;
            case 0x99: // CWD
                // TODO
                break;
            case 0x9a: // CALL cp
                // TODO
                break;
            case 0x9b: // WAIT
                // TODO
                break;
            case 0x9c: // PUSHF
                // TODO
                break;
            case 0x9d: // POPF
                // TODO
                break;
            case 0x9e: // SAHF
                // TODO
                break;
            case 0x9f: // LAHF
                // TODO
                break;
            case 0xa0: case 0xa1: // MOV A, [iw]
                // TODO
                break;
            case 0xa2: case 0xa3: // MOV [iw], A
                // TODO
                break;
            case 0xa4: case 0xa5: // MOVS
                // TODO
                break;
            case 0xa6: case 0xa7: // CMPS
                // TODO
                break;
            case 0xa8: case 0xa9: // TEST A, imm
                // TODO
                break;
            case 0xaa: case 0xab: // STOS
                // TODO
                break;
            case 0xac: case 0xad: // LODS
                // TODO
                break;
            case 0xae: case 0xaf: // SCAS
                // TODO
                break;
            case 0xb0: case 0xb1: case 0xb2: case 0xb3:
            case 0xb4: case 0xb5: case 0xb6: case 0xb7:
            case 0xb8: case 0xb9: case 0xba: case 0xbb:
            case 0xbc: case 0xbd: case 0xbe: case 0xbf: // MOV r, imm
                // TODO
                break;
            case 0xc0: case 0xc1: case 0xc2: case 0xc3: 
            case 0xc8: case 0xc9: case 0xca: case 0xcb: // RET
                // TODO
                break;
            case 0xc4: case 0xc5: // LsS rw, rmd
                // TODO
                break;
            case 0xc6: case 0xc7: // MOV rm, imm
                // TODO
                break;
            case 0xcc: // INT 3
                // TODO
                break;
            case 0xcd: // INT
                // TODO
                break;
            case 0xce: // INTO
                // TODO
                break;
            case 0xcf: // IRET
                // TODO
                break;
            case 0xd0: case 0xd1: case 0xd2: case 0xd3: // rot rm
                // TODO
                break;
            case 0xd4: // AAM
                // TODO
                break;
            case 0xd5: // AAD
                // TODO
                break;
            case 0xd6: // SALC
                // TODO
                break;
            case 0xd7: // XLATB
                // TODO
                break;
            case 0xd8: case 0xd9: case 0xda: case 0xdb:
            case 0xdc: case 0xdd: case 0xde: case 0xdf: // esc i, r, rm
                // TODO
                break;
            case 0xe0: case 0xe1: case 0xe2: case 0xe3: // loop
                // TODO
                break;
            case 0xe4: case 0xe5: case 0xe6: case 0xe7:
            case 0xec: case 0xed: case 0xee: case 0xef: // INOUT
                // TODO
                break;
            case 0xe8: // CALL cw
                // TODO
                break;
            case 0xe9: // JMP cw
                // TODO
                break;
            case 0xea: // JMP cp
                // TODO
                break;
            case 0xeb: // JMP cb
                // TODO
                break;
            case 0xf0: case 0xf1: // LOCK
                // TODO
                break;
            case 0xf2: case 0xf3: // REP
                // TODO
                break;
            case 0xf4: // HLT
                // TODO
                break;
            case 0xf5: // CMC
                // TODO
                break;
            case 0xf6: case 0xf7: // math
                // TODO
                break;
            case 0xf8: case 0xf9: // CLCSTC
                // TODO
                break;
            case 0xfa: case 0xfb: // CLISTI
                // TODO
                break;
            case 0xfc: case 0xfd: // CLDSTD
                // TODO
                break;
            case 0xfe: case 0xff: // misc
                // tODO
                break;
        }
    }

    bool _logging;
    Test _test;
    String _log;
    int _cycles;
    Array<Byte> _ram;
    int _stopIP;

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

    Disassembler _disassembler;
    // These represent the CPU and ISA bus pins used to create the sniffer
    // logs.
    UInt8 _cpu_qs;
    // QS0           O QUEUE STATUS: provide status to allow external tracking of the internal 8088 instruction queue. The queue status is valid during the CLK cycle after which the queue operation is performed.
    // QS1           0 = No operation, 1 = First Byte of Opcode from Queue, 2 = Empty the Queue, 3 = Subsequent Byte from Queue

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