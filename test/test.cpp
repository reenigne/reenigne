#include "alfe/string.h"
#include "alfe/main.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        {
            String a("a");
            String b("b");
            String c = a+b;
            String d = c + "\n";
            _console.write(d);  // Should print "ab"
        }

        {
            String c = "foobar";
            CharacterSource s(c);
            s.get();
            int a = s.offset();
            s.get();
            s.get();
            s.get();
            int b = s.offset();
            _console.write(s.subString(a, b));  // Should print "oob"
        }
    }
};
