#include "alfe/main.h"
#include "alfe/space.h"
#include "alfe/hash_table.h"
#include "alfe/set.h"
#include <random>

#include "../xtce_lkg.h"
#include "../gentests.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        Array<Byte> testProgram;
        File("runtests.bin").readIntoArray(&testProgram);
        File("runstub.bin").readIntoArray(&_runStub);

        LKG::CPUEmulator emulator;
        _emulator = &emulator;

        console.write("Running tests\n");

        int maxTests = 1000;
        int availableLength = 0xf300 - testProgram.count();

        int firstTest = -1;

        if (_arguments.count() >= 2) {
            CharacterSource s(_arguments[1]);
            Rational r;
            if (Space::parseNumber(&s, &r))
                firstTest = r.floor();
        }

        Test retained;
        bool haveRetained = false;
        int totalCount = 0;
        do {
            int bunchLength = 0;
            AppendableArray<Test> bunch;
            AppendableArray<int> runningTests;
            do {
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
                int cycles = 0;
                if (totalCount > firstTest)
                    cycles = expected(t);
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
                //int cached = _cache.getTime(t);
                //if (cycles != cached) {
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
                //    //if (cached != -1 && !reRunAllBad) {
                //    //    console.write("\nFailing test " +
                //    //        decimal(totalCount - 1) + "\n");

                //    //    // This test will fail unless the cache is bad.
                //    //    // Just run the one to get a sniffer log.
                //    //    expectedFail = true;
                //    //    break;
                //    //}
                //    //if (bunch.count() >= maxTests)
                //    //    break;
                //}
            } while (true);
            console.write(decimal(totalCount) + "\n");
            if (totalCount <= firstTest)
                continue;     

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
                IF_FALSE_THROW(WaitForSingleObject(hProcess, 3 * 60 * 1000) ==
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
                    //int cachedCycles = _cache.getTime(t);

                    //if (cachedCycles != observedCycles) {
                    //    if (cachedCycles == -1)
                    //        _cache.setTime(t, observedCycles);
                    //    else {
                    //        console.write("Cache has wrong data for test: " + decimal(cachedCycles) + " cached, " + decimal(observedCycles) + " observed. Test is: ");
                    //    }
                    //}
                    //else
                    //    console.write("Test failed but cache was correct: " + decimal(cachedCycles) + " cached, " + decimal(observedCycles) + " observed, " + decimal(t.cycles()) + " computed. Test is: ");

                    console.write(decimal(n) + "\n");
                    t.write();
                    //dumpCache(bunch, index);
                    _generator.dumpFailed(t);

                    String expected = log(t);
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
            //dumpCache(bunch, bunch.count());
            //if (expectedFail) {
            //    reRunAllBad = true;
            //    console.write("Found incorrect data in cache, re-running all "
            //        "failing tests at once.\n");
            //}

            //if (maxTests < 1000000)
            //    maxTests *= 2;
        } while (true);

        console.write("Tests passing: " + decimal(totalCount) + "\n");
        _generator.dumpFailed(Test());
    }
private:
    String log(Test test)
    {
        initCPU(test);
        _emulator->setExtents(_logSkip, 4096, 4096, _stopIP, _stopSeg, _timeIP1, _timeSeg1);
        _emulator->run();
        return _emulator->log();
    }
    int expected(Test test)
    {
        initCPU(test);
        _emulator->setExtents(0, 0, 4096, _stopIP, _stopSeg, _timeIP1, _timeSeg1);
        try {
            _emulator->run();
        }
        catch (...) {}
        return _emulator->cycle();
    }


    void initCPU(Test test)
    {
        _emulator->reset();

        _emulator->getRegisters()[2] =  // DX
            test.refreshPeriod() + (test.refreshPhase() << 8);
        Word* segmentRegisters = _emulator->getSegmentRegisters();
        for (int i = 0; i < 4; ++i)
            segmentRegisters[i] = testSegment;
        Word seg = testSegment + 0x1000;
        Byte* ram = _emulator->getRAM();

        if (test.refreshPeriod() == 0) {
            _emulator->stubInit();
            ram[3 * 4 + 0] = 0x00;  // int 3 handler at 0x400
            ram[3 * 4 + 1] = 0x04;
            ram[3 * 4 + 2] = 0x00;
            ram[3 * 4 + 3] = 0x00;
            ram[0x400] = 0x83;
            ram[0x401] = 0xc4;
            ram[0x402] = 0x04;  // ADD SP,+4
            ram[0x403] = 0x9d;  // POPF
            ram[0x404] = 0xcb;  // RETF

            Byte* r = ram + (seg << 4);
            Byte* stopP = test.outputCode(r);
            _bytesUsed = stopP - r;
            _stopIP = stopP - (r + 2);
            _logSkip = 1;
            for (int i = 0; i < 4; ++i)
                segmentRegisters[i] = seg;
        }
        else {
            Byte* ram1 = ram + (testSegment << 4);
            for (int i = 0; i < _runStub.count(); ++i)
                ram1[i] = _runStub[i];
            _stopIP = 0xd1;
            Byte* r = ram + (seg << 4);
            Byte* stopP = test.outputCode(r);
            _bytesUsed = stopP - r;
            _logSkip = 1041 + 92;// + 17;
            seg = testSegment;
        }

        _timeIP1 = test.startIP();
        _stopSeg = seg;
        _timeSeg1 = testSegment + 0x1000;
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
    //void dumpCache(AppendableArray<Test> bunch, int count)
    //{
    //    for (int i = 0; i < count; ++i) {
    //        Test t = bunch[i];
    //        _cache.setTime(t, t.cycles());
    //    }
    //    _cache.save(_cacheFile);
    //}

    //File _cacheFile;
    //Cache _cache;

    LKG::CPUEmulator* _emulator;
    Array<Byte> _runStub;
    int _logSkip;
    int _stopIP;
    int _stopSeg;
    int _timeIP1;
    int _timeSeg1;
    int _bytesUsed;

    TestGenerator _generator;
};
