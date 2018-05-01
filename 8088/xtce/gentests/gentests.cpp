#include "alfe/main.h"
#include "alfe/space.h"
#include "alfe/hash_table.h"
#include "alfe/set.h"
#include <random>

static const UInt16 testSegment = 0xa8;

class Instruction
{
public:
    Instruction() { }
    Instruction(Byte opcode, Byte modrm = 0, Word offset = 0,
        DWord immediate = 0)
      : _opcode(opcode), _modrm(modrm), _offset(offset), _immediate(immediate)
    { }
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
            2, 0, 2, 0, 0, 0, 1, 2, 2, 0, 2, 0, 0, 1, 0, 0,
            0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 4, 1, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        if ((_opcode & 0xfe) == 0xf6 && (_modrm & 0x30) == 0)
            return (_opcode & 1) + 1;
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
            if (l > 1) {
                *p = (_offset & 0xff);
                if (l > 2)
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
        return o == 0xf6 || o == 0xfe || o == 0x80 || o == 0x82 || o == 0xd0 ||
            o == 0xd2;
    }
    void write()
    {
        console.write("{" + hex(_opcode, 2) + ", " + hex(_modrm, 2) + ", " +
            hex(_offset, 4) + ", " + hex(_immediate) + "}");
    }
    Byte opcode() { return _opcode; }
    Byte modrm() { return _modrm; }
    void setImmediate(DWord immediate) { _immediate = immediate; }
    void setModrm(Byte modrm) { _modrm = modrm; }
    bool operator==(const Instruction& other) const
    {
        return _opcode == other._opcode && _modrm == other._modrm &&
            _offset == other._offset && _immediate == other._immediate;
    }
    bool operator!=(const Instruction& other) const
    {
        return !(*this == other);
    }
    UInt32 hash() const
    {
        return Hash(typeid(Instruction)).mixin(_opcode).mixin(_modrm).
            mixin(_offset).mixin(_immediate);
    }
    Byte* read(Byte* p)
    {
        _opcode = *p;
        ++p;
        if (hasModrm()) {
            _modrm = *p;
            ++p;
            int l = modRMLength();
            if (l > 1) {
                _offset = *p;
                ++p;
                if (l > 2) {
                    _offset += *p << 8;
                    ++p;
                }
            }
        }
        int l = immediateBytes();
        if (l > 0) {
            _immediate = *p;
            ++p;
            if (l > 1) {
                _immediate += *p << 8;
                ++p;
                if (l > 2) {
                    _immediate += p[0] << 16;
                    _immediate += p[1] << 24;
                    p += 2;
                }
            }
        }
        return p;
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
    Test(int queueFiller = 0, int nops = -1)
      : _queueFiller(queueFiller), _nops(nops), _refreshPeriod(0),
        _refreshPhase(0)
    { }
    Test(std::initializer_list<Instruction> instructions, int nopCount,
        int refreshPeriod = 0, int refreshPhase = 0)
    {
        int n = instructions.size();
        auto p = instructions.begin();
        for (int i = 0; i < n; ++i)
            _instructions.append(p[i]);
        _refreshPeriod = refreshPeriod;
        _refreshPhase = refreshPhase;
    }
    Test copy()
    {
        Test t(*this);
        t._preamble = _preamble.copy();
        t._instructions = _instructions.copy();
        t._fixups = _fixups.copy();
        return t;
    }
    void addInstruction(Instruction instruction)
    {
        _instructions.append(instruction);
    }
    int codeLength()  // Just the code bytes
    {
        int l = 0;
        for (auto i : _instructions)
            l += i.length();
        return l;
    }
    int length()  // Entire test
    {
        return 6 + _preamble.count() + 1 + codeLength() + 1 +
            _fixups.count();
    }
    int nopBytes()
    {
        switch (_nops) {
            case 11:
            case 12:
                return 3;
            case 13:
                return 4;
            case 14:
                return 2;
            case 15:
                return 3;
            case 16:
                return 3;
            default:
                return _nops;
        }
    }
    void output(Byte* p)       // For the real hardware and cache
    {
        p[0] = (_queueFiller << 5) + _nops;
        p[1] = _refreshPeriod;
        p[2] = _refreshPhase;
        p += 3;
        int pc = _preamble.count();
        *p = pc;
        ++p;
        for (int i = 0; i < pc; ++i) {
            *p = _preamble[i];
            ++p;
        }
        int l = codeLength();
        *p = l;
        ++p;
        outputBytes(p);
        p += l;
        int f = _fixups.count();
        *p = f;
        ++p;
        for (int i = 0; i < f; ++i) {
            *p = _fixups[i];
            ++p;
        }
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
    Word readWord(Byte* p) { return p[0] + (p[1] << 8); }
    void writeWord(Byte* p, Word w)
    {
        p[0] = static_cast<Byte>(w);
        p[1] = w >> 8;
    }
    Byte* outputCode(Byte* p)  // For the emulator
    {
        Byte* pStart = p;

        int pc = _preamble.count();
        for (int i = 0; i < pc; ++i)
            p[i] = _preamble[i];
        p += pc;

        int ql = 0;
        switch (_queueFiller) {
            case 0:
                p[0] = 0xb0;
                p[1] = 0x00;
                p[2] = 0xf6;
                p[3] = 0xe0;
                ql = 4;
                break;
            case 1:
                p[0] = 0xb1;
                p[1] = 0x10;
                p[2] = 0xd2;
                p[3] = 0xe9;
                ql = 4;
                break;
            case 2:
                ql = 0;
                break;
            default:
                throw Exception("Unknown queue filler.");
        }
        p += ql;
        switch (_nops) {
            case 11:
            case 12:
                p[0] = 0x90;
                p[1] = 0x02;
                p[2] = _nops == 11 ? 0x28 : 0x10;
                break;
            case 13:
                p[0] = 0x90;
                p[1] = 0x90;
                p[2] = 0x90;
                p[3] = 0x37;
                break;
            case 14:
                p[0] = 0xc4;
                p[1] = 0x10;
                break;
            case 15:
                p[0] = 0x80;
                p[1] = 0x38;
                p[2] = 0x00;
                break;
            case 16:
                p[0] = 0xf6;
                p[1] = 0x00;
                p[2] = 0x00;
                break;
            default:
                for (int i = 0; i < _nops; ++i)
                    p[i] = 0x90;
        }
        p += nopBytes();
        Word instructionsOffset = pc + ql + nopBytes();
        _startIP = instructionsOffset;
        p = outputBytes(p);
        for (int i = 0; i < _fixups.count(); ++i) {
            Byte f = _fixups[i];
            Byte* base = pStart + (f & 0x7f);
            if ((f & 0x80) != 0)
                base += instructionsOffset;
            writeWord(base, instructionsOffset + readWord(base));
        }

        p[0] = 0xeb;
        p[1] = 0x00;
        p[2] = 0xcd;
        p[3] = 0xff;
        for (int i = 0; i < 4; ++i)
            p[i + 4] = 0;
        return p + 4;
    }
    Byte* read(Byte* p)
    {
        _cycles = p[0] + (p[1] << 8);
        _queueFiller = p[2] >> 5;
        _nops = p[2] & 0x1f;
        _refreshPeriod = p[3];
        _refreshPhase = p[4];
        int c = p[5];
        p += 6;
        for (int i = 0; i < c; ++i)
            _preamble.append(p[i]);
        p += c;
        c = p[0];
        ++p;
        int i = 0;
        while (i < c) {
            Instruction instruction(0);
            p = instruction.read(p);
            _instructions.append(instruction);
            i += instruction.length();
        }
        c = p[0];
        ++p;
        for (int i = 0; i < c; ++i)
            _fixups.append(p[i]);
        p += c;
        return p;
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
        console.write("}, " + decimal((_queueFiller << 5) + _nops) + ", " +
            decimal(_refreshPeriod) + ", " + decimal(_refreshPhase) + "}\n");
    }
    void setCycles(int cycles) { _cycles = cycles; }
    int cycles() { return _cycles; }
    void preamble(Byte p) { _preamble.append(p); }
    void fixup(Byte f = 0xff)
    {
        if (f == 0xff)
            f = _preamble.count();
        if ((f & 0x80) != 0)
            f += _instructions.count();
        _fixups.append(f);
    }
    void setQueueFiller(int queueFiller) { _queueFiller = queueFiller; }
    int queueFiller() { return _queueFiller; }
    void setNops(int nops) { _nops = nops; }
    int nops() { return _nops; }
    int startIP() { return _startIP; }
    Instruction instruction(int i) { return _instructions[i]; }
    void setRefreshPeriod(int p) { _refreshPeriod = p; }
    void setRefreshPhase(int p) { _refreshPhase = p; }
    int refreshPeriod() { return _refreshPeriod; }
    int refreshPhase() { return _refreshPhase; }

    bool operator==(const Test& other) const
    {
        if (_queueFiller != other._queueFiller || /*_nops != other._nops || */
            _refreshPeriod != other._refreshPeriod ||
            _refreshPhase != other._refreshPhase)
            return false;
        int c = _preamble.count();
        if (c != other._preamble.count())
            return false;
        for (int i = 0; i < c; ++i)
            if (_preamble[i] != other._preamble[i])
                return false;
        c = _instructions.count();
        if (c != other._instructions.count())
            return false;
        for (int i = 0; i < c; ++i)
            if (_instructions[i] != other._instructions[i])
                return false;
        c = _fixups.count();
        if (c != other._fixups.count())
            return false;
        for (int i = 0; i < c; ++i)
            if (_fixups[i] != other._fixups[i])
                return false;
        return true;
    }
    bool equalIncludingNops(const Test& other) const
    {
        return *this == other && _nops == other._nops;
    }
    bool operator!=(const Test& other) const { return !(*this == other); }
    UInt32 hash() const
    {
        Hash h(typeid(Test));
        //h.mixin(_queueFiller).mixin(_nops);
        int c = _preamble.count();
        h.mixin(c);
        for (int i = 0; i < c; ++i)
            h.mixin(_preamble[i]);
        c = _instructions.count();
        h.mixin(c);
        for (int i = 0; i < c; ++i)
            h.mixin(_instructions[i].hash());
        c = _fixups.count();
        h.mixin(c);
        for (int i = 0; i < c; ++i)
            h.mixin(_fixups[i]);
        h.mixin(_refreshPeriod);
        h.mixin(_refreshPhase);
        return h;
    }
private:
    int _queueFiller;
    int _nops;

    AppendableArray<Byte> _preamble;
    AppendableArray<Instruction> _instructions;
    AppendableArray<Byte> _fixups;
    int _cycles;
    int _startIP;
    int _refreshPeriod;
    int _refreshPhase;
};

#define GENERATE_NEWFAILS 1

#if GENERATE_NEWFAILS
#include "fails.h"
#endif

class Disassembler
{
public:
    Disassembler() { reset(); }
    void reset() { _byteCount = 0; }
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
            case 0xc4: _doubleWord = true; return "LES " + rw() + ", " + ea();
            case 0xc5:
                _doubleWord = true;
                _wordSize = false;
                return "LDS " + rw() + ", " + ea();
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

class SnifferDecoder
{
public:
    void reset()
    {
        _cpu_rqgt0 = false;  // Used by 8087 for bus mastering, NYI
        _cpu_ready = true;   // Used for DMA and wait states
        _cpu_test = false;   // Used by 8087 for synchronization, NYI
        _cpu_lock = false;
        _bus_dma = 0;        // NYI
        _dmas = 0;
        _bus_irq = 0xfc;     // NYI
        _int = false;
        _bus_iochrdy = true; // Used for wait states, NYI
        _bus_aen = false;    // Used for DMA
        _bus_tc = false;     // Used for DMA, NYI
        _cga = 0;

        _t = 0;
        _tNext = 0;
        _d = -1;
        _queueLength = 0;
        _lastS = 0;
        _cpu_s = 7;
        _cpu_qs = 0;

        _disassembler.reset();
    }
    String getLine()
    {
        static const char qsc[] = ".IES";
        static const char sc[] = "ARWHCrwp";
        static const char dmasc[] = " h:H";
        String line;
        if (_cpuDataFloating)
            line = String(hex(_cpu_ad >> 8, 3, false)) + "??";
        else
            line = String(hex(_cpu_ad, 5, false));
        line += " " +
            codePoint(qsc[_cpu_qs]) + codePoint(sc[_cpu_s]) +
            (_cpu_rqgt0 ? "G" : ".") + (_cpu_ready ? "." : "z") +
            (_cpu_test ? "T" : ".") + (_cpu_lock ? "L" : ".") +
            "  " + hex(_bus_address, 5, false) + " ";
        if (_isaDataFloating)
            line += "??";
        else
            line += hex(_bus_data, 2, false);
        line += " " + hex(_bus_dma, 2, false) + codePoint(dmasc[_dmas]) +
            " " + hex(_bus_irq, 2, false) + (_int ? "I" : " ") + " " +
            hex(_bus_pit, 1, false) + hex(_cga, 1, false) + " " + (_bus_ior ? "R" : ".") +
            (_bus_iow ? "W" : ".") + (_bus_memr ? "r" : ".") +
            (_bus_memw ? "w" : ".") + (_bus_iochrdy ? "." : "z") +
            (_bus_aen ? "D" : ".") +
            (_bus_tc ? "T" : ".");
        line += "  ";
        if (_cpu_s != 7 && _cpu_s != 3)
            switch (_tNext) {
                case 0:
                case 4:
                    // T1 state occurs after transition out of passive
                    _tNext = 1;
                    break;
                case 1:
                    _tNext = 2;
                    break;
                case 2:
                    _tNext = 3;
                    break;
                case 3:
                    _tNext = 5;
                    break;
            }
        else
            switch (_t) {
                case 4:
                    _d = -1;
                case 0:
                    _tNext = 0;
                    break;
                case 1:
                case 2:
                    _tNext = 6;
                    break;
                case 3:
                case 5:
                    _d = -1;
                    _tNext = 4;
                    break;
            }
        switch (_t) {
            case 0: line += "  "; break;
            case 1: line += "T1"; break;
            case 2: line += "T2"; break;
            case 3: line += "T3"; break;
            case 4: line += "T4"; break;
            case 5: line += "Tw"; break;
            default: line += "!c"; _tNext = 0; break;
        }
        line += " ";
        if (_bus_aen)
            switch (_d) {
                // This is a bit of a hack since we don't have access
                // to the right lines to determine the DMA state
                // properly. This probably breaks for memory-to-memory
                // copies.
                case -1: _d = 0; break;
                case 0: _d = 1; break;
                case 1: _d = 2; break;
                case 2: _d = 3; break;
                case 3:
                case 5:
                    if ((_bus_iow && _bus_memr) || (_bus_ior && _bus_memw))
                        _d = 4;
                    else
                        _d = 5;
                    break;
                case 4:
                    _d = -1;
            }
        switch (_d) {
            case -1: line += "  "; break;
            case 0: line += "S0"; break;
            case 1: line += "S1"; break;
            case 2: line += "S2"; break;
            case 3: line += "S3"; break;
            case 4: line += "S4"; break;
            case 5: line += "SW"; break;
            default: line += "!d"; _t = 0; break;
        }
        line += " ";
        String instruction;
        if (_cpu_qs != 0) {
            if (_cpu_qs == 2)
                _queueLength = 0;
            else {
                Byte b = _queue[0];
                for (int i = 0; i < 3; ++i)
                    _queue[i] = _queue[i + 1];
                --_queueLength;
                if (_queueLength < 0) {
                    line += "!g";
                    _queueLength = 0;
                }
                instruction = _disassembler.disassemble(b, _cpu_qs == 1);
            }
        }
        if (_tNext == 4 || _d == 4) {
            if (_tNext == 4 && _d == 4)
                line += "!e";
            String seg;
            switch (_cpu_ad & 0x30000) {
                case 0x00000: seg = "ES "; break;
                case 0x10000: seg = "SS "; break;
                case 0x20000: seg = "CS "; break;
                case 0x30000: seg = "DS "; break;
            }
            String type = "-";
            if (_lastS == 0)
                line += hex(_bus_data, 2, false) + " <-i           ";
            else {
                if (_lastS == 4) {
                    type = "f";
                    seg = "   ";
                }
                if (_d == 4) {
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
                if (_lastS == 4 && _d != 4) {
                    if (_queueLength >= 4)
                        line += "!f";
                    else {
                        _queue[_queueLength] = _bus_data;
                        ++_queueLength;
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
        _lastS = _cpu_s;
        _t = _tNext;
        if (_t == 4 || _d == 4) {
            _bus_ior = false;
            _bus_iow = false;
            _bus_memr = false;
            _bus_memw = false;
        }
        _cpu_qs = 0;
        return line;
    }
    void queueOperation(int qs) { _cpu_qs = qs; }
    void setStatus(int s) { _cpu_s = s; }
    void setStatusHigh(int segment)
    {
        _cpu_ad &= 0xcffff;
        switch (segment) {
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
        setBusFloating();
    }
    void setInterruptFlag(bool intf)
    {
        _cpu_ad = (_cpu_ad & 0xbffff) | (intf ? 0x40000 : 0);
    }
    void setBusOperation(int s)
    {
        switch (s) {
            case 1: _bus_ior = true; break;
            case 2: _bus_iow = true; break;
            case 4:
            case 5: _bus_memr = true; break;
            case 6: _bus_memw = true; break;
        }
    }
    void setData(Byte data)
    {
        _cpu_ad = (_cpu_ad & 0xfff00) | data;
        _bus_data = data;
        _cpuDataFloating = false;
        _isaDataFloating = false;
    }
    void setAddress(UInt32 address)
    {
        _cpu_ad = address;
        _bus_address = address;
        _cpuDataFloating = false;
    }
    void setBusFloating()
    {
        _cpuDataFloating = true;
        _isaDataFloating = true;
    }
    void setPITBits(int bits) { _bus_pit = bits; }
    void setAEN(bool aen) { _bus_aen = aen; }
    void setDMA(UInt8 dma) { _bus_dma = dma; }
    void setReady(bool ready) { _cpu_ready = ready; }
    void setLock(bool lock) { _cpu_lock = lock; }
    void setDMAS(UInt8 dmas) { _dmas = dmas; }
    void setIRQs(UInt8 irq) { _bus_irq = irq; }
    void setINT(bool intrq) { _int = intrq; }
    void setCGA(UInt8 cga) { _cga = cga; }
private:
    Disassembler _disassembler;

    // Internal variables that we use to keep track of what's going on in order
    // to be able to print useful logs.
    int _t;  // 0 = Tidle, 1 = T1, 2 = T2, 3 = T3, 4 = T4, 5 = Tw
    int _tNext;
    int _d;  // -1 = SI, 0 = S0, 1 = S1, 2 = S2, 3 = S3, 4 = S4, 5 = SW
    Byte _queue[4];
    int _queueLength;
    int _lastS;

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
    bool _cpu_lock;     // -LOCK    !87  O LOCK: indicates that other system bus masters are not to gain control of the system bus while LOCK is active (LOW). The LOCK signal is activated by the ``LOCK'' prefix instruction and remains active until the completion of the next instruction. This signal is active LOW, and floats to 3-state off in ``hold acknowledge''.
    UInt32 _bus_address;
    // +A19..+A0      O Address bits: These lines are used to address memory and I/O devices within the system. These lines are generated by either the processor or DMA controller.
    UInt8 _bus_data;
    // +D7..+D0      IO Data bits: These lines provide data bus bits 0 to 7 for the processor, memory, and I/O devices.
    UInt8 _bus_dma;
    // +DRQ0 JP6/1 == U28.19 == U73.9
    // +DRQ1..+DRQ3  I  DMA Request: These lines are asynchronous channel requests used by peripheral devices to gain DMA service. They are prioritized with DRQ3 being the lowest and DRQl being the highest. A request is generated by bringing a DRQ line to an active level (high). A DRQ line must be held high until the corresponding DACK line goes active.
    // -DACK0..-DACK3 O -DMA Acknowledge: These lines are used to acknowledge DMA requests (DRQ1-DRQ3) and to refresh system dynamic memory (DACK0). They are active low.
    UInt8 _dmas;        // JP9/4 HRQ DMA (bit 0), JP4/1 HOLDA (bit 1)
    UInt8 _bus_irq;
    // +IRQ0..+IRQ7  I  Interrupt Request lines: These lines are used to signal the processor that an I/O device requires attention. An Interrupt Request is generated by raising an IRQ line (low to high) and holding it high until it is acknowledged by the processor (interrupt service routine).
    bool _int;          // JP9/1 INT
    UInt8 _cga;         // JP7/2  CGA HCLK (bit 0), JP7/1  CGA LCLK (bit 1)
    UInt8 _bus_pit;     // clock, gate, output
    bool _bus_ior;      // -IOR         O -I/O Read Command: This command line instructs an I/O device to drive its data onto the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
    bool _bus_iow;      // -IOW         O -I/O Write Command: This command line instructs an I/O device to read the data on the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
    bool _bus_memr;     // -MEMR        O Memory Read Command: This command line instructs the memory to drive its data onto the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
    bool _bus_memw;     // -MEMW        O Memory Write Command: This command line instructs the memory to store the data present on the data bus. It may be driven by the processor or the DMA controller. This signal is active low.
    bool _bus_iochrdy;  // +I/O CH RDY I  I/O Channel Ready: This line, normally high (ready), is pulled low (not ready) by a memory or I/O device to lengthen I/O or memory cycles. It allows slower devices to attach to the I/O channel with a minimum of difficulty. Any slow device using this line should drive it low immediately upon detecting a valid address and a read or write command. This line should never be held low longer than 10 clock cycles. Machine cycles (I/O or memory) are extended by an integral number of CLK cycles (210 ns).
    bool _bus_aen;      // +AEN         O Address Enable: This line is used to de-gate the processor and other devices from the I/O channel to allow DMA transfers to take place. When this line is active (high), the DMA controller has control of the address bus, data bus, read command lines (memory and I/O), and the write command lines (memory and I/O).
    bool _bus_tc;       // +T/C         O Terminal Count: This line provides a pulse when the terminal count for any DMA channel is reached. This signal is active high.
    bool _cpuDataFloating;
    bool _isaDataFloating;
};

class PITEmulator
{
public:
    void reset()
    {
        for (int i = 0; i < 3; ++i)
            _counters[i].reset();
    }
    void stubInit()
    {
        for (int i = 0; i < 3; ++i)
            _counters[i].stubInit();
    }
    void write(int address, Byte data)
    {
        if (address == 3) {
            int counter = data >> 6;
            if (counter == 3)
                return;
            _counters[counter].control(data & 0x3f);
        }
        else
            _counters[address].write(data);
    }
    Byte read(int address)
    {
        if (address == 3)
            return 0xff;
        return _counters[address].read();
    }
    void wait()
    {
        for (int i = 0; i < 3; ++i)
            _counters[i].wait();
    }
    void setGate(int counter, bool gate)
    {
        _counters[counter].setGate(gate);
    }
    bool getOutput(int counter) { return _counters[counter]._output; }
    //int getMode(int counter) { return _counters[counter]._control; }
private:
    enum State
    {
        stateWaitingForCount,
        stateCounting,
        stateWaitingForGate,
        stateGateRose,
        stateLoadDelay,
        statePulsing
    };
    struct Counter
    {
        void reset()
        {
            _value = 0;
            _count = 0;
            _firstByte = true;
            _latched = false;
            _output = true;
            _control = 0x30;
            _state = stateWaitingForCount;
        }
        void stubInit()
        {
            _value = 0xffff;
            _count = 0xffff;
            _firstByte = true;
            _latched = false;
            _output = true;
            _control = 0x34;
            _state = stateCounting;
            _gate = true;
        }
        void write(Byte data)
        {
            _writeByte = data;
            _haveWriteByte = true;
        }
        Byte read()
        {
            if (!_latched) {
                // TODO: corrupt countdown in a deterministic but
                // non-trivial way.
                _latch = _count;
            }
            switch (_control & 0x30) {
                case 0x10:
                    _latched = false;
                    return _latch & 0xff;
                case 0x20:
                    _latched = false;
                    return _latch >> 8;
                case 0x30:
                    if (_firstByte) {
                        _firstByte = false;
                        return _latch & 0xff;
                    }
                    _firstByte = true;
                    _latched = false;
                    return _latch >> 8;
            }
            // This should never happen.
            return 0;
        }
        void wait()
        {
            switch (_control & 0x0e) {
                case 0x00:  // Interrupt on Terminal Count
                    if (_state == stateLoadDelay) {
                        _state = stateCounting;
                        _value = _count;
                        break;
                    }
                    if (_gate && _state == stateCounting) {
                        countDown();
                        if (_value == 0)
                            _output = true;
                    }
                    break;
                case 0x02:  // Programmable One-Shot
                    if (_state == stateLoadDelay) {
                        _state = stateWaitingForGate;
                        break;
                    }
                    if (_state == stateGateRose) {
                        _output = false;
                        _value = _count;
                        _state = stateCounting;
                    }
                    countDown();
                    if (_value == 0) {
                        _output = true;
                        _state = stateWaitingForGate;
                    }
                    break;
                case 0x04:  
                case 0x0c:  // Rate Generator
                    if (_state == stateLoadDelay) {
                        _state = stateCounting;
                        _value = _count;
                        break;
                    }
                    if (_gate && _state == stateCounting) {
                        countDown();
                        if (_value == 1)
                            _output = false;
                        if (_value == 0) {
                            _output = true;
                            _value = _count;
                        }
                    }
                    break;
                case 0x06:  
                case 0x0e:  // Square Wave Rate Generator
                    if (_state == stateLoadDelay) {
                        _state = stateCounting;
                        _value = _count;
                        break;
                    }
                    if (_gate && _state == stateCounting) {
                        if ((_value & 1) != 0) {
                            if (!_output) {
                                countDown();
                                countDown();
                            }
                        }
                        else
                            countDown();
                        countDown();
                        if (_value == 0) {
                            _output = !_output;
                            _value = _count;
                        }
                    }
                    break;
                case 0x08:  // Software Triggered Strobe
                    if (_state == stateLoadDelay) {
                        _state = stateCounting;
                        _value = _count;
                        break;
                    }
                    if (_state == statePulsing) {
                        _output = true;
                        _state = stateWaitingForCount;
                    }
                    if (_gate && _state == stateCounting) {
                        countDown();
                        if (_value == 0) {
                            _output = false;
                            _state = statePulsing;
                        }
                    }
                    break;
                case 0x0a:  // Hardware Triggered Strobe
                    if (_state == stateLoadDelay) {
                        _state = stateWaitingForGate;
                        break;
                    }
                    if (_state == statePulsing) {
                        _output = true;
                        _state = stateWaitingForCount;
                    }
                    if (_state == stateGateRose) {
                        _output = false;
                        _value = _count;
                        _state = stateCounting;
                    }
                    if (_state == stateCounting) {
                        countDown();
                        if (_value == 1)
                            _output = false;
                        if (_value == 0) {
                            _output = true;
                            _state = stateWaitingForGate;
                        }
                    }
                    break;
            }
            if (_haveWriteByte) {
                _haveWriteByte = false;
                switch (_control & 0x30) {
                    case 0x10:
                        load(_writeByte);
                        break;
                    case 0x20:
                        load(_writeByte << 8);
                        break;
                    case 0x30:
                        if (_firstByte) {
                            _lowByte = _writeByte;
                            _firstByte = false;
                        }
                        else {
                            load((_writeByte << 8) + _lowByte);
                            _firstByte = true;
                        }
                        break;
                }
            }
        }
        void countDown()
        {
            if ((_control & 1) == 0) {
                --_value;
                return;
            }
            if ((_value & 0xf) != 0) {
                --_value;
                return;
            }
            if ((_value & 0xf0) != 0) {
                _value -= (0x10 - 9);
                return;
            }
            if ((_value & 0xf00) != 0) {
                _value -= (0x100 - 0x99);
                return;
            }
            _value -= (0x1000 - 0x999);
        }
        void load(Word count)
        {
            _count = count;
            switch (_control & 0x0e) {
                case 0x00:  // Interrupt on Terminal Count
                    if (_state == stateWaitingForCount)
                        _state = stateLoadDelay;
                    _output = false;
                    break;
                case 0x02:  // Programmable One-Shot
                    if (_state != stateCounting)
                        _state = stateLoadDelay;
                    break;
                case 0x04:  
                case 0x0c:  // Rate Generator
                    if (_state == stateWaitingForCount)
                        _state = stateLoadDelay;
                    break;
                case 0x06:  
                case 0x0e:  // Square Wave Rate Generator
                    if (_state == stateWaitingForCount)
                        _state = stateLoadDelay;
                    break;
                case 0x08:  // Software Triggered Strobe
                    if (_state == stateWaitingForCount)
                        _state = stateLoadDelay;
                    break;
                case 0x0a:  // Hardware Triggered Strobe
                    if (_state != stateCounting)
                        _state = stateLoadDelay;
                    break;
            }
        }
        void control(Byte control)
        {
            int command = control & 0x30;
            if (command == 0) {
                _latch = _value;
                _latched = true;
                return;
            }
            _control = control;
            _firstByte = true;
            _latched = false;
            _state = stateWaitingForCount;
            switch (_control & 0x0e) {
                case 0x00:  // Interrupt on Terminal Count
                    _output = false;
                    break;
                case 0x02:  // Programmable One-Shot
                    _output = true;
                    break;
                case 0x04:  
                case 0x0c:  // Rate Generator
                    _output = true;
                    break;
                case 0x06:  
                case 0x0e:  // Square Wave Rate Generator
                    _output = true;
                    break;
                case 0x08:  // Software Triggered Strobe
                    _output = true;
                    break;
                case 0x0a:  // Hardware Triggered Strobe
                    _output = true;
                    break;
            }
        }
        void setGate(bool gate)
        {
            if (_gate == gate)
                return;
            switch (_control & 0x0e) {
                case 0x00:  // Interrupt on Terminal Count
                    break;
                case 0x02:  // Programmable One-Shot
                    if (gate)
                        _state = stateGateRose;
                    break;
                case 0x04:  
                case 0x0c:  // Rate Generator
                    if (!gate)
                        _output = true;
                    else
                        _value = _count;
                    break;
                case 0x06:  
                case 0x0e:  // Square Wave Rate Generator
                    if (!gate)
                        _output = true;
                    else
                        _value = _count;
                    break;
                case 0x08:  // Software Triggered Strobe
                    break;
                case 0x0a:  // Hardware Triggered Strobe
                    if (gate)
                        _state = stateGateRose;
                    break;
            }
            _gate = gate;
        }

        Word _count;
        Word _value;
        Word _latch;
        Byte _control;
        Byte _lowByte;
        bool _gate;
        bool _output;
        bool _firstByte;
        bool _latched;
        State _state;
        Byte _writeByte;
        bool _haveWriteByte;
    };

    Counter _counters[3];
};

class PICEmulator
{
public:
    void reset()
    {
        _interruptPending = false;
        _interrupt = 0;
        _irr = 0;
        _imr = 0;
        _isr = 0;
        _icw1 = 0;
        _icw2 = 0;
        _icw3 = 0;
        _icw4 = 0;
        _ocw3 = 0;
        _lines = 0;
        _specialMaskMode = false;
        _acknowledgedBytes = 0;
        _priority = 0;
        _rotateInAutomaticEOIMode = false;
        _initializationState = initializationStateNone;
    }
    void stubInit()
    {
        _icw1 = 0x13;
        _icw2 = 0x08;
        _icw4 = 0x0f;
        _imr = 0xbc;
    }
    void write(int address, Byte data)
    {
        if (address == 0) {
            if ((data & 0x10) != 0) {
                _icw1 = data;
                if (levelTriggered())
                    _irr = _lines;
                else
                    _irr = 0;
                _initializationState = initializationStateICW2;
                _imr = 0;
                _isr = 0;
                _icw2 = 0;
                _icw3 = 0;
                _icw4 = 0;
                _ocw3 = 0;
                _acknowledgedBytes = 0;
                _priority = 0;
                _rotateInAutomaticEOIMode = false;
                _specialMaskMode = false;
                _interrupt = 0;
                _interruptPending = false;
            }
            else {
                if ((data & 8) == 0) {
                    Byte b = 1 << (data & 7);
                    switch (data & 0xe0) {
                        case 0x00:  // Rotate in automatic EOI mode (clear) (Automatic Rotation)
                            _rotateInAutomaticEOIMode = false;
                            break;
                        case 0x20:  // Non-specific EOI command (End of Interrupt)
                            nonSpecificEOI(false);
                            break;
                        case 0x40:  // No operation
                            break;
                        case 0x60:  // Specific EOI command (End of Interrupt)
                            _isr &= ~b;
                            break;
                        case 0x80:  // Rotate in automatic EOI mode (set) (Automatic Rotation)
                            _rotateInAutomaticEOIMode = true;
                            break;
                        case 0xa0:  // Rotate on non-specific EOI command (Automatic Rotation)
                            nonSpecificEOI(true);
                            break;
                        case 0xc0:  // Set priority command (Specific Rotation)
                            _priority = (data + 1) & 7;
                            break;
                        case 0xe0:  // Rotate on specific EOI command (Specific Rotation)
                            if ((_isr & b) != 0) {
                                _isr &= ~b;
                                _priority = (data + 1) & 7;
                            }
                            break;
                    }
                }
                else {
                    _ocw3 = data;
                    if ((_ocw3 & 0x40) != 0)
                        _specialMaskMode = (_ocw3 & 0x20) != 0;
                }
            }
        }
        else {
            switch (_initializationState) {
                case initializationStateICW2:
                    _icw2 = data;
                    if (cascadeMode())
                        _initializationState = initializationStateICW3;
                    else
                        checkICW4Needed();
                    break;
                case initializationStateICW3:
                    _icw3 = data;
                    checkICW4Needed();
                    break;
                case initializationStateICW4:
                    _icw4 = data;
                    _initializationState = initializationStateNone;
                    break;
                case initializationStateNone:
                    _imr = data;
                    break;
            }
        }
    }
    Byte read(int address)
    {
        if ((_ocw3 & 4) != 0) {  // Poll mode
            acknowledge();
            return (_interruptPending ? 0x80 : 0) + _interrupt;
        }
        if (address == 0) {
            if ((_ocw3 & 1) != 0)
                return _isr;
            return _irr;
        }
        else
            return _imr;
    }
    Byte interruptAcknowledge()
    {
        if (_acknowledgedBytes == 0) {
            acknowledge();
            _acknowledgedBytes = 1;
            if (i86Mode())
                return 0xff;
            else
                return 0xcd;
        }
        if (i86Mode()) {
            _acknowledgedBytes = 0;
            if (autoEOI())
                nonSpecificEOI(_rotateInAutomaticEOIMode);
            _interruptPending = false;
            if (slaveOn(_interrupt))
                return 0xff;  // Filled in by slave PIC
            return _interrupt + (_icw2 & 0xf8);
        }
        if (_acknowledgedBytes == 1) {
            _acknowledgedBytes = 2;
            if (slaveOn(_interrupt))
                return 0xff;  // Filled in by slave PIC
            if ((_icw1 & 4) != 0)  // Call address interval 4
                return (_interrupt << 2) + (_icw1 & 0xe0);
            return (_interrupt << 3) + (_icw1 & 0xc0);
        }
        _acknowledgedBytes = 0;
        if (autoEOI())
            nonSpecificEOI(_rotateInAutomaticEOIMode);
        _interruptPending = false;
        if (slaveOn(_interrupt))
            return 0xff;  // Filled in by slave PIC
        return _icw2;
    }
    void setIRQLine(int line, bool state)
    {
        Byte b = 1 << line;
        if (state) {
            if (levelTriggered() || (_lines & b) == 0)
                _irr |= b;
            _lines |= b;
        }
        else {
            _irr &= ~b;
            _lines &= ~b;
        }
        if (findBestInterrupt() != -1)
            _interruptPending = true;
    }
    bool interruptPending() const { return _interruptPending; }
    UInt8 getIRQLines() { return _lines; }
private:
    bool cascadeMode() { return (_icw1 & 2) == 0; }
    bool levelTriggered() { return (_icw1 & 8) != 0; }
    bool i86Mode() { return (_icw4 & 1) != 0; }
    bool autoEOI() { return (_icw4 & 2) != 0; }
    bool slaveOn(int channel)
    {
        return cascadeMode() && (_icw4 & 0xc0) == 0xc0 &&
            (_icw3 & (1 << channel)) != 0;
    }
    int findBestInterrupt()
    {
        int n = _priority;
        for (int i = 0; i < 8; ++i) {
            Byte b = 1 << n;
            bool s = (_icw4 & 0x10) != 0 && slaveOn(n);
            if ((_isr & b) != 0 && !_specialMaskMode && !s)
                break;
            if ((_irr & b) != 0 && (_imr & b) == 0 && ((_isr & b) == 0 || s))
                return n;
            if ((_isr & b) != 0 && !_specialMaskMode && s)
                break;
            n = (n + 1) & 7;
        }
        return -1;
    }
    void acknowledge()
    {
        int i = findBestInterrupt();
        if (i == -1) {
            _interrupt = 7;
            return;
        }
        Byte b = 1 << i;
        _isr |= b;
        if (!levelTriggered())
            _irr &= ~b;
    }
    void nonSpecificEOI(bool rotatePriority = false)
    {
        int n = _priority;
        for (int i = 0; i < 8; ++i) {
            Byte b = 1 << n;
            n = (n + 1) & 7;
            if ((_isr & b) != 0 && (_imr & b) == 0) {
                _isr &= ~b;
                if (rotatePriority)
                    _priority = n & 7;
                break;
            }
        }
    }
    void checkICW4Needed()
    {
        if ((_icw1 & 1) != 0)
            _initializationState = initializationStateICW4;
        else
            _initializationState = initializationStateNone;
    }

    enum InitializationState
    {
        initializationStateNone,
        initializationStateICW2,
        initializationStateICW3,
        initializationStateICW4
    };
    bool _interruptPending;
    int _interrupt;
    Byte _irr;
    Byte _imr;
    Byte _isr;
    Byte _icw1;
    Byte _icw2;
    Byte _icw3;
    Byte _icw4;
    Byte _ocw3;
    Byte _lines;
    int _acknowledgedBytes;
    int _priority;
    bool _specialMaskMode;
    bool _rotateInAutomaticEOIMode;
    InitializationState _initializationState;
};

class DMACEmulator
{
public:
    void reset()
    {
        for (int i = 0; i < 4; ++i)
            _channels[i].reset();
        _temporaryAddress = 0;
        _temporaryWordCount = 0;
        _status = 0;
        _command = 0;
        _temporary = 0;
        _mask = 0xf;
        _request = 0;
        _high = false;
        _channel = -1;
        _needHighAddress = true;
    }
    void write(int address, Byte data)
    {
        switch (address) {
            case 0x00: case 0x02: case 0x04: case 0x06:
                _channels[(address & 6) >> 1].setAddress(_high, data);
                _high = !_high;
                break;
            case 0x01: case 0x03: case 0x05: case 0x07:
                _channels[(address & 6) >> 1].setCount(_high, data);
                _high = !_high;
                break;
            case 0x08:  // Write Command Register
                _command = data;
                break;
            case 0x09:  // Write Request Register
                setRequest(data & 3, (data & 4) != 0);
                break;
            case 0x0a:  // Write Single Mask Register Bit
                {
                    Byte b = 1 << (data & 3);
                    if ((data & 4) != 0)
                        _mask |= b;
                    else
                        _mask &= ~b;
                }
                break;
            case 0x0b:  // Write Mode Register
                _channels[data & 3]._mode = data;
                break;
            case 0x0c:  // Clear Byte Pointer Flip/Flop
                _high = false;
                break;
            case 0x0d:  // Master Clear
                reset();
                break;
            case 0x0e:  // Clear Mask Register
                _mask = 0;
                break;
            case 0x0f:  // Write All Mask Register Bits
                _mask = data;
                break;
        }
    }
    Byte read(int address)
    {
        switch (address) {
            case 0x00: case 0x02: case 0x04: case 0x06:
                _high = !_high;
                return _channels[(address & 6) >> 1].getAddress(!_high);
            case 0x01: case 0x03: case 0x05: case 0x07:
                _high = !_high;
                return _channels[(address & 6) >> 1].getCount(!_high);
            case 0x08:  // Read Status Register
                return _status;
                break;
            case 0x0d:  // Read Temporary Register
                return _temporary;
                break;
            default:  // Illegal
                return 0xff;
        }
    }
    void setDMARequestLine(int line, bool state)
    {
        setRequest(line, state != dreqSenseActiveLow());
    }
    bool getHoldRequestLine()
    {
        if (_channel != -1)
            return true;
        if (disabled())
            return false;
        for (int i = 0; i < 4; ++i) {
            int channel = i;
            if (rotatingPriority())
                channel = (channel + _priorityChannel) & 3;
            if ((_request & (1 << channel)) != 0) {
                _channel = channel;
                _priorityChannel = (channel + 1) & 3;
                return true;
            }
        }
        return false;
    }
    void dmaCompleted() { _channel = -1; }
    Byte dmaRead()
    {
        if (memoryToMemory() && _channel == 1)
            return _temporary;
        return 0xff;
    }
    void dmaWrite(Byte data)
    {
        if (memoryToMemory() && _channel == 0)
            _temporary = data;
    }
    Word address()
    {
        Word address = _channels[_channel]._currentAddress;
        _channels[_channel].incrementAddress();
        return address;
    }
    int channel() { return _channel; }
private:
    struct Channel
    {
        void setAddress(bool high, Byte data)
        {
            if (high) {
                _baseAddress = (_baseAddress & 0xff00) + data;
                _currentAddress = (_currentAddress & 0xff00) + data;
            }
            else {
                _baseAddress = (_baseAddress & 0xff) + (data << 8);
                _currentAddress = (_currentAddress & 0xff) + (data << 8);
            }
        }
        void setCount(bool high, Byte data)
        {
            if (high) {
                _baseWordCount = (_baseWordCount & 0xff00) + data;
                _currentWordCount = (_currentWordCount & 0xff00) + data;
            }
            else {
                _baseWordCount = (_baseWordCount & 0xff) + (data << 8);
                _currentWordCount = (_currentWordCount & 0xff) + (data << 8);
            }
        }
        Byte getAddress(bool high)
        {
            if (high)
                return _currentAddress >> 8;
            else
                return _currentAddress & 0xff;
        }
        Byte getCount(bool high)
        {
            if (high)
                return _currentWordCount >> 8;
            else
                return _currentWordCount & 0xff;
        }
        void reset()
        {
            _baseAddress = 0;
            _baseWordCount = 0;
            _currentAddress = 0;
            _currentWordCount = 0;
            _mode = 0;
        }
        void incrementAddress()
        {
            if (!addressDecrement())
                ++_currentAddress;
            else
                --_currentAddress;
            --_currentWordCount;
        }
        bool write() { return (_mode & 0x0c) == 4; }
        bool read() { return (_mode & 0x0c) == 8; }
        bool verify() { return (_mode & 0x0c) == 0; }
        bool autoinitialization() { return (_mode & 0x10) != 0; }
        bool addressDecrement() { return (_mode & 0x20) != 0; }
        bool demand() { return (_mode & 0xc0) == 0x00; }
        bool single() { return (_mode & 0xc0) == 0x40; }
        bool block() { return (_mode & 0xc0) == 0x80; }
        bool cascade() { return (_mode & 0xc0) == 0xc0; }

        Word _baseAddress;
        Word _baseWordCount;
        Word _currentAddress;
        Word _currentWordCount;
        Byte _mode;  // Only 6 bits used
    };

    bool memoryToMemory() { return (_command & 1) != 0; }
    bool channel0AddressHold() { return (_command & 2) != 0; }
    bool disabled() { return (_command & 4) != 0; }
    bool compressedTiming() { return (_command & 8) != 0; }
    bool rotatingPriority() { return (_command & 0x10) != 0; }
    bool extendedWriteSelection() { return (_command & 0x20) != 0; }
    bool dreqSenseActiveLow() { return (_command & 0x40) != 0; }
    bool dackSenseActiveHigh() { return (_command & 0x80) != 0; }
    void setRequest(int line, bool active)
    {
        Byte b = 1 << line;
        Byte s = 0x10 << line;
        if (active) {
            _request |= b;
            _status |= s;
        }
        else {
            _request &= ~b;
            _status &= ~s;
        }
    }

    Channel _channels[4];
    Word _temporaryAddress;
    Word _temporaryWordCount;
    Byte _status;
    Byte _command;
    Byte _temporary;
    Byte _mask;  // Only 4 bits used
    Byte _request;  // Only 4 bits used
    bool _high;
    int _channel;
    int _priorityChannel;
    bool _needHighAddress;
};

// Most of the complexity of the PPI is in the strobed and bidirectional bus
// modes, which aren't actually used in the PC/XT. These modes are implemented
// here for future reference, but are untested.
class PPIEmulator
{
public:
    void reset()
    {
        _mode = 0x99;  // XT normal operation: mode 0, A and C input, B output
        //_mode = 0x9b; // Default: all mode 0, all inputs
        _a = 0;
        _b = 0;
        _c = 0;
        _aLines = 0xff;
        _bLines = 0xff;
        _cLines = 0xff;
    }
    void write(int address, Byte data)
    {
        switch (address) {
            case 0:
                _a = data;
                if (aStrobedOutput())
                    _c &= 0x77;  // Clear -OFB and INTR
                break;
            case 1:
                _b = data;
                if (bStrobedOutput())
                    _c &= 0xfc;  // Clear -OFB and INTR
                break;
            case 2: _c = data; break;
            case 3:
                if ((data & 0x80) != 0) {  // Mode set
                    _mode = data;
                    _a = 0;
                    _b = 0;
                    _c = 0;
                    break;
                }
                if ((data & 1) == 0)  // Port C bit reset
                    _c &= ~(1 << ((data & 0xe) >> 1));
                else
                    _c |= 1 << ((data & 0xe) >> 1);
        }
    }
    Byte read(int address)
    {
        switch (address) {
            case 0:
                if (aStrobedInput())
                    _c &= 0xd7;  // Clear IBF and INTR
                if (aMode() == 0 && aInput())
                    return _aLines;
                return _a;
            case 1:
                if (bMode() != 0)
                    _c &= 0xfc;  // Clear IBF and INTR
                if (bMode() == 0 && bInput())
                    return _bLines;
                return _b;
            case 2:
                {
                    Byte c = _c;
                    if (aMode() == 0) {
                        if (cUpperInput())
                            c = (c & 0x0f) + (_cLines & 0xf0);
                    }
                    else {
                        if (aMode() == 0x20 && cUpperInput()) {
                            if (aInput())
                                c = (c & 0x3f) + (_cLines & 0xc0);
                            else
                                c = (c & 0xcf) + (_cLines & 0x30);
                        }
                    }
                    if (bMode() == 0 && cLowerInput())
                        c = (c & 0xf0) + (_cLines & 0x0f);
                    return c;
                }
        }
        return _mode;
    }
    void setA(int line, bool state)
    {
        if (aStrobedInput() && aStrobe())
            _b = _bLines;
        _aLines = (_aLines & ~(1 << line)) | (state ? (1 << line) : 0);
    }
    void setB(int line, bool state)
    {
        if (bStrobedInput() && bStrobe())
            _b = _bLines;
        _bLines = (_bLines & ~(1 << line)) | (state ? (1 << line) : 0);
    }
    void setC(int line, bool state)
    {
        if (aStrobedInput() && line == 4 && (!state || aStrobe())) {
            _a = _aLines;
            _c |= 0x20;  // Set IBF
            if (aInputInterruptEnable() && state)
                _c |= 8;  // Set INTR on rising edge
        }
        if (aStrobedOutput() && line == 6 && (!state || aAcknowledge())) {
            _c |= 0x80;  // Set -OBF
            if (aOutputInterruptEnable() && state)
                _c |= 8;  // Set INTR on rising edge
        }
        if (bStrobedInput() && line == 2 && (!state || bStrobe())) {
            _b = _bLines;
            _c |= 2;  // Set IBF
            if (bInterruptEnable() && state)
                _c |= 1;  // Set INTR on rising edge
        }
        if (bStrobedOutput() && line == 2 && (!state || bStrobe())) {
            _c |= 2;  // Set -OBF
            if (bInterruptEnable() && state)
                _c |= 1;  // Set INTR on rising edge
        }
        _cLines = (_cLines & ~(1 << line)) | (state ? (1 << line) : 0);
    }
    bool getA(int line)
    {
        Byte m = 1 << line;
        if (aMode() == 0) {
            if (aInput())
                return (_aLines & m) != 0;
            return (_a & _aLines & m) != 0;
        }
        return (_a & m) != 0;
    }
    bool getB(int line)
    {
        Byte m = 1 << line;
        if (bMode() == 0) {
            if (bInput())
                return (_bLines & m) != 0;
            return (_b & _bLines & m) != 0;
        }
        return (_b & m) != 0;
    }
    bool getC(int line)
    {
        // 0 bit means output enabled, so _c bit low drives output low
        // 1 bit means tristate from PPI so return _cLine bit.
        static const Byte m[] = {
            0x00, 0x0f, 0x00, 0x0f, 0x04, 0x0c, 0x04, 0x0c,  // A mode 0 
            0xf0, 0xff, 0xf0, 0xff, 0xf4, 0xfc, 0xf4, 0xfc,
            0x00, 0x0f, 0x00, 0x0f, 0x04, 0x0c, 0x04, 0x0c,
            0xf0, 0xff, 0xf0, 0xff, 0xf4, 0xfc, 0xf4, 0xfc,

            0x40, 0x47, 0x40, 0x47, 0x44, 0x44, 0x44, 0x44,  // A mode 1 output
            0x70, 0x77, 0x70, 0x77, 0x74, 0x74, 0x74, 0x74,
            0x10, 0x17, 0x10, 0x17, 0x14, 0x14, 0x14, 0x14,  // A mode 1 input
            0xd0, 0xd7, 0xd0, 0xd7, 0xd4, 0xd4, 0xd4, 0xd4,

            0x50, 0x57, 0x50, 0x57, 0x54, 0x54, 0x54, 0x54,  // A mode 2
            0x50, 0x57, 0x50, 0x57, 0x54, 0x54, 0x54, 0x54,
            0x50, 0x57, 0x50, 0x57, 0x54, 0x54, 0x54, 0x54,
            0x50, 0x57, 0x50, 0x57, 0x54, 0x54, 0x54, 0x54,

            0x50, 0x57, 0x50, 0x57, 0x54, 0x54, 0x54, 0x54,  // A mode 2
            0x50, 0x57, 0x50, 0x57, 0x54, 0x54, 0x54, 0x54,
            0x50, 0x57, 0x50, 0x57, 0x54, 0x54, 0x54, 0x54,
            0x50, 0x57, 0x50, 0x57, 0x54, 0x54, 0x54, 0x54,
        };                            
        return (_cLines & (_c | m[_mode & 0x7f]) & (1 << line)) != 0; 
    }
private:
    Byte aMode() { return _mode & 0x60; }
    Byte bMode() { return _mode & 4; }
    bool aInput() { return (_mode & 0x10) != 0; }
    bool cUpperInput() { return (_mode & 8) != 0; }
    bool bInput() { return (_mode & 2) != 0; }
    bool cLowerInput() { return (_mode & 1) != 0; }
    bool aStrobe() { return (_cLines & 0x10) == 0; }
    bool bStrobe() { return (_cLines & 4) == 0; }
    bool aAcknowledge() { return (_cLines & 0x40) == 0; }
    bool bAcknowledge() { return (_cLines & 4) == 0; }
    bool aStrobedInput()
    {
        return (aMode() == 0x20 && aInput()) || aMode() == 0x40;
    }
    bool aStrobedOutput()
    {
        return (aMode() == 0x20 && !aInput()) || aMode() == 0x40;
    }
    bool bStrobedInput() { return bMode() != 0 && bInput(); }
    bool bStrobedOutput() { return bMode() != 0 && !bInput(); }
    bool aInputInterruptEnable() { return (_c & 0x10) != 0; }
    bool aOutputInterruptEnable() { return (_c & 0x40) != 0; }
    bool bInterruptEnable() { return (_c & 4) != 0; }

    Byte _a;
    Byte _b;
    Byte _c;
    Byte _aLines;
    Byte _bLines;
    Byte _cLines;
    Byte _mode;
};

class BusEmulator
{
public:
    BusEmulator() : _ram(0xa0000), _rom(0x8000)
    {
        File("Q:\\external\\8088\\roms\\ibm5160\\1501512.u18", true).
            openRead().read(&_rom[0], 0x8000);
        _pit.setGate(0, true);
        _pit.setGate(1, true);
        _pit.setGate(2, true);
    }
    Byte* ram() { return &_ram[0]; }
    void reset()
    {
        _dmac.reset();
        _pic.reset();
        _pit.reset();
        _ppi.reset();
        _pitPhase = 2;
        _lastCounter0Output = false;
        _lastCounter1Output = true;
        _counter2Output = false;
        _counter2Gate = false;
        _speakerMask = false;
        _speakerOutput = false;
        _dmaState = sIdle;
        _passiveOrHalt = true;
        _lock = false;
        _previousLock = false;
        _previousPassiveOrHalt = true;
        _lastNonDMAReady = true;
        _cgaPhase = 0;
    }
    void stubInit()
    {
        _pic.stubInit();
        _pit.stubInit();
        _pitPhase = 2;
        _lastCounter0Output = true;
    }
    void startAccess(DWord address, int type)
    {
        _address = address;
        _type = type;
        _cycle = 0;
    }
    void wait()
    {
        _cgaPhase = (_cgaPhase + 3) & 0x0f;
        ++_pitPhase;
        if (_pitPhase == 4) {
            _pitPhase = 0;
            _pit.wait();
            bool counter0Output = _pit.getOutput(0);
            if (_lastCounter0Output != counter0Output)
                _pic.setIRQLine(0, counter0Output);
            _lastCounter0Output = counter0Output;
            bool counter1Output = _pit.getOutput(1);
            if (counter1Output && !_lastCounter1Output && !dack0()) {
                _dmaRequests |= 1;
                _dmac.setDMARequestLine(0, true);
            }
            _lastCounter1Output = counter1Output;
            bool counter2Output = _pit.getOutput(2);
            if (_counter2Output != counter2Output) {
                _counter2Output = counter2Output;
                setSpeakerOutput();
                _ppi.setC(5, counter2Output);
                updatePPI();
            }
        }
        if (_speakerCycle != 0) {
            --_speakerCycle;
            if (_speakerCycle == 0) {
                _speakerOutput = _nextSpeakerOutput;
                _ppi.setC(4, _speakerOutput);
                updatePPI();
            }
        }

        // Set to false to implement 5160s without the U90 fix and 5150s
        // without the U101 fix as described in
        // http://www.vcfed.org/forum/showthread.php?29211-Purpose-of-U90-in-XT-second-revision-board
        bool hasDMACFix = true;

        if (_type != 2 || (_address & 0x3e0) != 0x000 || !hasDMACFix)
            _lastNonDMAReady = nonDMAReady();
        //if (_previousLock && !_lock)
        //    _previousLock = false;
        //_previousLock = _lock;
        switch (_dmaState) {
            case sIdle:
                if (_dmac.getHoldRequestLine())
                    _dmaState = sDREQ;
                break;
            case sDREQ:
                _dmaState = sHRQ; //(_passiveOrHalt && !_previousLock) ? sHRQ : sHoldWait;
                break;
            case sHRQ:
                //_dmaState = _lastNonDMAReady ? sAEN : sPreAEN;
                if ((_passiveOrHalt || _previousPassiveOrHalt) && !_previousLock && _lastNonDMAReady)
                    _dmaState = sAEN;
                break;
            //case sHoldWait:
            //    if (_passiveOrHalt && !_previousLock)
            //        _dmaState = _lastNonDMAReady ? sAEN : sPreAEN;
            //    break;
            //case sPreAEN:
            //    if (_lastNonDMAReady)
            //        _dmaState = sAEN;
            //    break;
            case sAEN: _dmaState = s0; break;
            case s0:
                if ((_dmaRequests & 1) != 0) {
                    _dmaRequests &= 0xfe;
                    _dmac.setDMARequestLine(0, false);
                }
                _dmaState = s1; break;
            case s1: _dmaState = s2; break;
            case s2: _dmaState = s3; break;
            case s3: _dmaState = s4; break;
            case s4: _dmaState = sDelayedT1; _dmac.dmaCompleted(); break;
            case sDelayedT1: _dmaState = sDelayedT2; break;
            case sDelayedT2: _dmaState = sDelayedT3; break;
            case sDelayedT3: _dmaState = sIdle; break;
        }
        _previousLock = _lock;
        _previousPassiveOrHalt = _passiveOrHalt;

        _lastNonDMAReady = nonDMAReady();
    }
    bool ready()
    {
        if (_dmaState == s1 || _dmaState == s2 || _dmaState == s3 ||
            _dmaState == sWait || _dmaState == s4 || _dmaState == sDelayedT1 ||
            _dmaState == sDelayedT2 /*|| _dmaState == sDelayedT3*/)
            return false;
        ++_cycle;
        return nonDMAReady();
    }
    void write(Byte data)
    {
        if (_type == 2) {
            switch (_address & 0x3e0) {
                case 0x00:
                    _dmac.write(_address & 0x0f, data);
                    break;
                case 0x20:
                    _pic.write(_address & 1, data);
                    break;
                case 0x40:
                    _pit.write(_address & 3, data);
                    break;
                case 0x60:
                    _ppi.write(_address & 3, data);
                    updatePPI();
                    break;
                case 0x80:
                    _dmaPages[_address & 3] = data;
                    break;
                case 0xa0:
                    _nmiEnabled = (data & 0x80) != 0;
                    break;
            }
        }
        else
            if (_address < 0xa0000)
                _ram[_address] = data;
    }
    Byte read()
    {
        if (_type == 0) // Interrupt acknowledge
            return _pic.interruptAcknowledge();
        if (_type == 1) { // Read port
            switch (_address & 0x3e0) {
                case 0x00: return _dmac.read(_address & 0x0f);
                case 0x20: return _pic.read(_address & 1);
                case 0x40: return _pit.read(_address & 3);
                case 0x60:
                    {
                        Byte b = _ppi.read(_address & 3);
                        updatePPI();
                        return b;
                    }
                    
            }
            return 0xff;
        }
        if (_address >= 0xf8000)
            return _rom[_address - 0xf8000];
        if (_address >= 0xa0000)
            return 0xff;
        return _ram[_address];
    }
    bool interruptPending() { return _pic.interruptPending(); }
    int pitBits()
    {
        return (_pitPhase == 1 || _pitPhase == 2 ? 1 : 0) +
            (_counter2Gate ? 2 : 0) + (_pit.getOutput(2) ? 4 : 0);
    }
    void setPassiveOrHalt(bool v) { _passiveOrHalt = v; }
    bool getAEN()
    {
        return _dmaState == sAEN || _dmaState == s0 || _dmaState == s1 ||
            _dmaState == s2 || _dmaState == s3 || _dmaState == sWait ||
            _dmaState == s4;
    }
    UInt8 getDMA()
    {
        return _dmaRequests | (dack0() ? 0x10 : 0);
    }
    String snifferExtra()
    {
        return ""; //hex(_pit.getMode(1), 4, false) + " ";
    }
    int getBusOperation()
    {
        switch (_dmaState) {
            case s2: return 5;  // memr
            case s3: return 2;  // iow
        }
        return 0;
    }
    bool getDMAS3() { return _dmaState == s3; }
    DWord getDMAAddress()
    {
        return dmaAddressHigh(_dmac.channel()) + _dmac.address();
    }
    void setLock(bool lock) { _lock = lock; }
    UInt8 getIRQLines() { return _pic.getIRQLines(); }
    UInt8 getDMAS()
    {
        if (_dmaState == sAEN || _dmaState == s0 || _dmaState == s1 ||
            _dmaState == s2 || _dmaState == s3 || _dmaState == sWait)
            return 3;
        if (_dmaState == sHRQ || _dmaState == sHoldWait ||
            _dmaState == sPreAEN)
            return 1;
        return 0;
    }
    UInt8 getCGA()
    {
        return _cgaPhase >> 2;
    }
private:
    bool nonDMAReady()
    {
        if (_type == 1 || _type == 2)  // Read port, write port
            return _cycle > 1;  // System board adds a wait state for onboard IO devices
        return true;
    }
    bool dack0()
    {
        return _dmaState == s1 || _dmaState == s2 || _dmaState == s3 ||
            _dmaState == sWait;
    }
    void setSpeakerOutput()
    {
        bool o = !(_counter2Output && _speakerMask);
        if (_nextSpeakerOutput != o) {
            if (_speakerOutput == o)
                _speakerCycle = 0;
            else
                _speakerCycle = o ? 3 : 2;
            _nextSpeakerOutput = o;
        }
    }
    void updatePPI()
    {
        bool speakerMask = _ppi.getB(1);
        if (speakerMask != _speakerMask) {
            _speakerMask = speakerMask;
            setSpeakerOutput();
        }
        _counter2Gate = _ppi.getB(0);
        _pit.setGate(2, _counter2Gate);
    }
    DWord dmaAddressHigh(int channel)
    {
        static const int pageRegister[4] = {0x83, 0x83, 0x81, 0x82};
        return _dmaPages[pageRegister[channel]] << 16;
    }

    enum DMAState
    {
        sIdle,
        sDREQ,
        sHRQ,
        sHoldWait,
        sPreAEN,
        sAEN,
        s0,
        s1,
        s2,
        s3,
        sWait,
        s4,
        sDelayedT1,
        sDelayedT2,
        sDelayedT3
    };

    Array<Byte> _ram;
    Array<Byte> _rom;
    DWord _address;
    int _type;
    int _cycle;
    DMACEmulator _dmac;
    PICEmulator _pic;
    PITEmulator _pit;
    PPIEmulator _ppi;
    int _pitPhase;
    bool _lastCounter0Output;
    bool _lastCounter1Output;
    bool _counter2Output;
    bool _counter2Gate;
    bool _speakerMask;
    bool _speakerOutput;
    bool _nextSpeakerOutput;
    Word _dmaAddress;
    int _dmaCycles;
    int _dmaType;
    int _speakerCycle;
    Byte _dmaPages[4];
    bool _nmiEnabled;
    bool _passiveOrHalt;
    DMAState _dmaState;
    Byte _dmaRequests;
    bool _lock;
    bool _previousLock;
    bool _previousPassiveOrHalt;
    bool _lastNonDMAReady;
    Byte _cgaPhase;
};

class CPUEmulator
{
public:
    CPUEmulator() : _logging(false)
    {
        ax() = 0x100;
        Byte* byteData = (Byte*)(&ax());
        int bigEndian = *byteData;
        int byteNumbers[8] = {0, 2, 4, 6, 1, 3, 5, 7};
        for (int i = 0 ; i < 8; ++i)
            _byteRegisters[i] = &byteData[byteNumbers[i] ^ bigEndian];
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
        _logging = false;
        _test = test;
        try {
            run();
        } catch (...) { }
        return _cycle;
    }
    int instructionCycles() { return _cycle2 - _cycle1; }
    void setStub(Array<Byte> stub) { _stub = stub; }
private:
    Word getRealIP()
    {
        Word r = _ip - _queueBytes;
        if (_busState == t4StatusSet || _busState == tIdleStatusSet)
            --r;
        if ((_busState == t1 || _busState == t2t3tWaitNotLast ||
            _busState == t3tWaitLast || _busState == t4 ||
            _busState == t4StatusSet) && _io._type == ioCodeAccess)
            --r;
        return r;
    }
    void run()
    {
        _bus.reset();

        _cycle = 0;
        ax() = 0;
        cx() = 0;
        dx() = _test.refreshPeriod() + (_test.refreshPhase() << 8);
        bx() = 0;
        sp() = 0;
        bp() = 0;
        si() = 0;
        di() = 0;
        es() = testSegment;
        cs() = testSegment;
        ss() = testSegment;
        ds() = testSegment;
        _flags = 0;
        Word seg = testSegment + 0x1000;

        if (_test.refreshPeriod() == 0) {
            _bus.stubInit();
            Byte* ram = _bus.ram();
            ram[3*4 + 0] = 0x00;  // int 3 handler at 0x400
            ram[3*4 + 1] = 0x04;
            ram[3*4 + 2] = 0x00;
            ram[3*4 + 3] = 0x00;
            ram[0x400] = 0x83;
            ram[0x401] = 0xc4;
            ram[0x402] = 0x04;  // ADD SP,+4
            ram[0x403] = 0x9d;  // POPF
            ram[0x404] = 0xcb;  // RETF

            Byte* r = _bus.ram() + (seg << 4);
            Byte* stopP = _test.outputCode(r);
            _stopIP = stopP - (r + 2);
            _logSkip = 1;
            es() = seg;
            cs() = seg;
            ss() = seg;
            ds() = seg;
        }
        else {
            Byte* ram = _bus.ram() + (testSegment << 4);
            for (int i = 0; i < _stub.count(); ++i)
                ram[i] = _stub[i];
            _stopIP = 0xd1;
            Byte* r = _bus.ram() + (seg << 4);
            Byte* stopP = _test.outputCode(r);
            _logSkip = 1041 + 92;// + 17;
            seg = testSegment;
        }

        _timeIP1 = _test.startIP();

        _busState = tIdle;
        _io._type = ioPassive;
        _ioNext = _io;
        _ioLast = _io;
        _snifferDecoder.reset();
        _prefetchedRemove = false;
        _delayedPrefetchedRemove = false;
        _prefetching = true;
        _transferStarting = false;
        _log = "";
        _ip = 0;
        _nmiRequested = false;
        _queueReadPosition = 0;
        _queueWritePosition = 0;
        _queueBytes = 0;
        _queueEmptied = false;
        _queueHasSpace = true;
        _queueHasByte = false;
        _segmentOverride = -1;
        _forcedSegment = -1;
        _rep = 0;
        _lock = false;
        _completed = true;
        _repeating = false;
        _clearLock = false;
        _ioToFillQueue = false;
        _ioToFillQueuePopulated = false;
        _ioToFillQueuePopulated1 = false;

        do {
            executeOneInstruction();
        } while ((getRealIP() != _stopIP || cs() != seg) && _cycle < 4096 /* 2048*/);
        _cycle2 = _cycle;
    }
    void wait(int cycles)
    {
        while (cycles > 0) {
            BusState nextState = _busState;

            bool queueEmptied = _queueEmptied;
            _queueEmptied = false;
            if (_prefetchedRemove) {
                --_queueBytes;
                _queueHasSpace = true;
                _prefetchedRemove = false;
                if (_queueBytes == 0)
                    _queueEmptied = true;
            }
            if (_delayedPrefetchedRemove) {
                _prefetchedRemove = true;
                _delayedPrefetchedRemove = false;
                if (_queueBytes == 1)
                    _queueHasByte = false;
            }

            bool write = _io._type == ioWriteMemory ||
                _io._type == ioWritePort;
            _bus.wait();
            bool ready = true;
            switch (_busState) {
                case t1:
                    _snifferDecoder.setStatusHigh(_io._segment);
                    _snifferDecoder.setBusOperation((int)_io._type);
                    if (write)
                        _snifferDecoder.setData(_io._data);
                    nextState = t2t3tWaitNotLast;
                    _ioWasDelayed = false;
                    _ioToFillQueuePopulated1 = false;
                    _ioToFillQueuePopulated = false;
                    _ioToFillQueue = false;
                    break;
                case t2t3tWaitNotLast:
                    if (!_ioToFillQueuePopulated && _ioToFillQueuePopulated1) {
                        _ioToFillQueue =  (_queueBytes == 3 && _io._type == ioCodeAccess);
                        _ioToFillQueuePopulated = true;
                    }
                    if (!_ioToFillQueuePopulated1)
                        _ioToFillQueuePopulated1 = true;
                    ready = _bus.ready();
                    if (!ready) {
                        _ioWasDelayed = true;
                        break;
                    }
                    nextState = t3tWaitLast;
                    if (_io._type == ioInterruptAcknowledge ||
                        _io._type == ioReadPort || _io._type == ioReadMemory ||
                        _io._type == ioCodeAccess) {
                        Byte data = _bus.read();
                        _io._data = data;
                        _snifferDecoder.setData(data);
                    }
                    _bus.setPassiveOrHalt(true);
                    _snifferDecoder.setStatus((int)ioPassive);
                    break;
                case t3tWaitLast:
                    nextState = t4;
                    if (write)
                        _bus.write(_io._data);
                    break;
                case t4:
                case t4StatusSet:
                    if (_io._type == ioCodeAccess) {
                        _queueData[_queueWritePosition] = _io._data;
                        _queueWritePosition = (_queueWritePosition + 1) & 3;
                        _queueHasByte = true;
                        ++_queueBytes;
                    }
                    _ioLast = _io;
                    _io._type = ioPassive;
                    nextState = tFirstIdle;
                    break;
                case tFirstIdle:
                    nextState = tSecondIdle;
                    _ioWasDelayed = false;
                    break;
                case tSecondIdle:
                    nextState = tIdle;
                    break;
            }
            if ((nextState == t4 && (!_ioToFillQueue || _queueBytes > 2 /*|| !queueEmptied*/ /*_queueBytes != 0*/ /*(!_ioWasDelayed || _io._type != ioCodeAccess)*/)) || nextState == tSecondIdle || nextState == tIdle) {
                int adjustedQueueBytes = _queueBytes + (nextState == t4 && _io._type == ioCodeAccess ? 1 : 0);
                if (_ioNext._type == ioPassive && adjustedQueueBytes != 4 &&
                    _prefetching && !_transferStarting && (nextState != tSecondIdle || _ioLast._type != ioCodeAccess || adjustedQueueBytes != 3)) {
                    _ioNext._type = ioCodeAccess;
                    _ioNext._address = physicalAddress(1, _ip);
                    ++_ip;
                    _ioNext._segment = 1;
                }
                if (_ioNext._type != ioPassive) {
                    _bus.setPassiveOrHalt(_ioNext._type == ioHalt);
                    _snifferDecoder.setStatus((int)_ioNext._type);
                    if (nextState == t4)
                        nextState = t4StatusSet;
                    else
                        nextState = tIdleStatusSet;
                }
            }
            if (_busState == t4StatusSet || _busState == tIdleStatusSet) {
                _io = _ioNext;
                _ioNext._type = ioPassive;
                if (_io._type != ioPassive) {
                    nextState = t1;
                    _snifferDecoder.setAddress(_io._address);
                    _bus.startAccess(_io._address,
                        (int)_io._type);
                }
            }
            if (_logging) {
                _snifferDecoder.setAEN(_bus.getAEN());
                _snifferDecoder.setDMA(_bus.getDMA());
                _snifferDecoder.setPITBits(_bus.pitBits());
                _snifferDecoder.setBusOperation(_bus.getBusOperation());
                _snifferDecoder.setInterruptFlag(intf());
                if (_bus.getDMAS3())
                    _snifferDecoder.setAddress(_bus.getDMAAddress());
                _snifferDecoder.setReady(ready);
                _snifferDecoder.setLock(_lock);
                _snifferDecoder.setDMAS(_bus.getDMAS());
                _snifferDecoder.setIRQs(_bus.getIRQLines());
                _snifferDecoder.setINT(_bus.interruptPending());
                _snifferDecoder.setCGA(_bus.getCGA());
                String l = _bus.snifferExtra() + _snifferDecoder.getLine();
                if (_logSkip > 0)
                    --_logSkip;
                else
                    _log += l;
            }

            _busState = nextState;
            ++_cycle;
            --cycles;
        }
    }
    enum IOType
    {
        ioInterruptAcknowledge = 0,
        ioReadPort = 1,
        ioWritePort = 2,
        ioHalt = 3,
        ioCodeAccess = 4,
        ioReadMemory = 5,
        ioWriteMemory = 6,
        ioPassive = 7
    };
    void initIO(IOType type, DWord address)
    {
        if (_forcedSegment != -1)
            _ioNext._segment = _forcedSegment;
        else {
            _ioNext._segment = _segment;
            if (_segmentOverride != -1)
                _ioNext._segment = _segmentOverride;
        }
        _ioNext._address = physicalAddress(_ioNext._segment, address);
        _ioNext._data = static_cast<Byte>(_data);
        _ioNext._type = type;
    }
    void busAccess(IOType type, Word address)
    {
        while (_ioNext._type != ioPassive)
            wait(1);
        _transferStarting = false;
        initIO(type, address);
        do {
            wait(1);
        } while (_ioNext._type != ioPassive);
    }
    void waitForBusIdle()
    {
        while (_busState == t2t3tWaitNotLast || _busState == t1 ||
            _busState == tIdleStatusSet || _busState == t4StatusSet)
            wait(1);
    }
    void busInit()
    {
        switch (_accessNumber) {
            case 0:
                printf("Error: no access number!\n");
                break;
            case 1:
            case 6:
                _transferStarting = true;
                printBusState();
                if (_busState == t4StatusSet || _busState == t1 || _busState == tIdleStatusSet) {
                }
                else {
                    waitForBusIdle();
                    wait(1);
                }
                wait(1);
                break;
            case 2:
                wait(1);
                _transferStarting = true;
                if (_busState == t4StatusSet || _busState == t1 || _busState == tIdleStatusSet || _busState == t3tWaitLast) {
                }
                else {
                    waitForBusIdle();
                    wait(1);
                }
                wait(1);
                break;
            case 3:
                _transferStarting = true;
                if (_busState == t4StatusSet || _busState == t1 || _busState == tIdleStatusSet || _busState == tFirstIdle) {
                    if (!(_busState == t1 || (_busState == tFirstIdle && _ioLast._type != ioCodeAccess)))
                        wait(1);
                }
                else {
                    waitForBusIdle();
                }
                wait(2);
                break;
            case 4:
                wait(3);
                _transferStarting = true;
                if (_busState == t4StatusSet || _busState == t1 || _busState == tIdleStatusSet) {
                }
                else {
                    waitForBusIdle();
                }
                wait(2);
                break;
            case 5:
                {
                    if (_opcode == 0xcc) {
                        bool codeLast = _io._type == ioCodeAccess;
                        wait(5);
                        if (_busState == t3tWaitLast || _busState == t4StatusSet || _busState == tIdleStatusSet || _busState == t4 || _busState == tFirstIdle) {
                            if (_busState == t4 && codeLast)
                                wait(1);
                            wait(1);
                            _transferStarting = true;
                        }
                        else {
                            _transferStarting = true;
                            waitForBusIdle();
                        }
                        wait(2);
                    }
                    else {
                        wait(2);
                        _transferStarting = true;
                        if (_busState == t3tWaitLast || _busState == t4StatusSet || _busState == tIdleStatusSet  || _busState == t1 || _busState == t4) {
                            if (_busState == t4 && _opcode == 0xcd)
                                wait(1);
                            wait(1);
                        }
                        else {
                            wait(1);
                            waitForBusIdle();
                        }
                        wait(1);
                    }
                }
                break;
            case 7:
                // Be careful changing this one, it's used in the stub. If the
                // stub gets desynchronized, set logskip = 0 here and first
                // patchSnifferInitialWait patch to 6 in runtests.asm.
                wait(1);
                _transferStarting = true;
                if (_busState == t4StatusSet || _busState == t1 || _busState == tIdleStatusSet) {
                }
                else {
                    waitForBusIdle();
                    wait(2);
                }
                break;
            case 8:
                if (_busState == t4StatusSet || _busState == t3tWaitLast || _busState == tSecondIdle || _busState == tIdleStatusSet) {
                    wait(1);
                    _transferStarting = true;
                    wait(1);
                }
                else {
                    _transferStarting = true;
                    waitForBusIdle();
                }
                wait(1);
                break;
            case 9:
                _transferStarting = true;
                if (_busState == t4StatusSet || _busState == t1 || _busState == tSecondIdle || _busState == tIdleStatusSet || _busState == tFirstIdle) {
                    if (_busState == tFirstIdle)
                        wait(1);
                    wait(1);
                }
                else {
                    waitForBusIdle();
                }
                wait(1);
                break;
            case 10:
                wait(1);
                if (_busState == t3tWaitLast || _busState == t4StatusSet || _busState == tIdleStatusSet  || _busState == t1 || _busState == t4) {
                    wait(1);
                    _transferStarting = true;
                }
                else {
                    _transferStarting = true;
                    wait(1);
                    waitForBusIdle();
                }
                wait(2);
                break;
            case 11:
                wait(1);
                _transferStarting = true;
                if (_busState == t1 || _busState == t3tWaitLast || _busState == t4StatusSet || _busState == tIdleStatusSet) {
                }
                else {
                    wait(1);
                    waitForBusIdle();
                }
                wait(1);
                break;
            case 12:
            case 13:
                wait(3);
                _transferStarting = true;
                if (_busState == t1 || _busState == t3tWaitLast || _busState == t4StatusSet || _busState == tIdleStatusSet) {
                }
                else {
                    wait(1);
                    waitForBusIdle();
                }
                wait(1);
                break;
            case 14:
                // Used by stub
                wait(3);
                _transferStarting = true;
                {
                    bool codeLast = _ioLast._type == ioCodeAccess;
                    if (_busState == t1 || _busState == t3tWaitLast || _busState == t4 || _busState == t4StatusSet || _busState == tIdleStatusSet) {
                        if (_busState == t4) {
                            if (codeLast)
                                wait(1);
                            else
                                wait(2);
                        }
                    }
                    else {
                        wait(1);
                        waitForBusIdle();
                    }
                    wait(1);
                }
                break;
            case 15:
                wait(1);
                _transferStarting = true;
                if (_busState == t1 || _busState == t3tWaitLast || _busState == t4StatusSet || _busState == tIdleStatusSet) {
                }
                else {
                    wait(1);
                    waitForBusIdle();
                }
                wait(1);
                break;
            case 16:
                if (!_wordSize) {
                    if (_busState == t4)
                        wait(1);
                }
                wait(1);
                _transferStarting = true;
                if (_busState == t1 || _busState == t3tWaitLast || _busState == t4StatusSet || _busState == tIdleStatusSet || _busState == tSecondIdle) {
                    wait(1);
                }
                else {
                    wait(1);
                    waitForBusIdle();
                }
                wait(1);
                break;
            case 17:
                _transferStarting = true;
                if (_busState == t1 || _busState == t3tWaitLast || _busState == t4StatusSet || _busState == tIdleStatusSet) {
                }
                else {
                    wait(1);
                    waitForBusIdle();
                }
                wait(1);
                break;
            case 18:
            case 19:
                wait(2);
                _transferStarting = true;
                if (_busState == t1 || _busState == t3tWaitLast || _busState == t4StatusSet || _busState == tIdleStatusSet) {
                }
                else {
                    wait(1);
                    waitForBusIdle();
                }
                wait(1);
                break;
            case 20:
            case 21:
            case 24:
                _transferStarting = true;
                if (_busState == t4StatusSet || _busState == t1 || _busState == tIdleStatusSet) {
                }
                else {
                    waitForBusIdle();
                    wait(1);
                }
                wait(1);
                break;
            case 22:
            case 23:
                _transferStarting = true;
                if (_segmentOverride != -1 || _busState == t4 || _busState == t4StatusSet || _busState == tIdle || _busState == t1 || _busState == t3tWaitLast || (_busState == tFirstIdle && _ioLast._type != ioCodeAccess)) {
                }
                else {
                    wait(1);
                    waitForBusIdle();
                }
                wait(2);
                break;
            case 25:
                _transferStarting = true;  // This version seems to be particularly refined - try replacing the others with this one
                if (_segmentOverride != -1 || _busState == t4 || _busState == t4StatusSet || _busState == tIdle || _busState == t1 || _busState == t3tWaitLast || (_busState == tFirstIdle && _ioLast._type != ioCodeAccess)) {
                    _segmentOverride = -1;
                }
                else {
                    wait(1);
                    waitForBusIdle();
                }
                wait(2);
                break;
            case 26:
                _transferStarting = true;
                if (_busState == tFirstIdle && _ioLast._type == ioCodeAccess)
                    wait(1);
                if (_busState == t4StatusSet || _busState == t1 || _busState == tIdleStatusSet) {
                }
                else {
                    waitForBusIdle();
                }
                wait(2);
                break;
            case 27:
            case 32:
            case 37:
                wait(1);
                _transferStarting = true;
                wait(2);
                break;
            case 28:
                _transferStarting = true;
                if (_busState == t1 || _busState == t3tWaitLast || _busState == t4StatusSet || _busState == tIdleStatusSet) {
                }
                else {
                    wait(1);
                    waitForBusIdle();
                }
                wait(1);
                break;
            case 29:
            case 30:
                wait(3);
                _transferStarting = true;
                if (_busState == t1 || _busState == t3tWaitLast || _busState == t4StatusSet || _busState == tIdleStatusSet) {
                }
                else {
                    wait(1);
                    waitForBusIdle();
                }
                wait(1);
                break;
            case 31:
                wait(1);
                _transferStarting = true;
                if (_busState == t1 || _busState == t3tWaitLast || _busState == t4StatusSet || _busState == tIdleStatusSet) {
                }
                else {
                    wait(1);
                    waitForBusIdle();
                }
                wait(5);
                break;
            case 33:
                wait(3);
                _transferStarting = true;
                if (_busState == t1 || _busState == t3tWaitLast || _busState == t4StatusSet || _busState == tIdleStatusSet) {
                }
                else {
                    wait(1);
                    waitForBusIdle();
                }
                wait(1);
                break;
            case 34:
            case 39:
            case 41:
                wait(2);
                _transferStarting = true;
                wait(2);
                break;
            case 35:
                _transferStarting = true;
                wait(2);
                break;
            case 36:
                wait(1);
                _prefetching = false;
                wait(1);
                if (_useMemory)
                    wait(1);
                while ((_busState != tIdle && _busState != tSecondIdle && _busState != tFirstIdle && _busState != tIdleStatusSet) || _ioNext._type != ioPassive)  // Weird
                    wait(1);
                wait(1);
                _transferStarting = true;
                wait(2);
                break;
            case 38:
                wait(4);
                _transferStarting = true;
                printBusState();
                if (_busState == t1 || _busState == t3tWaitLast || _busState == t4StatusSet || _busState == t4 || _busState == tIdleStatusSet) {
                    if (_busState == t4 && _ioLast._type == ioCodeAccess)
                        wait(1);
                    wait(1);
                }
                else {
                    wait(1);
                    waitForBusIdle();
                }
                wait(1);
                break;
            case 40:
                wait(4);
                _transferStarting = true;
                wait(2);
                break;
            case 42:
                wait(1);
                _transferStarting = true;
                if (_busState == tFirstIdle && _ioLast._type == ioCodeAccess)
                    wait(1);
                wait(2);
                break;
            case 43:
                wait(2);
                _prefetching = false;
                _transferStarting = true;
                if (_busState == t1 || _busState == t3tWaitLast || _busState == t4StatusSet || _busState == tIdleStatusSet) {
                }
                else {
                    wait(1);
                    waitForBusIdle();
                }
                wait(1);
                break;
            case 44:
            case 45:
                _transferStarting = true;
                if (_busState == tFirstIdle && _ioLast._type == ioCodeAccess)
                    wait(1);
                wait(2);
                break;
            case 46:
                _transferStarting = true;  // This version seems to be particularly refined - try replacing the others with this one
                if (_busState == t4 || _busState == t4StatusSet || _busState == tIdle || _busState == tIdleStatusSet || _busState == t1 || _busState == t3tWaitLast || _busState == tFirstIdle) {
                    if (_busState == t4)
                        wait(1);
                }
                else {
                    wait(1);
                    waitForBusIdle();
                }
                wait(2);
                break;
            case 47:
            case 48:
            case 49:
            case 50:
            case 51:
                _transferStarting = true;
                if (_busState == t4 || _busState == t4StatusSet || _busState == t1 || _busState == tIdleStatusSet || _busState == tFirstIdle) {
                    if (_busState == t4)
                        wait(1);
                    wait(1);
                }
                else {
                    waitForBusIdle();
                }
                wait(1);
                break;
            case 52:
                _transferStarting = true;
                if (_busState == t4StatusSet || _busState == t1 || _busState == tIdleStatusSet || _busState == t4) {
                    if (_busState == t4)
                        wait(1);
                }
                else {
                    waitForBusIdle();
                }
                wait(2);
                break;
            case 53:
                _transferStarting = true;
                if (_busState == t1 || _busState == t3tWaitLast ||  _busState == t4 || _busState == t4StatusSet || _busState == tIdleStatusSet || _busState == tFirstIdle) {
                    if (_busState == t4)
                        wait(1);
                    wait(1);
                }
                else {
                    wait(2);
                    waitForBusIdle();
                }
                wait(1);
                break;
            case 54:
                _transferStarting = true;
                if (_busState == t4StatusSet || _busState == t1 || _busState == tIdleStatusSet || _busState == t4) {
                    if (_busState == t4)
                        wait(1);
                }
                else {
                    waitForBusIdle();
                }
                wait(2);
                break;
            case 55:
                _transferStarting = true;
                if (_busState == t4StatusSet || _busState == t1 || _busState == t4 || _busState == tFirstIdle || _busState == tIdleStatusSet) {
                    if (_busState == t1 || (_busState == tFirstIdle && _ioLast._type == ioCodeAccess))
                        wait(1);
                    else
                        wait(2);
                }
                else {
                    waitForBusIdle();
                }
                wait(1);
                break;
            case 56:
                _transferStarting = true;
                if (_busState == t4StatusSet || _busState == t1 || _busState == tFirstIdle || _busState == tIdleStatusSet || _busState == t4) {
                    if (_busState != tFirstIdle && _busState != t1)
                        wait(1);
                    wait(1);
                }
                else {
                    waitForBusIdle();
                }
                wait(1);
                break;
            case 57:
                if (_useMemory)
                    wait(2);
                wait(2);
                _transferStarting = true;
                if (_busState == t4StatusSet || _busState == t1 || _busState == tIdleStatusSet) {
                }
                else {
                    waitForBusIdle();
                }
                wait(2);
                break;
            case 58:
                if (_useMemory)
                    wait(1);
                wait(2);
                _transferStarting = true;
                if (_busState == t4StatusSet || _busState == t1 || _busState == tIdleStatusSet) {
                }
                else {
                    waitForBusIdle();
                }
                wait(2);
                break;
            case 59:
                wait(2);
                _prefetching = false;
                if (_useMemory)
                    wait(1);
                waitForBusIdle();
                wait(1);
                _transferStarting = true;
                wait(2);
                break;
            case 60:
                wait(4);
                break;
            case 61:
                break;
            case 62:
                wait(1);
                break;
            case 63:
                break;
            case 64:
                break;
            case 65:
                wait(1);
                _prefetching = false;
                wait(2);
                if (_useMemory)
                    wait(1);
                while ((_busState != tIdle && _busState != tFirstIdle && _busState != tSecondIdle) || _ioNext._type != ioPassive)
                    wait(1);
                break;
            case 66:
                wait(1);
                break;
            case 67:
                break;
            case 68:
                while ((_busState != tIdle && _busState != tSecondIdle) || _ioNext._type != ioPassive)  // Weird
                    wait(1);
                wait(1);
                break;
            case 69:
                break;
            case 70:
                wait(2);
                waitForBusIdle();
                wait(3);
                break;
            case 71:
                break;
            case 72:
                break;
            default:
                printf("Error: unknown access number\n");
                break;
        }
    }
    Word busReadWord(IOType type)
    {
        busInit();
        while (_ioNext._type != ioPassive)
            wait(1);
        _transferStarting = false;
        initIO(type, _address);
        do {
            wait(1);
        } while (_ioNext._type != ioPassive);
        // Now on T1 of the first fetch
        initIO(type, _address + 1);
        do {
            wait(1);
        } while (_busState != t3tWaitLast);
        // Now on last T3/Tw of the first fetch
        Word result = _io._data;
        do {
            wait(1);
        } while (_ioNext._type != ioPassive || _busState != t3tWaitLast);
        // Now on last T3/Tw of the second fetch
        _forcedSegment = -1;
        return result | (_io._data << 8);
    }
    void busWriteWord(IOType type)
    {
        busInit();
        busAccess(type, _address);
        _data >>= 8;
        busAccess(type, _address + 1);
        do {
            wait(1);
        } while (_busState != t3tWaitLast);
        _forcedSegment = -1;
    }
    Byte busReadByte(IOType type)
    {
        busInit();
        busAccess(type, _address);
        do {
            wait(1);
        } while (_ioNext._type != ioPassive || _busState != t3tWaitLast);
        _forcedSegment = -1;
        return _io._data;
    }
    void busWriteByte(IOType type)
    {
        busInit();
        busAccess(type, _address);
        do {
            wait(1);
        } while (_busState != t3tWaitLast);
        _forcedSegment = -1;
    }
    Byte queueRead()
    {
        while (!_queueHasByte)
            wait(1);
        Byte byte = _queueData[(_queueReadPosition) & 3];
        _snifferDecoder.queueOperation(3);
        acknowledgeInstructionByte();
        return byte;
    }
    void acknowledgeInstructionByte()
    {
        _queueReadPosition = (_queueReadPosition + 1) & 3;
        _delayedPrefetchedRemove = true;
    }
    Byte fetchInstructionByte()
    {
        Byte b = queueRead();
        wait(1);
        return b;
    }
    Word fetchInstructionWord()
    {
        Byte low = queueRead();
        wait(1);
        Byte high = queueRead();
        return (high << 8) | low;
    }
    Word fetchInstructionData()
    {
        if (_wordSize)
            return fetchInstructionWord();
        return fetchInstructionByte();
    }
    void busRead(IOType type = ioReadMemory)
    {
        if (_wordSize)
            _data = busReadWord(type);
        else
            _data = busReadByte(type) | 0xff00;
    }
    Word readByteRegister(int n)
    {
        Word r = _wordRegisters[n & 3];
        if ((n & 4) != 0)
            r = (r >> 8) + (r << 8);
        return r;
    }
    void readEA(bool memoryOnly = false)
    {
        if (_useMemory) {
            busRead();
            return;
        }
        if (!memoryOnly) {
            if (!_wordSize)
                _data = readByteRegister(modRMReg2());
            else
                _data = _wordRegisters[modRMReg2()];
        }
    }
    void readEA2()
    {
        _address += 2;
        busRead();
    }
    void busWrite(IOType type = ioWriteMemory)
    {
        if (_wordSize)
            busWriteWord(type);
        else
            busWriteByte(type);
    }
    void writeEA(Word data)
    {
        _data = data;
        if (_useMemory) {
            busWrite();
            return;
        }
        if (!_wordSize)
            *_byteRegisters[modRMReg2()] = static_cast<Byte>(_data);
        else
            _wordRegisters[modRMReg2()] = _data;
    }
    void doModRM()
    {
        _modRM = queueRead();
        _useMemory = (_modRM & 0xc0) != 0xc0;
        if (!_useMemory)
            return;
        wait(1);
        if ((_modRM & 0xc7) == 0x06) {
            wait(1);
            _address = fetchInstructionWord();
            _segment = 3;
            wait(1);
            return;
        }
        switch (_modRM & 7) {
            case 0: wait(2); _address = bx() + si(); _segment = 3; break;
            case 1: wait(3); _address = bx() + di(); _segment = 3; break;
            case 2: wait(3); _address = bp() + si(); _segment = 2; break;
            case 3: wait(2); _address = bp() + di(); _segment = 2; break;
            case 4:          _address =        si(); _segment = 3; break;
            case 5:          _address =        di(); _segment = 3; break;
            case 6:          _address = bp();        _segment = 2; break;
            case 7:          _address = bx();        _segment = 3; break;
        }
        switch (_modRM & 0xc0) {
            case 0x40:
                wait(3);
                _address += signExtend(fetchInstructionByte());
                break;
            case 0x80:
                wait(3);
                _address += fetchInstructionWord();
                break;
        }
        wait(2);
    }
    void executeOneInstruction()
    {
        _accessNumber = 0;
        if (!_repeating) {
            if (getRealIP() == _timeIP1 && cs() == testSegment + 0x1000)
                _cycle1 = _cycle;
            _opcode = queueRead();
            _snifferDecoder.queueOperation(1);
            if (_clearLock) {
                _lock = false;
                _clearLock = false;
                _bus.setLock(false);
            }
            wait(1);
            _wordSize = ((_opcode & 1) != 0);
        }
        _completed = true;
        switch (_opcode) {
            case 0x00: case 0x01: case 0x02: case 0x03:
            case 0x08: case 0x09: case 0x0a: case 0x0b:
            case 0x10: case 0x11: case 0x12: case 0x13:
            case 0x18: case 0x19: case 0x1a: case 0x1b:
            case 0x20: case 0x21: case 0x22: case 0x23:
            case 0x28: case 0x29: case 0x2a: case 0x2b:
            case 0x30: case 0x31: case 0x32: case 0x33:
            case 0x38: case 0x39: case 0x3a: case 0x3b: // alu rm, r / r, rm
                doModRM();
                _accessNumber = 46;
                readEA();
                _aluOperation = (_opcode >> 3) & 7;
                if ((_opcode & 2) == 0) {
                    _destination = _data;
                    _source = getReg();
                }
                else {
                    _destination = getReg();
                    _source = _data;
                }
                if (_useMemory)
                    wait(2);
                wait(1);
                doALUOperation();
                if (_aluOperation != 7) {
                    if ((_opcode & 2) == 0) {
                        _accessNumber = 10;
                        writeEA(_data);
                        if (!_useMemory)
                            wait(1);
                    }
                    else {
                        setReg(_data);
                        wait(1);
                    }
                }
                else
                    wait(1);
                break;
            case 0x04: case 0x05: case 0x0c: case 0x0d:
            case 0x14: case 0x15: case 0x1c: case 0x1d:
            case 0x24: case 0x25: case 0x2c: case 0x2d:
            case 0x34: case 0x35: case 0x3c: case 0x3d: // alu A, imm
                wait(1);
                _data = fetchInstructionData();
                _destination = getAccum();                                 
                _source = _data;
                _aluOperation = (_opcode >> 3) & 7;
                doALUOperation();
                if (_aluOperation != 7)
                    setAccum();
                wait(1);
                break;
            case 0x06: case 0x0e: case 0x16: case 0x1e: // PUSH segreg
                _wordSize = true;
                _accessNumber = 29;
                push(_segmentRegisters[_opcode >> 3]);
                break;
            case 0x07: case 0x0f: case 0x17: case 0x1f: // POP segreg
                _accessNumber = 22;
                _segmentRegisters[_opcode >> 3] = pop();
                wait(1);
                break;
            case 0x26: case 0x2e: case 0x36: case 0x3e: // segreg:
                wait(1);
                _segmentOverride = (_opcode >> 3) & 3;
                _completed = false;
                break;
            case 0x27: // DAA
                wait(1);
                if (af() || (al() & 0x0f) > 9) {
                    _data = al() + 6;
                    al() = static_cast<Byte>(_data);
                    setAF(true);
                    if ((_data & 0x100) != 0)
                        setCF(true);
                }
                if (cf() || al() > 0x9f) {
                    al() += 0x60;
                    setCF(true);
                }
                da();
                break;
            case 0x2f: // DAS
                {
                    wait(1);
                    Byte t = al();
                    if (af() || ((al() & 0xf) > 9)) {
                        _data = al() - 6;
                        al() = static_cast<Byte>(_data);
                        setAF(true);
                        if ((_data & 0x100) != 0)
                            setCF(true);
                    }
                    if (cf() || t > 0x9f) {
                        al() -= 0x60;
                        setCF(true);
                    }
                }
                da();
                break;
            case 0x37: // AAA
                wait(1);
                if (af() || ((al() & 0xf) > 9)) {
                    al() += 6;
                    ++ah();
                    setCA();
                }
                else {
                    clearCA();
                    wait(1);
                }

                aa();
                break;
            case 0x3f: // AAS
                wait(1);
                if (af() || ((al() & 0xf) > 9)) {
                    al() -= 6;
                    --ah();
                    setCA();
                }
                else {
                    clearCA();
                    wait(1);
                }
                aa();
                break;
            case 0x40: case 0x41: case 0x42: case 0x43:
            case 0x44: case 0x45: case 0x46: case 0x47:
            case 0x48: case 0x49: case 0x4a: case 0x4b:
            case 0x4c: case 0x4d: case 0x4e: case 0x4f: // INCDEC rw
                wait(1);
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
                break;
            case 0x50: case 0x51: case 0x52: case 0x53:
            case 0x54: case 0x55: case 0x56: case 0x57: // PUSH rw
                _wordSize = true;
                _accessNumber = 30;
                push(rw());
                break;
            case 0x58: case 0x59: case 0x5a: case 0x5b:
            case 0x5c: case 0x5d: case 0x5e: case 0x5f: // POP rw
                _accessNumber = 23;
                rw() = pop();
                wait(1);
                break;
            case 0x60: case 0x61: case 0x62: case 0x63:
            case 0x64: case 0x65: case 0x66: case 0x67:
            case 0x68: case 0x69: case 0x6a: case 0x6b:
            case 0x6c: case 0x6d: case 0x6e: case 0x6f:
            case 0x70: case 0x71: case 0x72: case 0x73:
            case 0x74: case 0x75: case 0x76: case 0x77:
            case 0x78: case 0x79: case 0x7a: case 0x7b:
            case 0x7c: case 0x7d: case 0x7e: case 0x7f: // Jcond cb
                wait(1);
                _data = fetchInstructionByte();
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
                    wait(1);
                    if (jump)
                        jumpShort();
                }
                break;
            case 0x80: case 0x81: case 0x82: case 0x83: // alu rm, imm
                doModRM();
                _accessNumber = 47;
                readEA();
                _destination = _data;
                if (_useMemory)
                    wait(3);
                if (_opcode == 0x81) {
                    if (!_useMemory)
                        wait(1);
                    _source = fetchInstructionWord();
                }
                else {
                    if (!_useMemory)
                        wait(1);
                    if (_opcode == 0x83)
                        _source = signExtend(fetchInstructionByte());
                    else
                        _source = fetchInstructionByte() | 0xff00;
                }
                wait(1);
                _aluOperation = modRMReg();
                doALUOperation();
                if (_aluOperation != 7) {
                    _accessNumber = 11;
                    writeEA(_data);
                }
                else {
                    if (_useMemory)
                        wait(1);
                }
                break;
            case 0x84: case 0x85: // TEST rm, reg
                doModRM();
                _accessNumber = 48;
                readEA();
                test(_data, getReg());
                if (_useMemory)
                    wait(2); //4);
                wait(2);
                break;
            case 0x86: case 0x87: // XCHG rm, reg
                doModRM();
                _accessNumber = 49;
                readEA();
                _source = getReg();
                setReg(_data);
                wait(3);
                _accessNumber = 12;
                writeEA(_source);
                break;
            case 0x88: case 0x89: // MOV rm, reg
                doModRM();
                wait(1);
                _accessNumber = 13;
                writeEA(getReg());
                break;
            case 0x8a: case 0x8b: // MOV reg, rm
                doModRM();
                _accessNumber = 50;
                readEA();
                setReg(_data);
                wait(1);
                if (_useMemory)
                    wait(2);
                break;
            case 0x8c: // MOV rmw, segreg
                doModRM();
                _wordSize = true;
                if (!_useMemory)
                    wait(1);
                _accessNumber = 14;
                writeEA(_segmentRegisters[modRMReg() & 3]);
                break;
            case 0x8d: // LEA rw, rmw
                doModRM();
                setReg(_address);
                wait(1);
                if (_useMemory)
                    wait(2);
                break;
            case 0x8e: // MOV segreg, rmw
                doModRM();
                _wordSize = true;
                _accessNumber = 51;
                readEA();
                _segmentRegisters[modRMReg() & 3] = _data;
                wait(1);
                if (_useMemory)
                    wait(2);
                break;
            case 0x8f: // POP rmw
                doModRM();
                wait(1);
                _source = _address;
                _accessNumber = 24;
                if (_useMemory)
                    wait(2);
                _data = pop();
                _address = _source;
                wait(2);

                _accessNumber = 15;
                writeEA(_data);
                break;
            case 0x90: case 0x91: case 0x92: case 0x93:
            case 0x94: case 0x95: case 0x96: case 0x97: // XCHG AX, rw
                wait(1);
                _data = rw();
                rw() = ax();
                ax() = _data;
                wait(1);
                break;
            case 0x98: // CBW
                wait(1);
                ax() = signExtend(al());
                break;
            case 0x99: // CWD
                wait(1);
                wait(3);
                if (!topBit(ax()))
                    dx() = 0;
                else {
                    wait(1);
                    dx() = 0xffff;
                }
                break;
            case 0x9a: // CALL cd
                {
                    wait(1);
                    Word newIP = fetchInstructionWord();
                    wait(1);
                    Word newCS = fetchInstructionWord();
                    _prefetching = false;
                    _wordSize = true;

                    _accessNumber = 31;
                    push(cs());

                    _accessNumber = 60;
                    Word oldIP = ip();
                    cs() = newCS;
                    setIP(newIP);

                    _accessNumber = 32;
                    push(oldIP);
                }
                break;
            case 0x9b: // WAIT
                if (!_repeating)
                    wait(2);
                wait(5);
                if (interruptPending()) {
                    wait(7);
                    _snifferDecoder.queueOperation(2);
                    checkInterrupts();
                }
                else {
                    _repeating = true;
                    _completed = false;
                }
                break;
            case 0x9c: // PUSHF
                _wordSize = true;
                _accessNumber = 33;
                push((_flags & 0x0fd7) | 0xf000);
                break;
            case 0x9d: // POPF
                _accessNumber = 25;
                _flags = pop() | 2;
                wait(1);
                break;
            case 0x9e: // SAHF
                wait(1);
                _flags = (_flags & 0xff02) | ah();
                wait(2);
                break;
            case 0x9f: // LAHF
                wait(1);
                ah() = _flags & 0xd7;
                break;
            case 0xa0: case 0xa1: // MOV A, [iw]
                wait(1);
                _address = fetchInstructionWord();
                _accessNumber = 1;
                busRead();
                setAccum();
                wait(1);
                break;
            case 0xa2: case 0xa3: // MOV [iw], A
                wait(1);
                _address = fetchInstructionWord();
                _data = getAccum();
                _accessNumber = 7;
                busWrite();
                break;
            case 0xa4: case 0xa5: // MOVS
            case 0xac: case 0xad: // LODS
                if (!_repeating) {
                    wait(1);
                    if (_busState == tFirstIdle)  // Weird
                        wait(1);
                    if ((_opcode & 8) == 0 && _rep != 0)
                        wait(1);
                }
                if (repAction()) {
                    wait(1);
                    if ((_opcode & 8) != 0)
                        wait(1);
                    break;
                }             
                if (_rep != 0 && (_opcode & 8) != 0)
                    wait(1);

                _accessNumber = 20;
                lods();
                if ((_opcode & 8) == 0) {
                    _accessNumber = 27;
                    stos();
                }
                else {
                    if (_rep != 0)
                        wait(2);
                }
                if (_rep == 0) {
                    wait(3);
                    if ((_opcode & 8) != 0)
                        wait(1);
                    break;
                }
                _repeating = true;
                break;
            case 0xa6: case 0xa7: // CMPS
            case 0xae: case 0xaf: // SCAS
                if (!_repeating)
                    wait(1);
                if (repAction()) {
                    wait(2);
                    break;
                }
                if (_rep != 0)
                    wait(1);
                wait(1);
                _destination = getAccum();
                if ((_opcode & 8) == 0) {
                    _accessNumber = 21;
                    lods();
                    wait(1);
                    _destination = _data;
                }
                _address = di();
                _forcedSegment = 0;
                _accessNumber = 2;
                busRead();
                di() = stringIncrement();
                _source = _data;
                sub();
                wait(2);
                if (_rep == 0) {
                    wait(3);
                    break;
                }
                if (zf() == (_rep == 1)) {
                    _completed = true;
                    wait(4);
                    break;
                }
                _repeating = true;
                break;
            case 0xa8: case 0xa9: // TEST A, imm
                wait(1);
                _data = fetchInstructionData();
                test(getAccum(), _data);
                wait(1);
                break;
            case 0xaa: case 0xab: // STOS
                if (!_repeating) {
                    wait(1);
                    if (_busState == tFirstIdle)  // Weird
                        wait(1);
                    if (_rep != 0)
                        wait(1);
                }
                if (repAction()) {
                    wait(1);
                    break;
                }
                _data = ax();
                _accessNumber = 28;
                stos();
                if (_rep == 0) {
                    wait(3);
                    break;
                }
                _repeating = true;
                break;
            case 0xb0: case 0xb1: case 0xb2: case 0xb3:
            case 0xb4: case 0xb5: case 0xb6: case 0xb7: // MOV rb, ib
                wait(1);
                *_byteRegisters[_opcode & 7] = fetchInstructionByte();
                wait(1);
                break;
            case 0xb8: case 0xb9: case 0xba: case 0xbb:
            case 0xbc: case 0xbd: case 0xbe: case 0xbf: // MOV rw, iw
                wait(1);
                rw() = fetchInstructionWord();
                wait(1);
                break;
            case 0xc0: case 0xc1: case 0xc2: case 0xc3:
            case 0xc8: case 0xc9: case 0xca: case 0xcb: // RET
                {
                    if ((_opcode & 9) != 1)
                        wait(1);
                    if (!_wordSize) {
                        _source = fetchInstructionWord();
                        wait(1);
                    }
                    if ((_opcode & 9) == 9)
                        wait(1);
                    _prefetching = false;
                    _segmentOverride = -1;
                    _accessNumber = 26;
                    Word newIP = pop();
                    wait(2);
                    Word newCS;
                    if ((_opcode & 8) == 0)
                        newCS = cs();
                    else {
                        _accessNumber = 42;
                        newCS = pop();
                        if (_wordSize)
                            wait(1);
                    }
                    if (!_wordSize) {
                        sp() += _source;
                        wait(1);
                    }
                    cs() = newCS;
                    _accessNumber = 72;
                    setIP(newIP);
                }
                break;
            case 0xc4: case 0xc5: // LsS rw, rmd
                doModRM();
                _wordSize = true;
                _accessNumber = 52;
                readEA(true);
                setReg(_data);
                _accessNumber = 57;
                readEA2();
                _segmentRegisters[!_wordSize ? 3 : 0] = _data;
                wait(1);
                break;
            case 0xc6: case 0xc7: // MOV rm, imm
                doModRM();
                wait(1);
                if (_useMemory)
                    wait(2);
                _data = fetchInstructionData();

                if (!_useMemory)
                    wait(1);
                _accessNumber = 16;
                writeEA(_data);
                break;
            case 0xcc: // INT 3
                interrupt(3);
                break;
            case 0xcd: // INT
                {
                    wait(1);
                    Byte i = fetchInstructionByte();
                    //wait(1);
                    interrupt(i);
                }
                break;
            case 0xce: // INTO
                wait(3);
                if (of()) {
                    wait(2); //3);
                    interrupt(4);
                }
                break;
            case 0xcf: // IRET
                {
                    _segmentOverride = -1;
                    _accessNumber = 43;
                    Word newIP = pop();
                    wait(3);
                    _accessNumber = 44;
                    Word newCS = pop();
                    cs() = newCS;
                    _accessNumber = 62;
                    setIP(newIP);
                    _accessNumber = 45;
                    _flags = pop() | 2;
                    wait(5);
                }
                break;
            case 0xd0: case 0xd1: case 0xd2: case 0xd3: // rot rm
                doModRM();
                if (!_useMemory)
                    wait(1);
                _accessNumber = 53;
                readEA();
                if ((_opcode & 2) == 0) {
                    _source = 1;
                    wait(_useMemory ? 4 : 0);
                }
                else {
                    _source = cl();
                    wait(_useMemory ? 9 : 6);
                }
                while (_source != 0) {
                    _destination = _data;
                    bool oldCF = cf();
                    switch (modRMReg()) {
                        case 0:  // ROL
                            setCF(topBit(_data));
                            _data <<= 1;
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
                            setCF(topBit(_data));
                            _data = (_data << 1) | (oldCF ? 1 : 0);
                            setOFRotate();
                            break;
                        case 3:  // RCR
                            setCF((_data & 1) != 0);
                            _data >>= 1;
                            if (oldCF)
                                _data |= (!_wordSize ? 0x80 : 0x8000);
                            setCF((_destination & 1) != 0);
                            setOFRotate();
                            break;
                        case 4:  // SHL
                            setCF(topBit(_data));
                            _data <<= 1;
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
                        case 6:  // SETMO
                            bitwise(0xffff);
                            setCF(false);
                            setOFRotate();
                            setAF(false);
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
                        wait(4);
                    --_source;
                }
                _accessNumber = 17;
                writeEA(_data);
                break;
            case 0xd4: // AAM
                wait(1);
                _source = fetchInstructionByte();
                if (div(al(), 0)) {
                    _wordSize = true;  // Probably incorrect
                    setPZS();
                }
                break;
            case 0xd5: // AAD
                wait(1);
                _wordSize = false;
                mul(fetchInstructionByte(), ah());
                al() += _data;
                ah() = 0;
                _wordSize = true;  // Probably incorrect
                setPZS();
                break;
            case 0xd6: // SALC
                wait(1);
                al() = (cf() ? 0xff : 0x00);
                wait(1);
                break;
            case 0xd7: // XLATB
                _address = bx() + al();
                _segment = 3;
                _accessNumber = 4;
                al() = busReadByte(ioReadMemory);
                wait(1);
                break;
            case 0xd8: case 0xd9: case 0xda: case 0xdb:
            case 0xdc: case 0xdd: case 0xde: case 0xdf: // esc i, r, rm
                doModRM();
                _wordSize = true;
                _accessNumber = 54;
                readEA();
                wait(1);
                if (_useMemory)
                    wait(2);
                break;
            case 0xe0: case 0xe1: case 0xe2: case 0xe3: // loop
                wait(3);
                _data = fetchInstructionByte();
                {
                    bool jump;
                    if (_opcode != 0xe2)
                        wait(1);
                    if (_opcode != 0xe3) {
                        --cx();
                        jump = (cx() != 0);
                        switch (_opcode) {
                            case 0xe0: if (zf()) jump = false; break;
                            case 0xe1: if (!zf()) jump = false; break;
                        }
                    }
                    else
                        jump = (cx() == 0);
                    if (jump)
                        jumpShort();
                }
                break;
            case 0xe4: case 0xe5: case 0xe6: case 0xe7:
            case 0xec: case 0xed: case 0xee: case 0xef: // INOUT
                if ((_opcode & 0x0e) != 0x0c)
                    wait(1);
                if ((_opcode & 8) == 0)
                    _data = fetchInstructionByte();
                else
                    _data = dx();
                _segmentOverride = -1;
                _segment = 7;
                _address = _data;
                if ((_opcode & 2) == 0) {
                    _accessNumber = 3;
                    busRead(ioReadPort);
                    wait(1);
                    setAccum();
                }
                else {
                    _data = getAccum();
                    if ((_opcode & 8) == 0)
                        _accessNumber = 8;
                    else
                        _accessNumber = 9;
                    busWrite(ioWritePort);
                }
                break;
            case 0xe8: // CALL cw
                {
                    wait(1);
                    Word oldIP = jumpNear();
                    _wordSize = true;
                    _accessNumber = 34;
                    push(oldIP);
                }
                break;
            case 0xe9: // JMP cw
                wait(1);
                jumpNear();
                break;
            case 0xea: // JMP cp
                {
                    wait(1);
                    Word newIP = fetchInstructionWord();
                    wait(1);
                    Word newCS = fetchInstructionWord();
                    cs() = newCS;
                    _accessNumber = 70;
                    _prefetching = false;
                    setIP(newIP);
                }
                break;
            case 0xeb: // JMP cb
                wait(1);
                _data = fetchInstructionByte();
                jumpShort();
                wait(1);
                break;
            case 0xf0: case 0xf1: // LOCK
                _lock = true;
                _bus.setLock(true);
                wait(1);
                _completed = false;
                break;
            case 0xf2: case 0xf3: // REP
                wait(1);
                _rep = (_opcode == 0xf2 ? 1 : 2);
                _completed = false;
                break;
            case 0xf4: // HLT
                if (!_repeating) {
                    _accessNumber = 69;
                    if (_busState == tIdle || (_busState == tFirstIdle && _ioLast._type != ioCodeAccess))
                        _data = 1;
                    else
                        _data = 2;
                    wait(1);
                    _prefetching = false;
                }
                wait(1);
                if (interruptPending()) {
                    wait(_data);
                    checkInterrupts();
                }
                else {
                    _repeating = true;
                    _completed = false;
                }
                break;
            case 0xf5: // CMC
                wait(1);
                _flags ^= 1;
                break;
            case 0xf6: case 0xf7: // math
                doModRM();
                _accessNumber = 55;
                readEA();
                switch (modRMReg()) {
                    case 0:
                    case 1:  // TEST
                        wait(2);
                        if (_useMemory)
                            wait(1);
                        _source = fetchInstructionData();
                        wait(1);
                        test(_data, _source);
                        if (_useMemory)
                            wait(1);
                        break;
                    case 2:  // NOT
                    case 3:  // NEG
                        wait(2);
                        if (modRMReg() == 2)
                            _data = ~_data;
                        else {
                            _source = _data;
                            _destination = 0;
                            sub();
                        }
                        _accessNumber = 18;
                        writeEA(_data);
                        break;
                    case 4:  // MUL
                    case 5:  // IMUL
                        wait(1);
                        mul(getAccum(), _data);
                        if (_wordSize) {
                            ax() = _data;
                            dx() = _destination;
                            _data |= dx();
                            setCOMul(dx() != ((ax() & 0x8000) == 0 || modRMReg() == 4 ? 0 : 0xffff));
                        }
                        else {
                            al() = static_cast<Byte>(_data);
                            ah() = static_cast<Byte>(_destination);
                            setCOMul(ah() != ((al() & 0x80) == 0 || modRMReg() == 4 ? 0 : 0xff));
                        }
                        setZF();
                        if (_useMemory)
                            wait(1);
                        break;
                    case 6:  // DIV
                    case 7:  // IDIV
                        if (_useMemory)
                            wait(1);
                        _source = _data;
                        if (div(al(), ah()))
                            wait(1);
                        break;
                }
                break;
            case 0xf8: case 0xf9: // CLCSTC
                wait(1);
                setCF(_wordSize);
                break;
            case 0xfa: case 0xfb: // CLISTI
                wait(1);
                setIF(_wordSize);
                break;                                            
            case 0xfc: case 0xfd: // CLDSTD
                wait(1);
                setDF(_wordSize);
                break;
            case 0xfe: case 0xff: // misc
                doModRM();
                _accessNumber = 56;
                readEA(modRMReg() == 3 || modRMReg() == 5);
                switch (modRMReg()) {
                    case 0:  // INC rm
                    case 1:  // DEC rm
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
                        wait(2);
                        _accessNumber = 19;
                        writeEA(_data);
                        break;
                    case 2:  // CALL rm
                        {
                            if (!_wordSize) {
                                if (_useMemory)
                                    _data |= 0xff00;
                                else
                                    _data = _wordRegisters[modRMReg2()];
                            }
                            _accessNumber = 63;
                            wait(1);
                            _prefetching = false;
                            wait(4);
                            if (_useMemory)
                                wait(1);
                            while (_busState != tIdle || _ioNext._type != ioPassive)  // Weird
                                wait(1);
                            wait(1);

                            Word oldIP = ip();
                            setIP(_data);
                            wait(2);
                            _accessNumber = 35;
                            push(oldIP);
                        }
                        break;
                    case 3:  // CALL rmd
                        {
                            Word newIP = _data;
                            _accessNumber = 58;
                            readEA2();
                            if (!_wordSize)
                                _data |= 0xff00;
                            Word newCS = _data;
                            _accessNumber = 36;
                            push(cs());

                            _accessNumber = 64;
                            wait(4);
                            Word oldIP = ip();
                            cs() = newCS;
                            setIP(newIP);

                            _accessNumber = 37;
                            push(oldIP);
                        }
                        break;
                    case 4:  // JMP rm
                        {
                            if (!_wordSize) {
                                if (_useMemory)
                                    _data |= 0xff00;
                                else
                                    _data = _wordRegisters[modRMReg2()];
                            }
                            _accessNumber = 65;
                            setIP(_data);
                        }
                        break;
                    case 5:  // JMP rmd
                        {
                            Word newIP = _data;
                            _accessNumber = 59;
                            readEA2();
                            if (!_wordSize)
                                _data |= 0xff00;
                            Word newCS = _data;
                            cs() = newCS;
                            _accessNumber = 66;
                            setIP(newIP);
                        }
                        break;
                    case 6:  // PUSH rm
                    case 7:
                        if (_useMemory)
                            wait(1);
                        _segmentOverride = -1;
                        _accessNumber = 38;
                        push(_data);
                        break;
                }
                break;
        }
        if (_completed) {
            _repeating = false;
            _segmentOverride = -1;
            _rep = 0;
            if (_lock)
                _clearLock = true;
            if (tf())
                interrupt(1);
            checkInterrupts();
        }
    }
    String stringForAccessType(IOType type)
    {
        switch (type) {
            case ioInterruptAcknowledge: return "ioInterruptAcknowledge";
            case ioReadPort: return "ioReadPort";
            case ioWritePort: return "ioWritePort";
            case ioHalt: return "ioHalt";
            case ioCodeAccess: return "ioCodeAccess";
            case ioReadMemory: return "ioReadMemory";
            case ioWriteMemory: return "ioWriteMemory";
            case ioPassive: return "ioPassive";
        }
        return "unknown: " + decimal(static_cast<int>(type));
    }
    void printBusState()
    {
        if (!_logging)
            return;
        String s;
        switch (_busState) {
            case t1: s = "t1"; break;
            case t2t3tWaitNotLast: s = "t2t3tWaitNotLast"; break;
            case t3tWaitLast: s = "t3tWaitLast"; break;
            case t4: s = "t4"; break;
            case t4StatusSet: s = "t4StatusSet"; break;
            case tFirstIdle: s = "tFirstIdle"; break;
            case tSecondIdle: s = "tSecondIdle"; break;
            case tIdle: s = "tIdle"; break;
            case tIdleStatusSet: s = "tIdleStatusSet"; break;
        }
        console.write("Bus state: " + s + ", last = " + stringForAccessType(_ioLast._type) + ", current = " + stringForAccessType(_io._type) + "\n");
    }
    bool interruptPending()
    {
        return _nmiRequested || (intf() && _bus.interruptPending());
    }
    void checkInterrupts()
    {
        if (!interruptPending())
            return;
        if (_nmiRequested) {
            _nmiRequested = false;
            interrupt(2);
            return;
        }
        _repeating = false;
        _completed = true;
        _segmentOverride = 1;
        wait(3);
        busAccess(ioInterruptAcknowledge, 0);
        wait(1);
        _bus.setLock(true);  // 8088 datasheet says LOCK set/cleared on T2. TODO: Modify sniffer so we can check
        busAccess(ioInterruptAcknowledge, 0);
        wait(1);
        _bus.setLock(false);
        _lock = false;
        _clearLock = false;
        do {
            wait(1);
        } while (_ioNext._type != ioPassive || _busState != t3tWaitLast);
        Byte i = _io._data;
        wait(3); //4);
        _opcode = 0;
        interrupt(i);
    }
    bool div(Word l, Word h)
    {
        int bitCount = 8;
        if (_wordSize) {
            l = ax();
            h = dx();
            bitCount = 16;
        }
        bool negative = false;
        bool dividendNegative = false;
        if (_opcode != 0xd4) {
            if (modRMReg() == 7) {
                if (topBit(h)) {
                    h = ~h;
                    l = (~l + 1) & sizeMask();
                    if (l == 0)
                        ++h;
                    h &= sizeMask();
                    negative = true;
                    dividendNegative = true;
                    wait(4);
                }
                if (topBit(_source)) {
                    _source = ~_source + 1;
                    negative = !negative;
                }
                else
                    wait(1);
                wait(9);
            }
            wait(3);
        }
        wait(8);
        _source &= sizeMask();
        if (h >= _source) {
            //wait(1);
            if (_opcode != 0xd4)
                wait(1);
            interrupt(0);
            return false;
        }
        if (_opcode != 0xd4)
            wait(1);
        wait(2);
        bool carry = true;
        for (int b = 0; b < bitCount; ++b) {
            Word r = (l << 1) + (carry ? 1 : 0);
            carry = topBit(l);
            l = r;
            r = (h << 1) + (carry ? 1 : 0);
            carry = topBit(h);
            h = r;
            wait(8);
            if (carry) {
                carry = false;
                h -= _source;
                if (b == bitCount - 1)
                    wait(2);
            }
            else {
                carry = _source > h;
                if (!carry) {
                    h -= _source;
                    wait(1);
                    if (b == bitCount - 1)
                        wait(2);
                }
            }
        }
        l = ~((l << 1) + (carry ? 1 : 0));
        if (_opcode != 0xd4 && modRMReg() == 7) {
            wait(4);
            if (topBit(l)) {
                if (!_useMemory)
                    wait(1);
                //wait(1);
                interrupt(0);
                return false;
            }
            wait(7);
            if (negative)
                l = ~l + 1;
            if (dividendNegative)
                h = ~h + 1;
        }
        ah() = h & 0xff;
        al() = l & 0xff;
        if (_wordSize) {
            dx() = h;
            ax() = l;
        }
        return true;
    }
    void mul(Word a, Word b)
    {
        bool negate = false;
        int bitCount = 8;
        Word highBit = 0x80;
        if (_opcode != 0xd5) {
            if (_wordSize) {
                bitCount = 16;
                highBit = 0x8000;
            }
            else
                wait(8);
            if (modRMReg() == 5) {
                if (!topBit(a)) {
                    if (topBit(b)) {
                        wait(1);
                        if ((b & sizeMask()) != (_wordSize ? 0x8000 : 0x80))
                            wait(1);
                        b = ~b + 1;
                        negate = true;
                    }
                }
                else {
                    wait(1);
                    a = ~a + 1;
                    negate = true;
                    if (topBit(b)) {
                        b = ~b + 1;
                        negate = false;
                    }
                    else
                        wait(4);
                }
                wait(10);
            }
            wait(3);
        }
        Word c = 0;
        a &= sizeMask();
        bool carry = (a & 1) != 0;
        a >>= 1;
        for (int i = 0; i < bitCount; ++i) {
            wait(7);
            if (carry) {
                _source = c;
                _destination = b;
                add();
                c = _data & sizeMask();
                wait(1);
                carry = cf();
            }
            Word r = (c >> 1) + (carry ? highBit : 0);
            carry = (c & 1) != 0;
            c = r;
            r = (a >> 1) + (carry ? highBit : 0);
            carry = (a & 1) != 0;
            a = r;
        }
        if (negate) {
            c = ~c;
            a = (~a + 1) & sizeMask();
            if (a == 0)
                ++c;
            wait(9);
        }
        _data = a;
        _destination = c;

        setSF();
        setPF();
    }
    void da()
    {
        _wordSize = false;
        _data = al();
        setPZS();
        wait(2);
    }
    void aa()
    {
        setOF(false); // TODO: Figure out if this is ever set by AAA/AAS
        al() &= 0x0f;
        wait(6);
    }
    Word jump(Word delta)
    {
        _accessNumber = 67;
        _prefetching = false;
        wait(3);
        while ((_busState != tIdle && _busState != tIdleStatusSet && _busState != tSecondIdle) || _ioNext._type != ioPassive)  // Weird
            wait(1);
        wait(2);
        Word oldIP = ip();
        setIP(oldIP + delta);
        return oldIP;
    }
    void jumpShort()
    {
        jump(signExtend(static_cast<Byte>(_data)));
    }
    Word jumpNear() { return jump(fetchInstructionWord()); }

    void interrupt(Byte number)
    {
        _address = number << 2;
        _segment = 1;
        Word oldCS = cs();
        cs() = 0;
        _segmentOverride = -1;
        _accessNumber = 5;
        Word newIP = busReadWord(ioReadMemory);
        wait(1);
        _address += 2;
        _accessNumber = 6;
        Word newCS = busReadWord(ioReadMemory);
        _prefetching = false;
        _wordSize = true;
        _segmentOverride = -1;
        _accessNumber = 39;
        push(_flags & 0x0fd7);
        setIF(false);
        setTF(false);
        _accessNumber = 40;
        push(oldCS);
        Word oldIP = ip();
        cs() = newCS;
        _accessNumber = 68;
        setIP(newIP);
        _accessNumber = 41;
        push(oldIP);
    }
    void test(Word destination, Word source)
    {
        _destination = destination;
        _source = source;
        bitwise(_destination & _source);
    }
    Word stringIncrement()
    {
        int d = _wordSize ? 2 : 1;
        if (df())
            _address -= d;
        else
            _address += d;
        return _address;
    }
    bool repAction()
    {
        if (_rep == 0)
            return false;
        wait(2);
        Word t = cx();
        if (interruptPending()) {
            _accessNumber = 71;
            _prefetching = false;
            setIP(ip() - 2);
            t = 0;
        }
        if (t == 0) {
            wait(1);
            _completed = true;
            _repeating = false;
            return true;
        }
        --cx();
        _completed = false;
        wait(2);
        if (!_repeating)
            wait(2);
        return false;
    }
    void lods()
    {
        _address = si();
        _segment = 3;
        busRead();
        si() = stringIncrement();
    }
    void stos()
    {
        _address = di();
        _forcedSegment = 0;
        busWrite();
        di() = stringIncrement();
    }
    void push(Word data)
    {
        _data = data;
        sp() -= 2;
        _address = sp();
        _forcedSegment = 2;
        busWrite();
    }
    Word pop()
    {
        _address = sp();
        sp() += 2;
        _forcedSegment = 2;
        return busReadWord(ioReadMemory);
    }
    void setCOMul(bool carry)
    {
        setCF(carry);
        setOF(carry);
        if (!carry)
            wait(1);
    }
    void setCA() { setCF(true); setAF(true); }
    void clearCA() { setCF(false); setAF(false); }
    void clearCAO() { clearCA(); setOF(false); }
    void setPZS() { setPF(); setZF(); setSF(); }
    void bitwise(Word data) { _data = data; clearCAO(); setPZS(); }
    void doAF() { setAF(((_data ^ _source ^ _destination) & 0x10) != 0); }
    void setAPZS() { setPZS(); doAF(); }
    Word sizeMask() { return _wordSize ? 0xffff : 0xff; }
    bool topBit(Word w) { return (w & (_wordSize ? 0x8000 : 0x80)) != 0; }
    void setOFAdd()
    {
        setOF(topBit((_data ^ _source) & (_data ^ _destination)));
    }
    void add()
    {
        _data = _destination + _source;
        setAPZS();
        setOFAdd();
        setCF((_source & sizeMask()) > (_data & sizeMask()));
    }
    void setOFSub()
    {
        setOF(topBit((_destination ^ _source) & (_data ^ _destination)));
    }
    void sub()
    {
        _data = _destination - _source;
        setAPZS();
        setOFSub();
        setCF((_source & sizeMask()) > (_destination & sizeMask()));
    }
    void setOFRotate()
    {
        setOF(topBit(_data ^ _destination));
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
    Word signExtend(Byte data) { return data + (data < 0x80 ? 0 : 0xff00); }
    DWord physicalAddress(Word segment, Word offset)
    {
        return ((_segmentRegisters[segment] << 4) + offset) & 0xfffff;
    }

    Word& rw() { return _wordRegisters[_opcode & 7]; }
    Word& ax() { return _wordRegisters[0]; }
    Word& cx() { return _wordRegisters[1]; }
    Word& dx() { return _wordRegisters[2]; }
    Word& bx() { return _wordRegisters[3]; }
    Word& sp() { return _wordRegisters[4]; }
    Word& bp() { return _wordRegisters[5]; }
    Word& si() { return _wordRegisters[6]; }
    Word& di() { return _wordRegisters[7]; }
    Byte& al() { return *_byteRegisters[0]; }
    Byte& cl() { return *_byteRegisters[1]; }
    Byte& ah() { return *_byteRegisters[4]; }
    Word& es() { return _segmentRegisters[0]; }
    Word& cs() { return _segmentRegisters[1]; }
    Word& ss() { return _segmentRegisters[2]; }
    Word& ds() { return _segmentRegisters[3]; }

    bool cf() { return (_flags & 1) != 0; }
    void setCF(bool cf) { _flags = (_flags & ~1) | (cf ? 1 : 0); }
    bool pf() { return (_flags & 4) != 0; }
    void setPF()
    {
        static Byte table[0x100] = {
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
        _flags = (_flags & ~0x40) | ((_data & sizeMask()) == 0 ? 0x40 : 0);
    }
    bool sf() { return (_flags & 0x80) != 0; }
    void setSF() { _flags = (_flags & ~0x80) | (topBit(_data) ? 0x80 : 0); }
    bool tf() { return (_flags & 0x100) != 0; }
    void setTF(bool tf) { _flags = (_flags & ~0x100) | (tf ? 0x100 : 0); }
    bool intf() { return (_flags & 0x200) != 0; }
    void setIF(bool intf) { _flags = (_flags & ~0x200) | (intf ? 0x200 : 0); }
    bool df() { return (_flags & 0x400) != 0; }
    void setDF(bool df) { _flags = (_flags & ~0x400) | (df ? 0x400 : 0); }
    bool of() { return (_flags & 0x800) != 0; }
    void setOF(bool of) { _flags = (_flags & ~0x800) | (of ? 0x800 : 0); }
    int modRMReg() { return (_modRM >> 3) & 7; }
    int modRMReg2() { return _modRM & 7; }
    Word& modRMRW() { return _wordRegisters[modRMReg()]; }
    Word getReg()
    {
        if (!_wordSize)
            return readByteRegister(modRMReg());
        return modRMRW();
    }
    Word getAccum()
    {
        if (!_wordSize)
            return static_cast<Byte>(al());
        return ax();
    }
    void setAccum()
    {
        if (!_wordSize)
            al() = static_cast<Byte>(_data);
        else
            ax() = _data;
    }
    void setReg(Word value)
    {
        if (!_wordSize)
            *_byteRegisters[modRMReg()] = static_cast<Byte>(value);
        else
            modRMRW() = value;
    }
    void setIP(Word value)
    {
        busInit();
        _ip = value;
        _queueBytes = 0;
        _queueHasByte = false;
        _queueHasSpace = true;
        _queueReadPosition = 0;
        _queueWritePosition = 0;
        _snifferDecoder.queueOperation(2);
        _prefetchedRemove = false;
        _delayedPrefetchedRemove = false;
        wait(1);
        _prefetching = true;
        _ioToFillQueue = false;
        _queueEmptied = false;
    }
    Word ip()
    {
        _ip -= _queueBytes;
        return _ip;
    }

    enum BusState
    {
        t1,
        t2t3tWaitNotLast,
        t3tWaitLast,
        t4,
        t4StatusSet,
        tFirstIdle,
        tIdle,
        tIdleStatusSet,
        tSecondIdle
    };
    
    bool _logging;
    Test _test;
    String _log;
    int _logSkip;
    int _cycle;
    BusEmulator _bus;
    int _stopIP;
    int _cycle1;
    int _cycle2;
    int _timeIP1;

    Word _wordRegisters[8];
    Byte* _byteRegisters[8];
    Word _segmentRegisters[8];
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
    Word _flags;

    Byte _opcode;
    Byte _modRM;
    Word _data;
    Word _source;
    Word _destination;
    Word _address;
    bool _useMemory;
    bool _wordSize;
    int _aluOperation;
    int _rep;
    bool _lock;
    bool _repeating;
    bool _completed;
    bool _clearLock;
    int _segment;

    Byte _queueData[4];
    bool _prefetchedRemove;
    bool _delayedPrefetchedRemove;
    int _queueReadPosition;
    int _queueWritePosition;
    int _queueBytes;
    bool _queueEmptied;
    bool _queueHasSpace;
    bool _queueHasByte;
    Word _ip;
    bool _nmiRequested;

    BusState _busState;
    bool _prefetching;
    bool _transferStarting;

    struct IOInformation
    {
        IOType _type;
        DWord _address;
        Byte _data;
        int _segment;
    };

    IOInformation _io;
    IOInformation _ioNext;
    IOInformation _ioLast;
    bool _ioWasDelayed;
    bool _ioToFillQueue;
    bool _ioToFillQueuePopulated;
    bool _ioToFillQueuePopulated1;

    int _segmentOverride;
    int _forcedSegment;

    SnifferDecoder _snifferDecoder;

    Array<Byte> _stub;

    int _accessNumber;
};

static const int nopCounts = 17;

struct Measurements
{
    Measurements()
    {
        for (int i = 0; i < nopCounts; ++i)
            _cycles[i] = -1;
    }
    SInt16 _cycles[nopCounts];
};

class Cache
{
public:
    void setTime(Test test, int time)
    {
        _time[test]._cycles[test.nops()] = time;
    }
    int getTime(Test test)
    {
        return _time[test]._cycles[test.nops()];
    }
    void load(File file)
    {
        auto s = file.tryOpenRead();
        if (s.valid()) {
            UInt64 size = s.size();
            if (size > 0x7fffffff)
                throw Exception("Cache too large.");
            int ss = static_cast<int>(size);
            //printf("Loading cache, size %i\n", ss);
            Array<Byte> d(ss);
            Byte* p = &d[0];
            s.read(p, ss);
            int i = 0;
            //int n = 0;
            while (i < ss) {
                Test t;
                t.read(p);
                int l = t.length();
                p += l;
                i += l;
                setTime(t, t.cycles());
                //++n;
            }
            //printf("%i items loaded in %i records\n", n, _time.count());
        }

        //int nn = 0;
        //for (auto i : _time) {
        //    int l = i.key().length();
        //    for (int j = 0; j < nopCounts; ++j) {
        //        if (i.value()._cycles[j] != -1)
        //            ++nn;
        //    }
        //}
        //printf("found %i items\n",nn);
    }
    void save(File file)
    {
        int size = 0;
        for (auto i : _time) {
            int l = i.key().length();
            for (int j = 0; j < nopCounts; ++j) {
                if (i.value()._cycles[j] != -1)
                    size += l;
            }
        }
        Array<Byte> d(size);
        Byte* p = &d[0];
        //int n = 0;
        for (auto i : _time) {
            Test t = i.key();
            for (int j = 0; j < nopCounts; ++j) {
                int c = i.value()._cycles[j];
                if (c != -1) {
                    p[0] = c & 0xff;
                    p[1] = c >> 8;
                    t.setNops(j);
                    t.output(p + 2);
                    p += t.length();
                    //++n;
                }
            }
        }
        //printf("Saving cache, %i items, %i records, size %i\n",n,_time.count(),size);
        file.save(&d[0], size);
    }
    void dumpStats()
    {
        _time.dumpStats(File("cacheDump.bin"));
    }
private:
    HashTable<Test, Measurements> _time;
};

static const int refreshPeriods[19] = {0, 18, 19, 17, 16, 15, 14, 13, 12, 11,
    10, 9, 8, 7, 6, 5, 4, 3, 2};

static const Byte modrms[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0xc0};

class TestGenerator
{
public:
    TestGenerator() : _section(0), _suffix(0), _nopCount(0), _opcode(0),
        _m(0), _i(-1), _subsection(-1), _d(0, 65535), _refreshPeriod(0),
        _refreshPhase(0), _segment(-1)
    {
        Array<Byte> functional;
        File("functional.bin").readIntoArray(&functional);
        Byte* p = &functional[0];
        int i = 0;
        while (i < functional.count()) {
            Test t;
            t.read(p);
            p += t.length();
            i += t.length();
            _functional.append(t);
        }

        auto s = File("fails.dat").tryOpenRead();
        if (s.valid()) {
            UInt64 size = s.size();
            if (size > 0x7fffffff)
                throw Exception("fails.dat too large.");
            int ss = static_cast<int>(size);
            Array<Byte> d(ss);
            Byte* p = &d[0];
            s.read(p, ss);
            int i = 0;
            while (i < ss) {
                Test t;
                t.read(p);
                int l = t.length();
                p += l;
                i += l;
                _fails.append(t);
            }
        }

        increment();
    }
    Test getNextTest()
    {
        // Section 0: tests that previously failed, most recent failures first.
        if (_section == 0) {  
            Test t = _fails[_i];
            increment();
            return t;
        }
        do {
            bool skip = false;
            bool usesCH = false;
            if (_section == 2 || _section == 6 || _section == 7) {
                if (_subsection == 4 || _subsection == 5)
                    usesCH = true;
                if ((_opcode & 0xfe) == 0xfe && _nopCount == 14 && _segment == 0)
                    skip = true;
                if (_subsection == 0) {
                    Byte o = _opcode & 0xfe;
                    if (o == 0xa4 || o == 0xa6 || o == 0xaa || o == 0xac || o == 0xae || o == 0xf2)
                        usesCH = true;
                    if (_nopCount == 14) {
                        Byte m = (modrms[_m] + (_r << 3)) & 0xf8;
                        if (o == 0xfe && (m == 0xd8 || m == 0xe8))
                            skip = true;
                    }
                }
            }
            if ((usesCH && _nopCount == 11) || (!usesCH && _nopCount == 12))
                skip = true;
            Test t = getNextTest1();
            if (t == Test())
                return t;
            if (skip)
                continue;
            if (!inFails(t))
                return t;
        } while (true);
    }
    bool inFails(Test t)
    {
        for (int i = 0; i < _fails.count(); ++i) {
            if (_fails[i].equalIncludingNops(t))
                return true;
        }
        return false;
    }
    void dumpFailed(Test f)
    {
        int size = 0;
        for (auto i : _fails)
            size += i.length();
        if (f != Test() && !inFails(f))
            size += f.length();
        Array<Byte> d(size);
        Byte* p = &d[0];
        if (f != Test()) {
            p[0] = 0;
            p[1] = 0;
            f.output(p + 2);
            p += f.length();
        }

        for (auto t : _fails) {
            if (f == Test() || !f.equalIncludingNops(t)) {
                p[0] = 0;
                p[1] = 0;
                t.output(p + 2);
                p += t.length();
            }
        }
        File("fails.dat").save(&d[0], size);
    }
    bool finished() { return _section == 8; }
#if GENERATE_NEWFAILS
    bool inFailsArray() { return _inFailsArray; }
#endif

private:
    Test getNextTest1()
    {
        Test t;
        Instruction i;
        switch (_section) {
            case 1:  // Functional tests
                t = _functional[_i];
                break;
            case 2:  // Main section tests without DMA
            case 6:  // Main section tests with DMA
            case 7:  // Main section tests with override
                if (_segment != -1)
                    t.addInstruction(Instruction((_segment << 3) + 0x26));
                switch (_subsection) {
                    case 0:  // Basic tests
                        i = Instruction(_opcode, modrms[_m] + (_r << 3));
                        switch (_opcode) {
                            case 0x07: case 0x0f: case 0x17: case 0x1f: // POP segreg
                                t.preamble(_opcode - 1);  // PUSH segreg
                                break;
                            case 0x58: case 0x59: case 0x5a: case 0x5b:
                            case 0x5c: case 0x5d: case 0x5e: case 0x5f: // POP rw
                                t.preamble(_opcode - 8);  // PUSH rw
                                break;
                            case 0x8f: // POP rmw
                                t.preamble(0x50);  // PUSH AX
                                break;
                            case 0x9b: // WAIT
                                t.preamble(0x31);
                                t.preamble(0xdb);  // XOR BX,BX
                                t.preamble(0x8e);
                                t.preamble(0xdb);  // MOV DS,BX
                                t.preamble(0xc7);
                                t.preamble(0x47);
                                t.preamble(0x20);
                                t.fixup();
                                t.preamble(0x01 + t.codeLength());
                                t.preamble(0x00);  // MOV WORD[BX+0x20],1
                                t.preamble(0x8c);
                                t.preamble(0x4f);
                                t.preamble(0x22);  // MOV [BX+0x22],CS
                                t.preamble(0xb0);
                                t.preamble(0x14);  // MOV AL,0x14
                                t.preamble(0xe6);
                                t.preamble(0x43);  // OUT 0x43,AL
                                t.preamble(0xb0);
                                t.preamble(0x43);  // MOV AL,0x3F   was 1A but the next interrupt came too soon
                                t.preamble(0xe6);
                                t.preamble(0x40);  // OUT 0x40,AL
                                t.preamble(0xfb);  // STI
                                break;
                            case 0x9d: // POPF
                                t.preamble(0x9c);  // PUSHF
                                break;
                            case 0x9a: // CALL cp
                            case 0xea: // JMP cp
                                i.setImmediate(
                                    ((static_cast<DWord>(testSegment + 0x1000)) << 16) + 5 + t.codeLength());
                                t.fixup(0x81);
                                break;
                            case 0xa4: case 0xa5: case 0xa6: case 0xa7: case 0xaa: case 0xab:
                            case 0xac: case 0xad: // MOVS/CMPS/STOS/LODS/SCAS
                                t.preamble(0x90);  // NOP
                                t.preamble(0xb8);
                                t.preamble(0xa8);
                                t.preamble(0x10);  // MOV AX,0x10A8  so that LES doesn't change ES
                                break;
                            case 0xc0: case 0xc2: // RET iw
                                t.preamble(0xb8);  // MOV AX,3
                                t.fixup();
                                t.preamble(0x03 + t.codeLength());
                                t.preamble(0x00);
                                t.preamble(0x50);  // PUSH AX
                                break;
                            case 0xc1: case 0xc3: // RET
                                t.preamble(0xb8);  // MOV AX,1
                                t.fixup();
                                t.preamble(0x01 + t.codeLength());
                                t.preamble(0x00);
                                t.preamble(0x50);  // PUSH AX
                                break;
                            case 0xc5:  // LDS
                                t.preamble(0xc7);
                                t.preamble(0x07);
                                t.preamble(0x00);
                                t.preamble(0x00);  // MOV WORD[BX],0000
                                t.preamble(0x8c);
                                t.preamble(0x4f);
                                t.preamble(0x02);  // MOV WORD[BX+2],CS
                                break;
                            case 0xc8: case 0xca: // RETF iw
                                t.preamble(0x0e);  // PUSH CS
                                t.preamble(0xb8);  // MOV AX,3
                                t.fixup();
                                t.preamble(0x03 + t.codeLength());
                                t.preamble(0x00);
                                t.preamble(0x50);  // PUSH AX
                                break;
                            case 0xc9: case 0xcb: // RETF
                                t.preamble(0x0e);  // PUSH CS
                                t.preamble(0xb8);  // MOV AX,1
                                t.fixup();
                                t.preamble(0x01 + t.codeLength());
                                t.preamble(0x00);
                                t.preamble(0x50);  // PUSH AX
                                break;
                            case 0xcc: case 0xcd: // INT
                                if (_opcode == 0xcd)
                                    i.setImmediate(3);
                                t.preamble(0x31);
                                t.preamble(0xdb);  // XOR BX,BX
                                t.preamble(0x8e);
                                t.preamble(0xdb);  // MOV DS,BX
                                t.preamble(0xc7);
                                t.preamble(0x47);
                                t.preamble(0x0c);
                                t.fixup();
                                t.preamble(_opcode - 0xcb + t.codeLength());
                                t.preamble(0x00);  // MOV WORD[BX+0x0C],0000
                                t.preamble(0x8c);
                                t.preamble(0x4f);
                                t.preamble(0x0e);  // MOV [BX+0x0E],CS
                                break;
                            case 0xcf: // IRET
                                t.preamble(0x9c);  // PUSHF
                                t.preamble(0x0e);  // PUSH CS
                                t.preamble(0xb8);  // MOV AX,1
                                t.fixup();
                                t.preamble(0x01 + t.codeLength());
                                t.preamble(0x00);
                                t.preamble(0x50);  // PUSH AX
                                break;
                            case 0xd4: case 0xd5: // AAx
                                i.setImmediate(10);
                                break;
                            case 0xe4: case 0xe5: case 0xe6: case 0xe7: // IN/OUT ib
                                i.setImmediate(0xe0);
                                break;
                            case 0xf4: // HLT
                                t.preamble(0x31);
                                t.preamble(0xdb);  // XOR BX,BX
                                t.preamble(0x8e);
                                t.preamble(0xdb);  // MOV DS,BX
                                t.preamble(0xc7);
                                t.preamble(0x47);
                                t.preamble(0x20);
                                t.fixup();
                                t.preamble(0x01 + t.codeLength());
                                t.preamble(0x00);  // MOV WORD[BX+0x20],1
                                t.preamble(0x8c);
                                t.preamble(0x4f);
                                t.preamble(0x22);  // MOV [BX+0x22],CS
                                t.preamble(0xb0);
                                t.preamble(0x14);  // MOV AL,0x14
                                t.preamble(0xe6);
                                t.preamble(0x43);  // OUT 0x43,AL
                                t.preamble(0xb0);
                                t.preamble(0x3f);  // MOV AL,0x3F   was 1A but the next interrupt came too soon
                                t.preamble(0xe6);
                                t.preamble(0x40);  // OUT 0x40,AL
                                t.preamble(0xfb);  // STI
                                break;
                            case 0xf6: case 0xf7:
                                t.preamble(0x90);  // NOP
                                t.preamble(0xb8);
                                t.preamble(0xa8);
                                t.preamble(0x10);  // MOV AX,0x10A8  so that LES doesn't change ES
                                if ((i.modrm() & 0x30) == 0x30) {  // DIV/IDIV
                                    t.preamble(0x1e);  // PUSH DS
                                    t.preamble(0x31);
                                    t.preamble(0xdb);  // XOR BX,BX
                                    t.preamble(0x8e);
                                    t.preamble(0xdb);  // MOV DS,BX
                                    t.preamble(0xc7);
                                    t.preamble(0x47);
                                    t.preamble(0x00);
                                    t.fixup();
                                    t.preamble(i.length() + t.codeLength());
                                    t.preamble(0x00);  // MOV WORD[BX+0x00],0002
                                    t.preamble(0x8c);
                                    t.preamble(0x4f);
                                    t.preamble(0x02);  // MOV [BX+0x02],CS
                                    t.preamble(0x1f);  // POP DS
                                }
                                break;
                            case 0xfe: case 0xff:
                                switch (i.modrm() & 0x38) {
                                    case 0x10:  // CALL rm
                                        t.preamble(0xc6);
                                        t.preamble(0x47);
                                        t.preamble(0xc0);
                                        t.preamble(0xc3);  // MOV BYTE[BX-0x40],0xC3  // RET

                                        t.preamble(0xb8);
                                        t.fixup();
                                        t.preamble(i.length() + t.codeLength());
                                        t.preamble(0x00);  // MOV AX,0x0002
                                        t.preamble(0x50);  // PUSH AX
                                        t.preamble(0x58);  // POP AX

                                        t.preamble(0xc7);
                                        t.preamble(0x07);
                                        t.preamble(0xc0);
                                        t.preamble(0xff);  // MOV WORD[BX],0xFFC0  // pointer to RET

                                        t.preamble(0x8c);
                                        t.preamble(0x4f);
                                        t.preamble(0x02);  // MOV [BX+2],CS  // make it far
                                        break;
                                    case 0x18:  // CALL rmd
                                        if ((i.modrm() & 0xc0) != 0xc0) { // CALL md
                                            t.preamble(0x1e);  // PUSH DS
                                            t.preamble(0x31);
                                            t.preamble(0xdb);  // XOR BX,BX
                                            t.preamble(0x8e);
                                            t.preamble(0xdb);  // MOV DS,BX
                                            t.preamble(0xc6);
                                            t.preamble(0x47);
                                            t.preamble(0xdf);
                                            t.preamble(0xcb);  // MOV BYTE[BX+0xFFDF],0xCB  // RETF
                                            t.preamble(0x1f);  // POP DS
                                            t.preamble(0xc6);
                                            t.preamble(0x47);
                                            t.preamble(0x04);
                                            t.preamble(0xcb);  // MOV BYTE[BX+4],0xCB  // RETF

                                            t.preamble(0xb8);
                                            t.fixup();
                                            t.preamble(i.length() + t.codeLength());
                                            t.preamble(0x00);  // MOV AX,0x0002
                                            t.preamble(0x1e);  // PUSH DS
                                            t.preamble(0x50);  // PUSH AX
                                            t.preamble(0x58);  // POP AX
                                            t.preamble(0x1f);  // POP DS

                                            t.preamble(0xc7);
                                            t.preamble(0x07);
                                            t.preamble(0xef);
                                            t.preamble(0xff);  // MOV WORD[BX],0xFFEF  // pointer to RETF

                                            t.preamble(0xc7);
                                            t.preamble(0x47);
                                            t.preamble(0x02);
                                            t.preamble(0xff);
                                            t.preamble(0xff);  // MOV WORD[BX+2],0xFFFF  // pointer to RETF
                                        }
                                        else {  // CALL rd
                                            t.preamble(0x1e);  // PUSH DS
                                            t.preamble(0x31);
                                            t.preamble(0xdb);  // XOR BX,BX
                                            t.preamble(0x8e);
                                            t.preamble(0xdb);  // MOV DS,BX
                                            t.preamble(0xc6);
                                            t.preamble(0x06);
                                            t.preamble(0x70);
                                            t.preamble(0xfe);
                                            t.preamble(0xcb);  // MOV BYTE[BX+0xFE70],0xCB  // RETF
                                            t.preamble(0x1f);  // POP DS

                                            t.preamble(0xc7);
                                            t.preamble(0x47);
                                            t.preamble(0x02);
                                            t.preamble(0xf7);
                                            t.preamble(0xff);  // MOV WORD[BX+2],0xFFF7

                                            t.preamble(0xb8);
                                            t.fixup();
                                            t.preamble(i.length() + t.codeLength());
                                            t.preamble(0x00);  // MOV AX,0x0002
                                            t.preamble(0x1e);  // PUSH DS
                                            t.preamble(0x50);  // PUSH AX
                                            t.preamble(0x58);  // POP AX
                                            t.preamble(0x1f);  // POP DS

                                            t.preamble(0xc6);
                                            t.preamble(0x07);
                                            t.preamble(0x00);  // MOV BYTE[BX],0x00  // Set address register to 0. [0] also used by NOP pattern 11 to make _data
                                        }
                                        break;
                                    case 0x20:  // JMP rm
                                        if ((i.modrm() & 0xc0) != 0xc0) {  // JMP m
                                            t.preamble(0xc6);
                                            t.preamble(0x47);
                                            t.preamble(0xc0);
                                            t.preamble(0xc3);  // MOV BYTE[BX-0x40],0xC3  // RET

                                            t.preamble(0xb8);
                                            t.fixup();
                                            t.preamble(i.length() + t.codeLength());
                                            t.preamble(0x00);  // MOV AX,0x0002
                                            t.preamble(0x50);  // PUSH AX

                                            t.preamble(0xc7);
                                            t.preamble(0x07);
                                            t.preamble(0xc0);
                                            t.preamble(0xff);  // MOV WORD[BX],0xFFC0  // pointer to RET
                                        }
                                        else {  // JMP r
                                            t.preamble(0xc6);
                                            t.preamble(0x07);
                                            t.preamble(0xc3);  // MOV BYTE[BX],0xC3  // RET

                                            t.preamble(0xc6);
                                            t.preamble(0x06);
                                            t.preamble(0x00);
                                            t.preamble(0xff);
                                            t.preamble(0xc3);  // MOV BYTE[BX+0xFF00],0xC3  // RET

                                            t.preamble(0xb8);
                                            t.fixup();
                                            t.preamble(i.length() + t.codeLength());
                                            t.preamble(0x00);  // MOV AX,0x0002
                                            t.preamble(0x50);  // PUSH AX
                                        }
                                        break;
                                    case 0x28:  // JMP rmd
                                        if ((i.modrm() & 0xc0) != 0xc0) {  // JMP md
                                            t.preamble(0x1e);  // PUSH DS
                                            t.preamble(0x31);
                                            t.preamble(0xdb);  // XOR BX,BX
                                            t.preamble(0x8e);
                                            t.preamble(0xdb);  // MOV DS,BX
                                            t.preamble(0xc6);
                                            t.preamble(0x47);
                                            t.preamble(0xdf);
                                            t.preamble(0xcb);  // MOV BYTE[BX+0xFFDF],0xCB  // RETF
                                            t.preamble(0x1f);  // POP DS

                                            t.preamble(0xb8);
                                            t.fixup();
                                            t.preamble(i.length() + t.codeLength());
                                            t.preamble(0x00);  // MOV AX,0x0002
                                            t.preamble(0x1e);  // PUSH DS
                                            t.preamble(0x50);  // PUSH AX

                                            t.preamble(0xc7);
                                            t.preamble(0x07);
                                            t.preamble(0xef);
                                            t.preamble(0xff);  // MOV WORD[BX],0xFFEF  // pointer to RETF

                                            t.preamble(0xc7);
                                            t.preamble(0x47);
                                            t.preamble(0x02);
                                            t.preamble(0xff);
                                            t.preamble(0xff);  // MOV WORD[BX+2],0xFFFF  // pointer to RETF
                                        }
                                        else {  // JMP rd
                                            t.preamble(0x1e);  // PUSH DS
                                            t.preamble(0x31);
                                            t.preamble(0xdb);  // XOR BX,BX
                                            t.preamble(0x8e);
                                            t.preamble(0xdb);  // MOV DS,BX
                                            t.preamble(0xc6);
                                            t.preamble(0x06);
                                            t.preamble(0x70);
                                            t.preamble(0xfe);
                                            t.preamble(0xcb);  // MOV BYTE[BX+0xFE70],0xCB  // RETF
                                            t.preamble(0x1f);  // POP DS

                                            t.preamble(0xc7);
                                            t.preamble(0x47);
                                            t.preamble(0x02);
                                            t.preamble(0xf7);
                                            t.preamble(0xff);  // MOV WORD[BX+2],0xFFF7

                                            t.preamble(0xb8);
                                            t.fixup();
                                            t.preamble(i.length() + t.codeLength());
                                            t.preamble(0x00);  // MOV AX,0x0002
                                            t.preamble(0x1e);  // PUSH DS
                                            t.preamble(0x50);  // PUSH AX

                                            t.preamble(0xc6);
                                            t.preamble(0x07);
                                            t.preamble(0x00);  // MOV BYTE[BX],0x00  // Set address register to 0. [0] also used by NOP pattern 11 to make _data
                                        }
                                        break;
                                }
                                break;
                        }
                        t.addInstruction(i);
                        switch (_opcode) {
                            case 0xf2: case 0xf3: // REP
                                t.preamble(0x90);  // NOP
                                t.preamble(0xb8);
                                t.preamble(0xa8);
                                t.preamble(0x10);  // MOV AX,0x10A8  so that LES doesn't change ES
                                t.preamble(0xb9);
                                t.preamble(0x05);
                                t.preamble(0x00);  // MOV CX,5
                                t.addInstruction(Instruction(0xac));  // LODSB
                                break;
                        }

                        break;
                    case 1:  // CBW/CWD tests
                        t.addInstruction(Instruction(_opcode));
                        t.preamble(0xb8);
                        t.preamble(0xff);
                        t.preamble(0xff);  // MOV AX,-1
                        t.setQueueFiller(1);
                        break;
                    case 2:  // INTO overflow test
                        t.setQueueFiller(2);
                        t.addInstruction(Instruction(0xce));
                        t.preamble(0x31);
                        t.preamble(0xdb);  // XOR BX,BX
                        t.preamble(0x8e);
                        t.preamble(0xdb);  // MOV DS,BX
                        t.preamble(0xc7);
                        t.preamble(0x47);
                        t.preamble(0x10);
                        t.preamble(/*0x01 +*/ t.codeLength());
                        t.preamble(0x00);  // MOV WORD[BX+0x10],0001
                        t.preamble(0x8c);
                        t.preamble(0x4f);
                        t.preamble(0x12);  // MOV [BX+0x12],CS
                        t.fixup(0x07);

                        t.preamble(0xb8);
                        t.preamble(0xff);
                        t.preamble(0xff);  // MOV AX,0xFFFF
                        t.preamble(0xb1);
                        t.preamble(0x10);  // MOV CL,16
                        t.preamble(0xd3);
                        t.preamble(0xe0);  // SHL AX,CL
                        break;
                    case 3:  // Shift/rotate with various counts
                        t.preamble(0x90);  // NOP
                        t.preamble(0xb8);
                        t.preamble(0xa8);
                        t.preamble(0x10);  // MOV AX,0x10A8  so that LES doesn't change ES
                        i = Instruction(_opcode, modrms[_m] + (_r << 3));
                        t.addInstruction(i);
                        t.preamble(0xb1);
                        t.preamble(_count);
                        break;
                    case 4:  // LOOP with CX==1
                        i = Instruction(_opcode);
                        t.addInstruction(i);
                        t.preamble(0xb9);
                        t.preamble(0x01);
                        t.preamble(0x00);  // MOV CX,1
                        break;
                    case 5:  // String operations with various counts
                        i = Instruction(_rep);
                        t.addInstruction(i);
                        t.addInstruction(Instruction(_opcode));
                        t.preamble(0x90);  // NOP
                        t.preamble(0xb8);
                        t.preamble(0xa8);
                        t.preamble(0x10);  // MOV AX,0x10A8  so that LES doesn't change ES
                        t.preamble(0xb9);
                        t.preamble(_count);
                        t.preamble(0x00);  // MOV CX,c
                        t.preamble(0xbe);
                        t.preamble(0x00);
                        t.preamble(0x40);  // MOV SI,0x4000
                        t.preamble(0xbf);
                        t.preamble(0x00);
                        t.preamble(0x60);  // MOV DI,0x6000
                        t.preamble(0xbb);
                        t.preamble(0x00);
                        t.preamble(0xc0);  // MOV BX,0xc000
                        switch ((_rep << 8) + _opcode) {
                            case 0xf2a6:  // REPNE CMPSB
                                t.preamble(0xb8);
                                t.preamble(0x01);
                                t.preamble(0x02);  // MOV AX,0x0201
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xb8);
                                t.preamble(0x03);
                                t.preamble(0x04);  // MOV AX,0x0403
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xbf);
                                t.preamble(0x00);
                                t.preamble(0x40);  // MOV DI,0x4000
                                t.preamble(0xb8);
                                t.preamble(0x02);
                                t.preamble(0x03);  // MOV AX,0x0302
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xb8);
                                t.preamble(0x04);
                                t.preamble(0x04);  // MOV AX,0x0404
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xbf);
                                t.preamble(0x00);
                                t.preamble(0x60);  // MOV DI,0x6000
                                break;
                            case 0xf2a7:  // REPNE CMPSW
                                t.preamble(0xb8);
                                t.preamble(0x01);
                                t.preamble(0x02);  // MOV AX,0x0201
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xb8);
                                t.preamble(0x03);
                                t.preamble(0x04);  // MOV AX,0x0403
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xbf);
                                t.preamble(0x00);
                                t.preamble(0x40);  // MOV DI,0x4000
                                t.preamble(0xb8);
                                t.preamble(0x02);
                                t.preamble(0x03);  // MOV AX,0x0302
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xb8);
                                t.preamble(0x03);
                                t.preamble(0x04);  // MOV AX,0x0403
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xbf);
                                t.preamble(0x00);
                                t.preamble(0x60);  // MOV DI,0x6000
                                break;
                            case 0xf2ae:  // REPNE SCASB
                                t.preamble(0xb8);
                                t.preamble(0x01);
                                t.preamble(0x02);  // MOV AX,0x0201
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xb8);
                                t.preamble(0x03);
                                t.preamble(0x00);  // MOV AX,0x0003
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xbf);
                                t.preamble(0x00);
                                t.preamble(0x60);  // MOV DI,0x6000
                                break;
                            case 0xf2af:  // REPNE SCASW
                                t.preamble(0xb8);
                                t.preamble(0x01);
                                t.preamble(0x02);  // MOV AX,0x0201
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xb8);
                                t.preamble(0x00);
                                t.preamble(0x00);  // MOV AX,0x0000
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xbf);
                                t.preamble(0x00);
                                t.preamble(0x60);  // MOV DI,0x6000
                                break;
                            case 0xf3a6:  // REPE CMPSB
                                t.preamble(0xb8);
                                t.preamble(0x01);
                                t.preamble(0x02);  // MOV AX,0x0201
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xb8);
                                t.preamble(0x03);
                                t.preamble(0x04);  // MOV AX,0x0403
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xbf);
                                t.preamble(0x00);
                                t.preamble(0x40);  // MOV DI,0x4000
                                t.preamble(0xb8);
                                t.preamble(0x01);
                                t.preamble(0x02);  // MOV AX,0x0201
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xb8);
                                t.preamble(0x03);
                                t.preamble(0x03);  // MOV AX,0x0303
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xbf);
                                t.preamble(0x00);
                                t.preamble(0x60);  // MOV DI,0x6000
                                break;
                            case 0xf3a7:  // REPE CMPSW
                                t.preamble(0xb8);
                                t.preamble(0x01);
                                t.preamble(0x02);  // MOV AX,0x0201
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xb8);
                                t.preamble(0x03);
                                t.preamble(0x04);  // MOV AX,0x0403
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xbf);
                                t.preamble(0x00);
                                t.preamble(0x40);  // MOV DI,0x4000
                                t.preamble(0xb8);
                                t.preamble(0x01);
                                t.preamble(0x02);  // MOV AX,0x0201
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xb8);
                                t.preamble(0x03);
                                t.preamble(0x03);  // MOV AX,0x0303
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xbf);
                                t.preamble(0x00);
                                t.preamble(0x60);  // MOV DI,0x6000
                                break;
                            case 0xf3ae:  // REPE SCASB
                            case 0xf3af:  // REPE SCASW
                                t.preamble(0xb8);
                                t.preamble(0x00);
                                t.preamble(0x00);  // MOV AX,0x0000
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xb8);
                                t.preamble(0x00);
                                t.preamble(0x01);  // MOV AX,0x0100
                                t.preamble(0xab);  // STOSW
                                t.preamble(0xbf);
                                t.preamble(0x00);
                                t.preamble(0x60);  // MOV DI,0x6000
                                break;
                        }
                        break;
                    case 6:  // Math instructions with all registers
                        t.addInstruction(Instruction(_opcode, _m));
                        if ((_m & 0x30) == 0x30) {
                            t.preamble(0x1e);  // PUSH DS
                            t.preamble(0x31);
                            t.preamble(0xdb);  // XOR BX,BX
                            t.preamble(0x8e);
                            t.preamble(0xdb);  // MOV DS,BX
                            t.preamble(0xc7);
                            t.preamble(0x47);
                            t.preamble(0x00);
                            t.preamble(/*0x02 +*/ t.codeLength());
                            t.preamble(0x00);  // MOV WORD[BX+0x00],0002
                            t.preamble(0x8c);
                            t.preamble(0x4f);
                            t.preamble(0x02);  // MOV [BX+0x02],CS
                            t.preamble(0x1f);  // POP DS
                            t.fixup(0x08);
                        }
                        break;
                }
                t.setNops(_nopCount);
                switch (_suffix) {
                    case 0:
                        t.addInstruction(Instruction(4, 0));
                        break;
                    case 1:
                        t.addInstruction(Instruction(0x88, 0xc0));
                        t.addInstruction(Instruction(0, 0));
                        break;
                    case 2:
                        for (int i = 0; i < 4; ++i)
                            t.addInstruction(Instruction(4, 0));
                        break;
                    case 3:
                        break;
                    case 4:
                        t.addInstruction(Instruction(0, 0));
                        break;
                    case 5:
                        t.addInstruction(Instruction(5, 0));
                        break;
                    case 6:
                        t.addInstruction(Instruction(0, 4));
                        break;
                    case 7:
                        t.addInstruction(Instruction(0x90));
                        t.addInstruction(Instruction(0x90));
                        t.addInstruction(Instruction(0x90));
                        t.addInstruction(Instruction(0x90));
                        t.addInstruction(Instruction(0, 4));
                        t.addInstruction(Instruction(5, 0));
                        break;
                    case 8:
                        t.addInstruction(Instruction(0x88, 0xc0));
                        t.addInstruction(Instruction(4, 0));
                        break;
                }
                t.setRefreshPeriod(refreshPeriods[_refreshPeriod]);
                t.setRefreshPhase(_refreshPhase);
                break;
            case 3:  // 1000 input pairs for each multiply instruction
                t.setNops(0);
                t.addInstruction(Instruction(_opcode, _m));
                {
                    Word a = _d(_generator);
                    t.preamble(0xb8);
                    t.preamble(a & 0xff);
                    t.preamble(a >> 8);  // MOV AX,a
                }
                {
                    Word b = _d(_generator);
                    t.preamble(0xba);
                    t.preamble(b & 0xff);
                    t.preamble(b >> 8);  // MOV DX,a
                }
                t.setQueueFiller(1);
                break;
            case 4:  // 1000 input pairs for each divide instruction
                t.setNops(0);
                t.addInstruction(Instruction(_opcode, _m));

                t.preamble(0x1e);  // PUSH DS
                t.preamble(0x31);
                t.preamble(0xdb);  // XOR BX,BX
                t.preamble(0x8e);
                t.preamble(0xdb);  // MOV DS,BX
                t.preamble(0xc7);
                t.preamble(0x47);
                t.preamble(0x00);
                t.preamble(/*0x02 +*/ t.codeLength());
                t.preamble(0x00);  // MOV WORD[BX+0x00],0002
                t.preamble(0x8c);
                t.preamble(0x4f);
                t.preamble(0x02);  // MOV [BX+0x02],CS
                t.preamble(0x1f);  // POP DS
                t.fixup(0x08);
                {
                    Word a = _d(_generator);
                    t.preamble(0xb8);
                    t.preamble(a & 0xff);
                    t.preamble(a >> 8);  // MOV AX,a
                }
                {
                    Word b = _d(_generator);
                    t.preamble(0xba);
                    t.preamble(b & 0xff);
                    t.preamble(b >> 8);  // MOV DX,a
                }
                {
                    Word c = _d(_generator);
                    t.preamble(0xbb);
                    t.preamble(c & 0xff);
                    t.preamble(c >> 8);  // MOV BX,a
                }
                t.setQueueFiller(1);
                break;
            case 5:  // 1000 input pairs for each of AAM and AAD
                i = Instruction(_opcode);
                {
                    Word b = _d(_generator);
                    i.setImmediate(b & 0xff);
                }
                t.setNops(0);

                if (_opcode == 0xd4) {
                    t.preamble(0x1e);  // PUSH DS
                    t.preamble(0x31);
                    t.preamble(0xdb);  // XOR BX,BX
                    t.preamble(0x8e);
                    t.preamble(0xdb);  // MOV DS,BX
                    t.preamble(0xc7);
                    t.preamble(0x47);
                    t.preamble(0x00);
                    t.preamble(0x02 + t.codeLength());
                    t.preamble(0x00);  // MOV WORD[BX+0x00],0002
                    t.preamble(0x8c);
                    t.preamble(0x4f);
                    t.preamble(0x02);  // MOV [BX+0x02],CS
                    t.preamble(0x1f);  // POP DS
                    t.fixup(0x08);
                }

                t.addInstruction(i);
                {
                    Word a = _d(_generator);
                    t.preamble(0xb8);
                    t.preamble(a & 0xff);
                    t.preamble(a >> 8);  // MOV AX,a
                }
                t.setQueueFiller(1);
                break;
        }
        increment();
        return t;
    }
    bool incrementModrm()
    {
        ++_r;
        int maxR = 1;
        Instruction i(_opcode);
        if (i.hasModrm() && i.isGroup())
            maxR = 8;
        if (_r < maxR)
            return true;
        _r = 0;
        ++_m;
        int maxM = 1;
        if (Instruction(_opcode).hasModrm())
            maxM = 25;
        if (_m < maxM)
            return true;
        _m = 0;
        return false;
    }
    bool incrementMain()
    {
        int endNopCount = nopCounts;
        if (_section == 6 || _section == 7)
            endNopCount = 14;
        ++_nopCount;
        if (_nopCount < endNopCount)
            return true;
        _nopCount = 0;
        ++_suffix;
        int maxSuffix = 9;
        if (_section == 6)
            maxSuffix = 3;
        if (_section == 7)
            maxSuffix = 2;
        if (_suffix < maxSuffix)
            return true;
        _suffix = 0;

        if (_subsection == 0) {  // Basic tests
            if (incrementModrm())
                return true;
            ++_opcode;
            if (_opcode < 0x100)
                return true;
            _subsection = 1;
            _opcode = 0x98;
            return true;
        }
        if (_subsection == 1) {  // CBW/CWD tests
            ++_opcode;
            if (_opcode < 0x9a)
                return true;
            _subsection = 2;
            return true;
        }
        if (_subsection == 2) {  // INTO overflow test
            _subsection = 3;
            _opcode = 0xd2;
            _r = 0;
            _m = 0;
            _count = 0;
            return true;
        }
        if (_subsection == 3) {  // Shift/rotate with various counts
            ++_count;
            if (_count < 5)
                return true;
            _count = 0;
            if (incrementModrm())
                return true;
            ++_opcode;
            if (_opcode < 0xd4)
                return true;
            _subsection = 4;
            _opcode = 0xe0;
            return true;
        }
        if (_subsection == 4) {  // LOOP with CX==1
            ++_opcode;
            if (_opcode < 0xe4)
                return true;
            _subsection = 5;
            _opcode = 0xa4;
            _rep = 0xf2;
            _count = 0;
            return true;
        }
        if (_subsection == 5) {  // String operations with various counts
            ++_count;
            if (_count < 5)
                return true;
            _count = 0;
            do {
                ++_opcode;
            } while ((_opcode & 0x0e) == 8);
            if (_opcode < 0xb0)
                return true;
            _opcode = 0xa4;
            ++_rep;
            if (_rep < 0xf4)
                return true;
            _subsection = 6;
            _opcode = 0xf6;
            _m = 0xc0;
            return true;
        }
        if (_subsection == 6) {  // Math instructions with all registers
            ++_m;
            if (_m < 0x100)
                return true;
            _m = 0xc0;
            ++_opcode;
            if (_opcode < 0xf8)
                return true;
            _subsection = 0;
            _opcode = 0;
            _m = 0;
            _r = 0;
        }
        return false;
    }
    void increment()
    {
        if (_section == -1) {
            _section = 0;
            _i = -1;
        }
        if (_section == 0) {
            ++_i;
            if (_i < _fails.count())
                return;
            _section = 1;
            _i = -1;
        }
        if (_section == 1) {
            ++_i;
            if (_i < _functional.count())
                return;
            _section = 2;
            _opcode = 0;
            _m = 0;
            _r = 0;
            _nopCount = 0;
            _suffix = 0;
            _subsection = 0;
            _refreshPeriod = 0;
            _refreshPhase = 0;
            return;
            //_groupStartCount = _c;
        }
        if (_section == 2) {  // Main section tests without DMA
            if (incrementMain())
                return;
        //++_suffix;
        //if (_suffix < 6) {
        //    _opcode = 0;
        //    _m = 0;
        //    _r = 0;
        //    _nopCount = 0;
        //    _subsection = 0;
        //    return;
        //}
        //_suffix = 0;

            _section = 3;
            _opcode = 0xf6;
            _m = 0xe2;
            _count = 0;
            return;
        }
        if (_section == 3) {  // 1000 input pairs for each multiply instruction
            ++_count;
            if (_count < 1000)
                return;
            _count = 0;
            _m += 8;
            if (_m < 0xf2)
                return;
            _m = 0xe2;
            ++_opcode;
            if (_opcode < 0xf8)
                return;
            _section = 4;
            _opcode = 0xf6;
            _m = 0xf3;
            _count = 0;
            return;
        }
        if (_section == 4) {  // 1000 input pairs for each divide instruction
            ++_count;
            if (_count < 1000)
                return;
            _count = 0;
            _m += 8;
            if (_m < 0xff)
                return;
            _m = 0xf3;
            ++_opcode;
            if (_opcode < 0xf8)
                return;
            _section = 5;
            _opcode = 0xd4;
            _count = 0;
            return;
        }
        if (_section == 5) {
            ++_count;
            if (_count < 1000)
                return;
            _count = 0;
            ++_opcode;
            if (_opcode < 0xd6)
                return;
            _section = 7;
            _opcode = 0;
            _m = 0;
            _r = 0;
            _nopCount = 0;
            _suffix = 0;
            _subsection = 0;
            _segment = 0;
        }
        if (_section == 7) {
            if (incrementMain())
                return;
            ++_segment;
            if (_segment < 4)
                return;
            _section = 6;
            _opcode = 0;
            _m = 0;
            _r = 0;
            _nopCount = 0;
            _suffix = 0;
            _subsection = 0;
            _refreshPeriod = 1;
            _refreshPhase = 0;
            _segment = -1;
            return;
        }
        if (_section == 6) {
            if (incrementMain())
                return;
            ++_refreshPhase;
            if (_refreshPhase < 4*refreshPeriods[_refreshPeriod])
                return;
            _refreshPhase = 0;
            ++_refreshPeriod;
            if (_refreshPeriod < 19)
                return;
            _section = 8;
        }
    }

    int _section;
    int _subsection;
    int _suffix;
    int _nopCount;
    int _opcode;
    int _m;
    int _i;
    int _r;
    int _refreshPeriod;
    int _refreshPhase;
    int _count;
    int _rep;
    int _groupStartCount;
    int _segment;
