#include "alfe/main.h"
#include "alfe/hash_table.h"
#include "alfe/rational.h"
#include "alfe/space.h"

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
            int sc = ss.get();
            int mc = ms.get();
            if (mc == -1) {
                *s = ss;
                return true;
            }
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
            if (parse(&s, "*** ")) {
                if (parse(&s, "Bytes: ")) {
                    Rational r;
                    if (Space::parseNumber(&s, &r))
                        result._bytes = r.value<int>();
                }
                if (parse(&s, "Cycles: ")) {
                    Rational r;
                    if (Space::parseNumber(&s, &r))
                        result._cycles = r.value<int>();
                }
            }
            bool eof;
            for (int i = 0; i < 11; ++i) {
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
            _states[i] += "; ";
        String l1 = File(_arguments[1], true).contents();
        String l2 = File(_arguments[2], true).contents();
        _eol = String(codePoint(10));
        parseTestLog(l1, false);
        parseTestLog(l2, true);
    }
    String _eol;
    String _states[11];
    TestState _testStates[11];
    HashTable<String, TestResults> _results;
};