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
        _cycles = 0;
        Byte* stopP = _test.outputCode(&_ram[0x10a80]);
        _stopIP = (stopP - &_ram[0]) - 0x10a82;
        ax() = 0;
        cx() = 0;
        dx() = 0;
        bx() = 0;
        sp() = 0;
        bp() = 0;
        si() = 0;
        di() = 0;
        es() = 0x10a8;
        cs() = 0x10a8;
        ss() = 0x10a8;
        ds() = 0x10a8;
        do {
            //    String line = String(decimal(_cycle)).alignRight(5) + " ";
            //    switch (_busState) {
            //        case t1:
            //            line += "T1 " + hex(_busAddress, 5, false) + " ";
            //            break;
            //        case t2:
            //            line += "T2 ";
            //            if (_ioInProgress == ioWrite)
            //                line += "M<-" + hex(_busData, 2, false) + " ";
            //            else
            //                line += "      ";
            //            break;
            //        case t3: line += "T3       "; break;
            //        case tWait: line += "Tw       "; break;
            //        case t4:
            //            line += "T4 ";
            //            if (_ioInProgress == ioWrite)
            //                line += "      ";
            //            else
            //                if (_abandonFetch)
            //                    line += "----- ";
            //                else
            //                    line += "M->" + hex(_busData, 2, false) + " ";
            //            break;
            //        case tIdle: line += "         "; break;
            //    }
            if (_newInstruction) {
                UInt32 a = codeAddress(_newIP);
                if (!_visited[a]) {
                    console.write(String(decimal(_cycle)).alignRight(5) + " " +
                        hex(csQuiet(), 4, false) + ":" +
                        hex(_newIP, 4, false) + " " +
                        _disassembler.disassemble(_newIP) + "\n");
                }
                _visited[a] = true;
                //        line += hex(csQuiet(), 4, false) + ":" + hex(_newIP, 4, false)
                //            + " " + _disassembler.disassemble(_newIP);
            }
            //    line = line.alignLeft(50);
            //    for (int i = 0; i < 8; ++i) {
            //        line += _byteRegisters[i].text();
            //        line += _wordRegisters[i].text();
            //        line += _segmentRegisters[i].text();
            //    }
            //    line += _flags.text();
            //    //if(_newInstruction)
            //        console.write(line + "\n");
            _newInstruction = false;
        }

        ++_cycle;
    } while (true);
    }

    bool _logging;
    Test _test;
    String _log;
    int _cycles;
    Array<Byte> _ram;

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
    int _stopIP;

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
    UInt8 _opcode;

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