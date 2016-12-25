#include "alfe/main.h"
#include "alfe/hash_table.h"

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
        CharacterSource t = *s;
        do {

        }
    }

    void parseTestLog(CharacterSource s, bool right)
    {
        do {
            if (parse(&s, "***")) {

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
        String l1 = File(_arguments[1], true).contents();
        String l2 = File(_arguments[2], true).contents();
        HashTable<String, TestResults> results;
        parseTestLog(l1, false);
        parseTestLog(l2, true);
    }
};