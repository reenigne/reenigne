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
        int l = 2;
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
        for (auto i : _instructions) {
            int l = i.length();
            i.output(p);
            p += l;
        }
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
        console.write("}, " + decimal((_queueFiller << 5) + _nops) + "}");
    }
private:
    int _queueFiller;
    int _nops;
    AppendableArray<Instruction> _instructions;
};

class Emulator
{
public:
    Emulator() : _logging(false) { }
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
    }

    bool _logging;
    Test _test;
    String _log;
    int _cycles;

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
                int jj = 0;
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
                int nl = totalLength + _tests[i].length() + 1;
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
            NullTerminatedWideString data(String("doitclient wcmd xtrun "
                "q:\\reenigne\\8088\\xtce\\gentests\\runtest.bin"));

            File outputFile("runtests.output");

            {
                PROCESS_INFORMATION pi;
                ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

                STARTUPINFO si;
                ZeroMemory(&si, sizeof(STARTUPINFO));
                si.cb = sizeof(STARTUPINFO);
                si.dwFlags = STARTF_USESTDHANDLES;
                auto h = outputFile.openWrite();
                si.hStdOutput = h;
                si.hStdError = h;

                IF_FALSE_THROW(CreateProcess(NULL, data, NULL, NULL, TRUE, 0,
                    "HOME=C:\\Users\\Andrew\0DOIT_HOST=prospero\0"
                    "SystemRoot=C:\Windows\0", NULL, &si, &pi) != 0);
                CloseHandle(pi.hThread);
                WindowsHandle hProcess = pi.hProcess;
                IF_FALSE_THROW(WaitForSingleObject(hProcess, 3*60*1000) ==
                    WAIT_OBJECT_0);
            }

            String result = outputFile.contents();
            CharacterSource s(result);
            if (parse(&s, "FAIL")) {
                Rational result;
                if (!Space::parseNumber(&s, &result))
                    throw Exception("Cannot parse number of failing test");
                int n = result.floor() + nextTest;

                console.write(decimal(n) + "\n");
                _tests[n].write();

                CharacterSource s2(s);
                if (!parse(&s2, "Program ended normally."))
                    throw Exception("runtests didn't end properly");
                String observed = s.subString(s.offset(), s2.offset());
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
            if (!parse(&s, "PASS"))
                throw Exception("Test was inconclusive");

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