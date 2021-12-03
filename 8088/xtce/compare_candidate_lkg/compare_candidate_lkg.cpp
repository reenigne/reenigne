#include "alfe/main.h"
#include "alfe/space.h"
#include "alfe/hash_table.h"
#include "alfe/set.h"
#include <random>

#include "../xtce.h"
#include "../xtce_lkg.h"
//#include "../gentests.h"
#include "../gentests2.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        File("runstub.bin").readIntoArray(&_runStub);

        LKG::CPUEmulator lkgEmulator;
        _lkgEmulator = &lkgEmulator;
        CPUEmulator candidateEmulator;
        _candidateEmulator = &candidateEmulator;

        console.write("Running tests\n");

        int totalCount = 0;
        int lkgCycles = 0;
        int candidateCycles = 0;
        Test t;
        bool done = false;
        do {
            if (totalCount % 100000 == 0)
                printf("%i\n", totalCount);
            if (_generator.finished()) {
                done = true;
                break;
            }
            t = _generator.getNextTest();
            if (totalCount >= 0 /* 98800000*/) {
                lkgCycles = expected(_lkgEmulator, t);
                candidateCycles = expected(_candidateEmulator, t) + (t.refreshPeriod() != 0 ? 1 : 0);
            }
            ++totalCount;
        } while (lkgCycles == candidateCycles);

        console.write("Tests passing: " + decimal(totalCount - 1) + "\n");
        _generator.dumpFailed(t);
        if (done)
            return;
        t.write();

        String lkgLog = log(_lkgEmulator, t);
        String candidateLog = log(_candidateEmulator, t);

        //String expected1 = lkgLog;
        //String observed = candidateLog;
        //    
        //String expected1;

        //String observed;
        //CharacterSource e(expected);
        //int skipLines = 4;
        //do {
        //    int oc = s.get();
        //    if (oc == -1)
        //        break;
        //    if (oc == '\n')
        //        --skipLines;
        //} while (skipLines > 0);

        //int c = t.cycles() - 5;
        //int line = 0;
        //int column = 1;

        //do {
        //    int ec = e.get();
        //    if (ec == -1)
        //        break;
        //    ++column;
        //    /*if ((column >= 7 && column < 20) || column >= 23)*/ {
        //        //if (line < c)
        //        expected1 += codePoint(ec);
        //    }
        //    if (ec == '\n') {
        //        ++line;
        //        column = 1;
        //    }
        //} while (true);
        //line = 0;
        //column = 1;
        //do {
        //    int oc = s.get();
        //    if (oc == -1)
        //        break;
        //    ++column;
        //    /*if ((column >= 7 && column < 20) || column >= 23)*/ {
        //        //if (line < c)
        //        observed += codePoint(oc);
        //    }
        //    if (oc == '\n') {
        //        ++line;
        //        if (column == 1)
        //            break;
        //        column = 1;
        //    }
        //} while (true);

        File("lkg_c.txt").openWrite().write(lkgLog);
        File("candidate.txt").openWrite().write(candidateLog);

        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

        STARTUPINFO si;
        ZeroMemory(&si, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);

        NullTerminatedWideString data(
            String("q candidate.txt lkg_c.txt"));

        IF_FALSE_THROW(CreateProcess(NULL, data, NULL, NULL, FALSE,
            0, NULL, NULL, &si, &pi) != 0);

        exit(1);
    }
private:
    template<class T> String log(T* emulator, Test test)
    {
        initCPU(emulator, test);
        emulator->setExtents(_logSkip, 4096, 4096, _stopIP, _stopSeg, _timeIP1, _timeSeg1);
        emulator->run();
        return emulator->log();
    }
    template<class T> int expected(T* emulator, Test test)
    {
        initCPU(emulator, test);
        emulator->setExtents(0, 0, 4096, _stopIP, _stopSeg, _timeIP1, _timeSeg1);
        try {
            emulator->run();
        }
        catch (...) {}
        return emulator->cycle();
    }


    template<class T> void initCPU(T* emulator, Test test)
    {
        emulator->reset();

        emulator->getRegisters()[2] =  // DX
            test.refreshPeriod() + (test.refreshPhase() << 8);
        Word* segmentRegisters = emulator->getSegmentRegisters();
        for (int i = 0; i < 4; ++i)
            segmentRegisters[i] = testSegment;
        Word seg = testSegment + 0x1000;
        Byte* ram = emulator->getRAM();

        if (test.refreshPeriod() == 0) {
            emulator->stubInit();
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
            _logSkip = 1; // 1041 + 92;// + 17;
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

    LKG::CPUEmulator* _lkgEmulator;
    CPUEmulator* _candidateEmulator;
    Array<Byte> _runStub;
    int _logSkip;
    int _stopIP;
    int _stopSeg;
    int _timeIP1;
    int _timeSeg1;
    int _bytesUsed;

    TestGenerator _generator;
};