#if GENERATE_NEWFAILS
    bool _inFailsArray;
#endif

    std::mt19937 _generator;
    std::uniform_int_distribution<int> _d;

    AppendableArray<Test> _functional;

    AppendableArray<Test> _fails;
};

#define USE_REAL_HARDWARE 1

class Program : public ProgramBase
{
public:
    void run()
    {
        Array<Byte> testProgram;
        File("runtests.bin").readIntoArray(&testProgram);
        Array<Byte> runStub;
        File("runstub.bin").readIntoArray(&runStub);

#if GENERATE_NEWFAILS
        CPUEmulator emulator;
        emulator.setStub(runStub);
        AppendableArray<Test> newFails;
        for (int i = 0; i < 10000000; ++i) {
            Test t = _generator.getNextTest();
            if (!_generator.inFailsArray())
                continue;
            int cycles = emulator.expected(t);
            t.setCycles(cycles);
            bool alreadyThere = false;
            for (int j = 0; j < newFails.count(); ++j) {
                if (newFails[j].equalIncludingNops(t)) {
                    alreadyThere = true;
                    break;
                }
            }
            if (alreadyThere) {
                console.write("Test duplicated: ");
                t.write();
            }
            else
                newFails.append(t);
        }
        return;
#endif

        // Save all tests for comparison against previous implementation.
        //{
        //    int size = 0;
        //    {
        //        TestGenerator generator;
        //        while (!generator.finished())
        //            size += generator.getNextTest().length();
        //    }
        //    Array<Byte> d(size);
        //    Byte* p = &d[0];
        //    {
        //        TestGenerator generator;
        //        while (!generator.finished()) {
        //            Test t = generator.getNextTest();
        //            p[0] = 0;
        //            p[1] = 0;
        //            t.output(p + 2);
        //            p += t.length();
        //        }
        //    }
        //    File("tests.dat").save(&d[0], size);
        //}


        console.write("Loading cache\n");
        _cacheFile = File("cache.dat");
        _cache.load(_cacheFile);
        //_cache.dumpStats();

        console.write("Running tests\n");

        int maxTests = 1000;
        int availableLength = 0xf300 - testProgram.count();
        //Array<int> cycleCounts(_tests.count());
        bool reRunAllBad = false;
        bool expectedFail = false;

        Test retained;
        bool haveRetained = false;
        int totalCount = 0;
        do {
            int bunchLength = 0;
            AppendableArray<Test> bunch;
            AppendableArray<int> runningTests;
            do {
                if (totalCount % 100 == 99)
                    printf(".");
                ++totalCount;
                Test t;
                if (haveRetained) {
                    t = retained;
                    haveRetained = false;
                }
                else {
                    if (_generator.finished())
                        break;
                    t = _generator.getNextTest();
                }
                int cycles = emulator.expected(t);
                Instruction instruction = t.instruction(0);

                // Modify and uncomment to force a passing test to fail to see
                // its sniffer log.
                //if (instruction.opcode() == 0x00 && instruction.modrm() == 0x44)
                //    ++cycles;
                //if (instruction.opcode() == 0x26 && t.instruction(1).opcode() == 0xf2)
                //    ++cycles;
                //if (instruction.opcode() == 0xf2)
                //    ++cycles;

                t.setCycles(cycles);
                //t.setInstructionCycles(emulator.instructionCycles());
                int cached = _cache.getTime(t);
                if (cycles != cached) {
#if !USE_REAL_HARDWARE
                    if (cached == -1)
                        console.write(decimal(totalCount - 1) + " is not in cache.\n");
                    else
                        console.write(decimal(totalCount - 1) + " took " + decimal(cycles) + " cycles, cached value " + decimal(cached) + "\n");
                    return;
#else
                    int nl = bunchLength + t.length();
                    if (nl > availableLength) {
                        retained = t;
                        haveRetained = true;
                        --totalCount;
                        break;
                    }
                    bunch.append(t);
                    runningTests.append(totalCount - 1);
                    bunchLength = nl;
                    if (cached != -1 && !reRunAllBad) {
                        console.write("\nFailing test " +
                            decimal(totalCount - 1) + "\n");

                        // This test will fail unless the cache is bad.
                        // Just run the one to get a sniffer log.
                        expectedFail = true;
                        break;
                    }
                    if (bunch.count() >= maxTests)
                        break;
#endif
                }
            } while (true);

#if USE_REAL_HARDWARE
            if (bunch.count() == 0)
                break;
            Array<Byte> output(bunchLength + 2);
            Byte* p = &output[0];
            *p = bunchLength;
            p[1] = bunchLength >> 8;
            p += 2;
            for (int i = 0; i < bunch.count(); ++i) {
                Test t = bunch[i];
                int cycles = t.cycles() - 210;
                p[0] = cycles;
                p[1] = cycles >> 8;
                p += 2;
                t.output(p);
                p += t.length() - 2;

                //console.write(emulator.log(_tests[t]));
                //return;
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
                    int index = result.floor();
                    if (!Space::parseOperator(&s, ","))
                        throw Exception("Expected a comma");
                    if (!Space::parseNumber(&s, &result))
                        throw Exception("Cannot parse observed cycle count");
                    int observedCycles = (result.floor() + 210) & 0xffff;
                    int n = runningTests[index];
                    Test t = bunch[index];
                    int cachedCycles = _cache.getTime(t);

                    if (cachedCycles != observedCycles) {
                        if (cachedCycles == -1)
                            _cache.setTime(t, observedCycles);
                        else {
                            console.write("Cache has wrong data for test: " + decimal(cachedCycles) + " cached, " + decimal(observedCycles) + " observed. Test is: ");
                        }
                    }
                    else
                        console.write("Test failed but cache was correct: " + decimal(cachedCycles) + " cached, " + decimal(observedCycles) + " observed, " + decimal(t.cycles()) + " computed. Test is: ");

                    console.write(decimal(n) + "\n");
                    t.write();
                    dumpCache(bunch, index);
                    _generator.dumpFailed(t);

                    String expected = emulator.log(t);
                    String expected1;

                    String observed;
                    CharacterSource e(expected);
                    int skipLines = 4;
                    do {
                        int oc = s.get();
                        if (oc == -1)
                            break;
                        if (oc == '\n')
                            --skipLines;
                    } while (skipLines > 0);

                    int c = t.cycles() - 5;
                    int line = 0;
                    int column = 1;

                    do {
                        int ec = e.get();
                        if (ec == -1)
                            break;
                        ++column;
                        /*if ((column >= 7 && column < 20) || column >= 23)*/ {
                            //if (line < c)
                                expected1 += codePoint(ec);
                        }
                        if (ec == '\n') {
                            ++line;
                            column = 1;
                        }
                    } while (true);
                    line = 0;
                    column = 1;
                    do {
                        int oc = s.get();
                        if (oc == -1)
                            break;
                        ++column;
                        /*if ((column >= 7 && column < 20) || column >= 23)*/ {
                            //if (line < c)
                                observed += codePoint(oc);
                        }
                        if (oc == '\n') {
                            ++line;
                            if (column == 1)
                                break;
                            column = 1;
                        }
                    } while (true);

                    File("expected.txt").openWrite().write(expected1);
                    File("observed.txt").openWrite().write(observed);

                    PROCESS_INFORMATION pi;
                    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

                    STARTUPINFO si;
                    ZeroMemory(&si, sizeof(STARTUPINFO));
                    si.cb = sizeof(STARTUPINFO);

                    NullTerminatedWideString data(
                        String("q observed.txt expected.txt"));

                    IF_FALSE_THROW(CreateProcess(NULL, data, NULL, NULL, FALSE,
                        0, NULL, NULL, &si, &pi) != 0);

                    exit(1);
                }
                if (parse(&s, "PASS"))
                    break;
                int c = s.get();
                if (c == -1)
                    throw Exception("Test was inconclusive");
            } while (true);
            dumpCache(bunch, bunch.count());
            if (expectedFail) {
                reRunAllBad = true;
                console.write("Found incorrect data in cache, re-running all "
                    "failing tests at once.\n");
            }

            if (maxTests < 1000000)
                maxTests *= 2;
#endif
        } while (true);

