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
        auto n = instructions.size();
        auto p = instructions.begin();
        for (auto i = 0; i < n; ++i)
            _instructions.append(p[i]);
        _refreshPeriod = refreshPeriod;
        _refreshPhase = refreshPhase;
        _nops = nopCount & 31;
        _queueFiller = nopCount >> 5;
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
            case 17:
                return 11;
            case 18:
                return 18;
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
            case 17:
                for (int i = 0; i < 11; ++i)
                    p[i] = 0x90;
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
    bool sameInstructions(const Test& other) const
    {
        if (_queueFiller != other._queueFiller || _nops != other._nops ||
            _refreshPeriod != other._refreshPeriod ||
            _refreshPhase != other._refreshPhase)
            return false;
        int c = _instructions.count();
        if (c != other._instructions.count())
            return false;
        for (int i = 0; i < c; ++i)
            if (_instructions[i] != other._instructions[i])
                return false;
        return true;
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

static int nopCounts = 17;

struct Measurements
{
    Measurements()
    {
        for (int i = 0; i < 17 /* nopCounts */; ++i)
            _cycles[i] = -1;
    }
    SInt16 _cycles[17 /* nopCounts */];
};

#if 1
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
#endif

static const int refreshPeriods[19] = {0, 18, 19, 17, 16, 15, 14, 13, 12, 11,
    10, 9, 8, 7, 6, 5, 4, 3, 2};

static const Byte modrms[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0xc0};

static const Byte modrms2[] = {
    0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
    0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff};

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
#if GENERATE_NEWFAILS
        _inFailsArray = false;
#endif
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
                        t.addInstruction(Instruction(_opcode, modrms2[_m]));
                        if ((modrms2[_m] & 0x30) == 0x30) {
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
                    case 7:  // INC SI, POP ES (generated by a bug but found an error)
                        t.addInstruction(Instruction(0x46));
                        t.addInstruction(Instruction(0x07));
                        break;
                    case 8:  // IN/OUT to DMA port
                        {
                            Instruction i(_opcode);
                            i.setImmediate(0);
                            t.addInstruction(i);
                        }
                        break;
                    case 9:  // REP without string instruction
                        t.addInstruction(Instruction(0xf2));
                        break;

                }
                t.setNops(_nopCount);
                switch (_suffix) {
                    case 3:
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
                    case 0:
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
                    case 9:
                        t.addInstruction(Instruction(0, 6));
                        break;
                }
                t.setRefreshPeriod(refreshPeriods[_refreshPeriod]);
                t.setRefreshPhase(_refreshPhase);
#if GENERATE_NEWFAILS
                switch (_subsection) {
                    case 4:  // LOOP with CX==1
                        for (int i = 0; i < sizeof(loopFails)/sizeof(loopFails[0]); ++i) {
                            auto p = &loopFails[i];
                            if (p->sameInstructions(t))
                                _inFailsArray = true;
                        }
                        break;
                    case 5:  // String operations with various counts
                        for (int i = 0; i < sizeof(repFails)/sizeof(repFails[0]); ++i) {
                            auto p = &repFails[i];
                            if (p->_test.sameInstructions(t) && p->_w1 == _count)
                                _inFailsArray = true;
                        }
                        break;
                    case 0:  // Basic tests
                    case 1:  // CBW/CWD tests
                    case 2:  // INTO overflow test
//                    case 3:  // Shift/rotate with various counts
                    case 6:  // Math instructions with all registers
                    case 7:  // INC SI, POP ES
                    case 8:  // IN/OUT to DMA port
                    case 9:  // REP without string instruction
                        for (int i = 0; i < sizeof(mainFails)/sizeof(mainFails[0]); ++i) {
                            auto p = &mainFails[i];
                            if (p->sameInstructions(t))
                                _inFailsArray = true;
                        }
                }
#endif
                break;
            case 3:  // 1000 input pairs for each multiply instruction
                t.setNops(0);
                t.addInstruction(Instruction(_opcode, _m));
                {
                    Word a = _d(_generator);
                    t.preamble(0xb8);
                    t.preamble(a & 0xff);
                    t.preamble(a >> 8);  // MOV AX,a
                    Word b = _d(_generator);
                    t.preamble(0xba);
                    t.preamble(b & 0xff);
                    t.preamble(b >> 8);  // MOV DX,a
                    t.setQueueFiller(1);
#if GENERATE_NEWFAILS
                    for (int i = 0; i < sizeof(mulFails)/sizeof(mulFails[0]); ++i) {
                        auto p = &mulFails[i];
                        if (p->_test.sameInstructions(t) && a == p->_w1 && b == p->_w2)
                            _inFailsArray = true;
                    }
#endif
                }
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
                    Word a, b, c;
                    if (_count == 1000) {
                        a = 0x37FC;
                        b = 0x0000;
                        c = 0x008D;
                    }
                    else {
                        a = _d(_generator);
                        b = _d(_generator);
                        c = _d(_generator);
                    }
                    t.preamble(0xb8);
                    t.preamble(a & 0xff);
                    t.preamble(a >> 8);  // MOV AX,a
                    t.preamble(0xba);
                    t.preamble(b & 0xff);
                    t.preamble(b >> 8);  // MOV DX,a
                    t.preamble(0xbb);
                    t.preamble(c & 0xff);
                    t.preamble(c >> 8);  // MOV BX,a
                    t.setQueueFiller(1);
#if GENERATE_NEWFAILS
                    for (int i = 0; i < sizeof(divFails)/sizeof(divFails[0]); ++i) {
                        auto p = &divFails[i];
                        if (_opcode == 0xf6) {
                            if (p->_test.sameInstructions(t) && a == p->_w1 && (c & 0xff) == p->_w2)
                                _inFailsArray = true;
                        }
                        else {
                            if (p->_test.sameInstructions(t) && a == p->_w1 && b == p->_w2 && c == p->_w3)
                                _inFailsArray = true;
                        }
                    }
#endif
                }
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
                t.setQueueFiller(1);
                {
                    Word a = _d(_generator);
                    t.preamble(0xb8);
                    t.preamble(a & 0xff);
                    t.preamble(a >> 8);  // MOV AX,a
#if GENERATE_NEWFAILS
                    for (int i = 0; i < sizeof(aamdFails)/sizeof(aamdFails[0]); ++i) {
                        auto p = &aamdFails[i];
                        if (p->_test.sameInstructions(t) && a == p->_w1)
                            _inFailsArray = true;
                    }
#endif
                }
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
        //if (_section == 6 || _section == 7)
        //    endNopCount = 14;
        ++_nopCount;
        if (_nopCount < endNopCount)
            return true;
        _nopCount = 0;
        ++_suffix;
        int maxSuffix = 10; //9;
        if (_section == 6)
            maxSuffix = 6; //3;
        if (_section == 7)
            maxSuffix = 9; //2;
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
            if (_count < 6 /*5*/)
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
            _m = 0;
            return true;
        }
        if (_subsection == 6) {  // Math instructions with all registers
            ++_m;
            if (_m < 56)
                return true;
            _m = 0;
            ++_opcode;
            if (_opcode < 0xf8)
                return true;
            _subsection = 7;
            _opcode = 0;
            _m = 0;
            _r = 0;
            return true;
        }
        if (_subsection == 7) {  // INC SI, POP DS
            _subsection = 8;
            _opcode = 0xe4;
            return true;
        }
        if (_subsection == 8) {  // IN/OUT to DMA port
            ++_opcode;
            if (_opcode < 0xe8)
                return true;
            _subsection = 9;
            _opcode = 0;
            return true;
        }
        if (_subsection == 9)  // REP without string instruction
            _subsection = 0;
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
            if (_count < 1001)
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

#if GENERATE_NEWFAILS
    bool _inFailsArray;
public:
#endif
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

    std::mt19937 _generator;
    std::uniform_int_distribution<int> _d;

    AppendableArray<Test> _functional;

    AppendableArray<Test> _fails;
};

