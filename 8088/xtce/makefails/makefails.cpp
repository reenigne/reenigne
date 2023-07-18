#include "alfe/main.h"
#include "alfe/space.h"
#include "alfe/hash_table.h"
#include "alfe/set.h"
#include <random>

#define GENERATE_NEWFAILS 1

#include "../gentests.h"

class Program : public ProgramBase
{
public:
    class ProcessFails
    {
    public:
        void operator()(File file)
        {
            auto s = file.tryOpenRead();
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

                    console.write(file.path() + " has ");
                    t.write();
                }
            }
        }
        AppendableArray<Test> _fails;
    };
    void run()
    {
        ProcessFails f;
        Directory("../gentests").applyToContents(f, false, String("fails*.dat"));
        //Set<Test> failsDatFails;
        //for (auto t : f._fails)
        //    failsDatFails.add(t);

        TestGenerator generator;

        int nopCounts = 19;
        _bunchLength = 0;
        int maxBytesUsed = 0;
        for (int i = 0; ; ++i) {
            Test t = generator.getNextTest();
            if (generator.refreshPeriod() == 2)
                break;
            if (!generator.inFailsArray()) {
                bool found = false;
                for (auto tt : f._fails) {
                    if (tt.equalIncludingNops(t)) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    console.write("Need to add to fails.h: ");
                    t.write2();
                }
                continue;
            }
            //int cycles = expected(t);
            //t.setCycles(cycles);
            bool alreadyThere = false;
            for (int j = 0; j < _newFails.count(); ++j) {
                if (_newFails[j].equalIncludingNops(t)) {
                    alreadyThere = true;
                    break;
                }
            }
            if (alreadyThere) {
                console.write("Test duplicated: ");
                t.write();
            }
            else {
                _newFails.append(t);
                _newW1.append(generator.w1());
                _newW2.append(generator.w2());
                _newW3.append(generator.w3());
            }
            _bunchLength += t.length();
            //maxBytesUsed = max(maxBytesUsed, _bytesUsed);
            //if (newFails.count() - 1 == 511) {
            //    console.write(log(t));
            //    exit(1);
            //}
        }
        //console.write("Max bytes used: " + decimal(maxBytesUsed) + "\n");
        console.write("Found tests: " + decimal(_newFails.count()) + ":\n");
        for (int i = 0; i < _newFails.count(); ++i)
            _newFails[i].write();

        {
            Array<Byte> output(_bunchLength + 2);
            Byte* p = &output[0];
            *p = _bunchLength;
            p[1] = _bunchLength >> 8;
            p += 2;
            for (int i = 0; i < _newFails.count(); ++i) {
                Test t = _newFails[i];
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

        getLogs();
        for (int i = 0; i < _newFails.count(); ++i) {
            String emulated = 
        }

    }
    void getLogs()
    {
        Array<Byte> testProgram;
        File("runtests.bin").readIntoArray(&testProgram);
        for (int i = 0; i < _newFails.count(); ++i) {
            File logFile = log(i);
            if (logFile.exists())
                continue;

            Test t = _newFails[i];
            console.write("Getting log for test: ");
            t.write2();
            Array<Byte> output(t.length() + 2);
            Byte* p = &output[0];
            *p = _bunchLength;  // should be t.length() here and on next line
            p[1] = _bunchLength >> 8;
            p += 2;
            p[0] = 0;
            p[1] = 0x80;
            p += 2;
            t.output(p);
            p += t.length() - 2;
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
                IF_TRUE_THROW(WaitForSingleObject(hProcess, 3 * 60 * 1000) !=
                    WAIT_OBJECT_0, Exception("XT Timed out"));
            }
            String result = File("runtests.output").contents();
            CharacterSource s(result);
            do {
                if (parse(&s, "FAIL")) {
                    Rational rr;
                    Space::parse(&s);
                    if (!Space::parseNumber(&s, &rr))
                        throw Exception("Cannot parse number of failing test");
                    if (!Space::parseOperator(&s, ","))
                        throw Exception("Expected a comma");
                    if (!Space::parseNumber(&s, &rr))
                        throw Exception("Cannot parse observed cycle count");

                    String observed;
                    int skipLines = 4;
                    do {
                        int oc = s.get();
                        if (oc == -1)
                            break;
                        if (oc == '\n')
                            --skipLines;
                    } while (skipLines > 0);

                    int line = 0;
                    int column = 1;
                    do {
                        int oc = s.get();
                        if (oc == -1)
                            break;
                        ++column;
                        /*if ((column >= 7 && column < 20) || column >= 23)*/
                        {
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

                    logFile.openWrite().write(observed);
                    break;
                }
                if (parse(&s, "PASS")) {
                    t.write2();
                    throw Exception("Test unexpectedly passed!");
                }
                int c = s.get();
                if (c == -1)
                    throw Exception("Test was inconclusive");
            } while (true);
        }
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
    File log(int i)
    {
        String r = _newFails[i].hexName();
        if (_newW1[i] != 0xffff) {
            r += "_";
            if (_newW1[i] > 9)
                r += hex(_newW1[i], 4, false);
            else
                r += decimal(_newW1[i]);
            if (_newW2[i] != 0xffff) {
                r += "_";
                if (_newW2[i] > 9)
                    r += hex(_newW2[i], 4, false);
                else
                    r += decimal(_newW2[i]);
                if (_newW3[i] != 0xffff) {
                    r += "_";
                    if (_newW3[i] > 9)
                        r += hex(_newW3[i], 4, false);
                    else
                        r += decimal(_newW3[i]);
                }
            }
        }
        return File(r + ".txt");
    }

    AppendableArray<Test> _newFails;
    AppendableArray<Word> _newW1;
    AppendableArray<Word> _newW2;
    AppendableArray<Word> _newW3;
    int _bunchLength;
};
