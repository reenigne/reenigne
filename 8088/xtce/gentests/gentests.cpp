#include "alfe/main.h"
#include "alfe/space.h"
#include "alfe/hash_table.h"
#include "alfe/set.h"
#include <random>

#define GENERATE_NEWFAILS 0

#if GENERATE_NEWFAILS
#include "fails.h"
#endif

#include "../xtce_microcode.h"
#include "../gentests.h"

#define USE_REAL_HARDWARE 1

class Program : public ProgramBase
{
public:
    void run()
    {
        Array<Byte> testProgram;
        File("runtests.bin").readIntoArray(&testProgram);
        File("runstub.bin").readIntoArray(&_runStub);

        CPUEmulator emulator;
        _emulator = &emulator;

#if GENERATE_NEWFAILS
        nopCounts = 19;
        int bunchLength = 0;
        int maxBytesUsed = 0;
        AppendableArray<Test> newFails;
        for (int i = 0; ; ++i) {
            Test t = _generator.getNextTest();
            if (_generator._refreshPeriod == 2)
                break;
            if (!_generator.inFailsArray())
                continue;
            int cycles = expected(t);
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
            bunchLength += t.length();
            maxBytesUsed = max(maxBytesUsed, _bytesUsed);
            //if (newFails.count() - 1 == 511) {
            //    console.write(log(t));
            //    exit(1);
            //}
        }
        console.write("Max bytes used: " + decimal(maxBytesUsed) + "\n");
        console.write("Found tests: " + decimal(newFails.count()) + ":\n");
        for (int i = 0; i < newFails.count(); ++i)
            newFails[i].write();

        {
            Array<Byte> output(bunchLength + 2);
            Byte* p = &output[0];
            *p = bunchLength;
            p[1] = bunchLength >> 8;
            p += 2;
            for (int i = 0; i < newFails.count(); ++i) {
                Test t = newFails[i];
                int cycles = t.cycles() - 210;
                p[0] = cycles;
                p[1] = cycles >> 8;
                p += 2;
                t.output(p);
                p += t.length() - 2;
            }
            auto h = File("tests.bin").openWrite();
            h.write(output);
        }

        nopCounts = 17;
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
                //if (totalCount % 100 == 99)
                //    printf(".");
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
                int cycles = expected(t);
                Instruction instruction = t.instruction(0);

                if (totalCount % 10000 == 9999) {
                    console.write(decimal(totalCount) + ": ");
                    t.write();
                }

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
            console.write(decimal(totalCount) + ": ");
            bunch[0].write();

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
                IF_TRUE_THROW(WaitForSingleObject(hProcess, 3*60*1000) !=
                    WAIT_OBJECT_0, Exception("XT Timed out"));
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
        } catch (...) { }
        return _emulator->cycle() + (test.refreshPeriod() == 0 ? 0 : 3);
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
            ram[3*4 + 0] = 0x00;  // int 3 handler at 0x400
            ram[3*4 + 1] = 0x04;
            ram[3*4 + 2] = 0x00;
            ram[3*4 + 3] = 0x00;
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

    CPUEmulator* _emulator;
    Array<Byte> _runStub;
    int _logSkip;
    int _stopIP;
    int _stopSeg;
    int _timeIP1;
    int _timeSeg1;
    int _bytesUsed;

    TestGenerator _generator;
};
