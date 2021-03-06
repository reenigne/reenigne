#include "alfe/main.h"
#include "alfe/hash_table.h"
#include "alfe/rational.h"
#include "alfe/space.h"
#include <inttypes.h>

enum TestState
{
    pass,
    fail,
    xpass,
    kpass,
    xfail,
    kfail,
    unresolved,
    untested,
    unsupported,
    warning,
    error,
    missing
};

class TestResult
{
public:
    TestResult() : _state(missing), _bytes(-1), _cycles(-1) { }
    TestState _state;
    int _bytes;
    int _cycles;
    bool passed() { return _state == pass || _state == xpass; }
};

class TestResults
{
public:
    TestResult _left;
    TestResult _right;
};

class Program : public ProgramBase
{
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

    void parseTestLog(CharacterSource s, bool right)
    {
        TestResult result;
        do {
            CharacterSource s2 = s;
            if (s2.get() == -1)
                return;
            bool eof;
            if (parse(&s, "*** ")) {
                if (parse(&s, "Bytes: ")) {
                    Rational r;
                    if (Space::parseNumber(&s, &r))
                        result._bytes = r.value<int>();
                    continue;
                }
                if (parse(&s, "Cycles: ")) {
                    Rational r;
                    if (Space::parseNumber(&s, &r))
                        result._cycles = r.value<int>();
                    continue;
                }
            }
            int i;
            for (i = 0; i < 11; ++i) {
                if (parse(&s, _states[i])) {
                    result._state = _testStates[i];
                    String name = s.delimitString(_eol, &eof);
                    if (!right) {
                        TestResults results;
                        results._left = result;
                        _results.add(name, results);
                    }
                    else {
                        if (_results.hasKey(name))
                            _results[name]._right = result;
                        else {
                            TestResults results;
                            results._right = result;
                            _results.add(name, results);
                        }
                    }
                    result = TestResult();
                    break;
                }
            }
            if (i == 11)
                s.delimitString(_eol, &eof);
        } while (true);
    }