        console.write("Tests passing: " + decimal(totalCount) + "\n");
        _generator.dumpFailed(Test());

        return;


    //    Array<bool> useTest(noppingTests);
    //    for (int i = 0; i < noppingTests; ++i) {
    //        bool use = true;
    //        int t = (i % groupSize)/nopCounts;
    //        if (t == 2007 || t == 2480)  // skip WAIT and HLT
    //            use = false;
    //        if (_tests[i].queueFiller() != 0)  // skip tests with non-standard queue fillers
    //            use = false;
    //        Instruction inst = _tests[i].instruction(0);
    //        int o = inst.opcode();
    //        if ((o & 0xe0) == 0x60 || (o >= 0xe0 && o < 0xe4))  // skip conditional jump, time depends on flags
    //            use = false;
    //        if (o == 0xf2 || o == 0xf3)  // skip REP loops, need setup
    //            use = false;
    //        int op = inst.modrm() & 0x38;
    //        if ((o >= 0xf6 && o < 0xf8) || op >= 0x20)  // multiplies and divides are not the way to go
    //            use = false;
    //        if (o >= 0xfe && op >= 0x10 && op < 0x30)  // CALL/JMP EA are awkward
    //            use = false;

    //        useTest[i] = use;
    //    }

    //    // Look for a nopcount column that has the same timings as another one
    //    Array<bool> useNop(nopCounts);
    //    for (int c1 = 0; c1 < nopCounts; ++c1) {
    //        useNop[c1] = true;
    //        for (int c2 = 0; c2 < nopCounts; ++c2) {
    //            if (c2 == c1)
    //                continue;
    //            bool found = true;
    //            for (int t = 0; t < noppingTests; t += nopCounts) {
    //                if (!useTest[t])
    //                    continue;
    //                if (cycleCounts[t + c1] != cycleCounts[t + c2]) {
    //                    found = false;
    //                    //console.write("To distinguish between " + decimal(c1) + " and " + decimal(c2) + " look at test " + decimal(t) + " ");
    //                    //_tests[t].write();
    //                    break;
    //                }
    //            }       
    //            if (found) {
    //                printf("nopcounts %i and %i are identical\n", c1, c2);
    //            }
    //        }
    //    }
    //    //useNop[12] = false;
    //    useNop[13] = false;
    //    useNop[14] = false;
    //    useNop[15] = false;
    //    File("nopping.dat").save(cycleCounts);

