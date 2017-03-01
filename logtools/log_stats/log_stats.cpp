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

    void parseTestLog(CharacterSource s)
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
                    _results.add(name, result);
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
        if (_arguments.count() < 1) {
            console.write("Syntax: " + _arguments[0] +
                " <log file name>\n");
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
        _eol = String(codePoint(10));
        parseTestLog(l1);

        double cycles = 0;
        double bytes = 0;
        int speedTests = 0;
        int sizeTests = 0;
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
            TestResult result = e.value();
            if (optimization == 1) {
                if (result.passed()) {
                    cycles += result._cycles;
                    ++speedTests;
                }
            }
            if (optimization == 2) {
                if (result.passed()) {
                    bytes += result._bytes;
                    ++sizeTests;
                }
            }
        }
        printf("%f %f %f %f\n",cycles,cycles/speedTests,bytes,bytes/sizeTests);
    }
    String _eol;
    String _states[11];
    TestState _testStates[11];
    HashTable<String, TestResult> _results;
};