    void run()
    {
        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] +
                " <first log file name> <second log file name>\n");
            return;
        }
        _states[0] = "PASS";        _testStates[0] = pass;
        _states[1] = "FAIL";        _testStates[1] = fail;
        _states[2] = "XPASS";       _testStates[2] = xpass;
        _states[3] = "KPASS";       _testStates[3] = kpass;
        _states[4] = "XFAIL";       _testStates[4] = xfail;
        _states[5] = "KFAIL";       _testStates[5] = kfail;
        _states[6] = "UNRESOLVED";  _testStates[6] = unresolved;
        _states[7] = "UNTESTED";    _testStates[7] = untested;
        _states[8] = "UNSUPPORTED"; _testStates[8] = unsupported;
        _states[9] = "WARNING";     _testStates[9] = warning;
        _states[10] = "ERROR";      _testStates[10] = error;
        for (int i = 0; i < 11; ++i)
            _states[i] += ": ";
        String l1 = File(_arguments[1], true).contents();
        String l2 = File(_arguments[2], true).contents();
        _eol = String(codePoint(10));
        parseTestLog(l1, false);
        parseTestLog(l2, true);

        double cyclesBefore = 0;
        double cyclesAfter = 0;
        double bytesBefore = 0;
        double bytesAfter = 0;
        int speedTests = 0;
        int sizeTests = 0;
        int fasterTests = 0;
        int slowerTests = 0;
        int largerTests = 0;
        int smallerTests = 0;
        double cyclesFaster = 0;
        double cyclesSlower = 0;
        double bytesLarger = 0;
        double bytesSmaller = 0;
        for (auto e : _results) {
            String name = e.key();
            CharacterSource s(name);
            int optimization = 0;
            do {
                int c = s.get();
                if (c == -1)
                    break;
                if (c == '-') {
                    c = s.get();
                    if (c == -1)
                        break;
                    if (c == 'O') {
                        c = s.get();
                        switch (c) {
                            case '1':
                            case '2':
                            case '3':
                                optimization = 1;
                                break;
                            case 's':
                                optimization = 2;
                                break;
                        }
                        break;
                    }
                }
            } while (true);
            TestResults results = e.value();
            if (optimization == 1) {
                if (!results._left.passed())
                    results._left._cycles = -1;
                if (!results._right.passed())
                    results._right._cycles = -1;
                if (results._left._cycles != -1) {
                    if (results._right._cycles != -1) {
                        cyclesBefore += results._left._cycles;
                        cyclesAfter += results._right._cycles;
                        int delta = results._right._cycles -
                            results._left._cycles;
                        if (results._right._cycles < results._left._cycles) {
                            console.write("Improvement from " +
                                decimal(results._left._cycles) + " to " +
                                decimal(results._right._cycles) + " (" +
                                decimal(delta) +
                                ") cycles in test " + e.key() + ".\n");
                            ++fasterTests;
                            cyclesFaster += delta;
                        }
                        if (results._right._cycles > results._left._cycles) {
                            console.write("Regression from " +
                                decimal(results._left._cycles) + " to " +
                                decimal(results._right._cycles) + " (" +
                                decimal(delta) +
                                ") cycles in test " + e.key() + ".\n");
                            ++slowerTests;
                            cyclesSlower += delta;
                        }
                        ++speedTests;
                    }
                    else {
                        cyclesBefore += results._left._cycles;
                        cyclesAfter += results._left._cycles;
                        ++speedTests;
                    }
                }
                else {
                    if (results._right._cycles != -1) {
                        cyclesBefore += results._right._cycles;
                        cyclesAfter += results._right._cycles;
                        ++speedTests;
                    }
                }
            }
            if (optimization == 2) {
                if (!results._left.passed())
                    results._left._bytes = -1;
                if (!results._right.passed())
                    results._right._bytes = -1;
                if (results._left._bytes != -1) {
                    if (results._right._bytes != -1) {
                        bytesBefore += results._left._bytes;
                        bytesAfter += results._right._bytes;
                        int delta = results._right._bytes -
                            results._left._bytes;
                        if (results._right._bytes < results._left._bytes) {
                            console.write("Improvement from " +
                                decimal(results._left._bytes) + " to " +
                                decimal(results._right._bytes) + " (" +
                                decimal(delta) +
                                ") bytes in test " + e.key() + ".\n");
                            ++smallerTests;
                            bytesSmaller += delta;
                        }
                        if (results._right._bytes > results._left._bytes) {
                            console.write("Regression from " +
                                decimal(results._left._bytes) + " to " +
                                decimal(results._right._bytes) + " (" +
                                decimal(delta) +
                                ") bytes in test " + e.key() + ".\n");
                            ++largerTests;
                            bytesLarger += delta;
                        }
                        ++sizeTests;
                    }
                    else {
                        bytesBefore += results._left._bytes;
                        bytesAfter += results._left._bytes;
                        ++sizeTests;
                    }
                }
                else {
                    if (results._right._bytes != -1) {
                        bytesBefore += results._right._bytes;
                        bytesAfter += results._right._bytes;
                        ++sizeTests;
                    }
                }
            }
        }
        printf("Speed:\n");
        printf("%i tests got faster by an average of %f cycles.\n",
            fasterTests, cyclesFaster/fasterTests);
        printf("%i tests got slower by an average of %f cycles.\n",
            slowerTests, cyclesSlower/slowerTests);
        printf("Cycles: Before %f, after: %f (%f%%) in %i tests\n",
            cyclesBefore/speedTests, cyclesAfter/speedTests,
            100*(cyclesAfter - cyclesBefore)/cyclesAfter, speedTests);
        printf("Size:\n");
        printf("%i tests got smaller by an average of %f bytes.\n",
            smallerTests, bytesSmaller/smallerTests);
        printf("%i tests got larger by an average of %f bytes.\n",
            largerTests, bytesLarger/largerTests);
        printf("Bytes: Before %f, after: %f (%f%%) in %i tests\n",
            bytesBefore/sizeTests, bytesAfter/sizeTests,
            100*(bytesAfter - bytesBefore)/bytesAfter, sizeTests);
    }
    String _eol;
    String _states[11];
    TestState _testStates[11];
    HashTable<String, TestResults> _results;
};