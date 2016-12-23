#include "alfe/main.h"
#include "alfe/hash_table.h"

class TestResult
{

};

class Program : public ProgramBase
{
    void run()
    {
        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] +
                " <first log file name> <second log file name>\n");
            return;
        }
        String l1 = File(_arguments[1], true).contents();
        String l2 = File(_arguments[2], true).contents();
        HashTable<String, TestResult> results;
    }
};