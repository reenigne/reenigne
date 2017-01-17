#include "alfe/main.h"
#include "alfe/hash_table.h"
#include "alfe/space.h"
#include <inttypes.h>

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
        bool tooLarge = false;
        bool fail = false;
        bool excess = false;
        String t("Error: too large for a .com file.");
        String f("FAIL:");
        String e("(test for excess errors)");

        int offset = 0;
        do {
            if (parse(&s, t)) {
                if (excess)
                    excess = false;
                else
                    tooLarge = true;
                continue;
            }
            if (tooLarge) {
                int o = s.offset();
                if (parse(&s, f)) {
                    fail = true;
                    tooLarge = false;
                    console.write(s.subString(offset, o));
                    offset = s.offset();
                    console.write("TOO_BIG:");
                    continue;
                }
            }
            if (fail) {
                if (parse(&s, e)) {
                    excess = true;
                    fail = false;
                    continue;
                }
            }
            int c = s.get();
            if (c == 10)
                fail = false;
            if (c == -1)
                break;
        } while (true);
        console.write(s.subString(offset, s.length()));
    }

    void run()
    {
        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] +
                " <log file name>\n");
            return;
        }
        String l1 = File(_arguments[1], true).contents();
        _eol = String(codePoint(10));
        parseTestLog(l1);
    }
    String _eol;
};