    //    AppendableArray<int> uniqueTests;
    //    for (int i = 0; i < noppingTests; i += nopCounts) {
    //        if (!useTest[i])
    //            continue;
    //        bool isUnique = true;
    //        for (int j = 0; j < i; j += nopCounts) {
    //            if (!useTest[j])
    //                continue;
    //            bool isMatch = true;
    //            for (int k = 0; k < nopCounts; ++k) {
    //                if (!useNop[k])
    //                    continue;
    //                if (cycleCounts[i + k] != cycleCounts[j + k]) {
    //                    isMatch = false;
    //                    break;
    //                }
    //            }
    //            if (isMatch) {
    //                isUnique = false;
    //                break;
    //            }
    //        }
    //        if (isUnique)
    //            uniqueTests.append(i);
    //    }
    //    printf("Unique tests found: %i\n", uniqueTests.count());
    //    {
    //        auto ws = File("uniques.dat").openWrite();
    //        for (int i = 0; i < uniqueTests.count(); ++i) {
    //            for (int j = 0; j < nopCounts; ++j) {
    //                if (!useNop[j])
    //                    continue;
    //                Byte b = cycleCounts[uniqueTests[i] + j];
    //                ws.write(b);
    //            }
    //        }
    //    }

    //    for (int setSize = 1;; ++setSize) {
    //        printf("Trying set size %i\n",setSize);
    //        Array<int> setMembers(setSize);
    //        for (int i = 0; i < setSize; ++i)
    //            setMembers[i] = i;
    //        bool tryNextSize = false;
    //        do {
    //            bool working = true;
    //            for (int i = 0; i < nopCounts; ++i) {
    //                if (!useNop[i])
    //                    continue;
    //                for (int j = i + 1; j < nopCounts; ++j) {
    //                    if (!useNop[j])
    //                        continue;
    //                    bool canDistinguish = false;
    //                    for (int k = 0; k < setSize; ++k) {
    //                        int t = uniqueTests[setMembers[k]];
    //                        if (cycleCounts[t + i] != cycleCounts[t + j]) {
    //                            canDistinguish = true;
    //                            break;
    //                        }
    //                    }
    //                    if (!canDistinguish) {
    //                        i = nopCounts;
    //                        working = false;
    //                        break;
    //                    }
    //                }
    //            }
    //            if (working) {
    //                console.write("Found a set that works:\n");
    //                for (int i = 0; i < setSize; ++i) {
    //                    int u = setMembers[i];
    //                    int t = uniqueTests[u];
    //                    console.write(decimal(t) + ": ");
    //                    _tests[t].write();
    //                    console.write("\n");
    //                }
    //                console.write("\n");
    //                break;
    //            }
    //            bool foundDigit = false;
    //            for (int d = setSize - 1; d >= 0; --d) {
    //                if (d == 0)
    //                    printf(".");
    //                ++setMembers[d];
    //                if (setMembers[d] != uniqueTests.count() - ((setSize - 1) - d)) {
    //                    foundDigit = true;
    //                    for (int i = d + 1; i < setSize; ++i)
    //                        setMembers[i] = setMembers[i - 1] + 1;
    //                    break;
    //                }
    //            }
    //            if (!foundDigit) {
    //                tryNextSize = true;
    //                break;
    //            }
    //        } while (true);
    //        if (!tryNextSize)
    //            break;
    //    }
    }
private:
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
    void dumpCache(AppendableArray<Test> bunch, int count)
    {
        for (int i = 0; i < count; ++i) {
            Test t = bunch[i];
            _cache.setTime(t, t.cycles());
        }
        _cache.save(_cacheFile);
    }

    File _cacheFile;
    Cache _cache;

    TestGenerator _generator;
};
