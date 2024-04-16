#include "alfe/main.h"
#include "../gentests.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        AppendableArray<Test> fails;
        Array<Byte> d;
        File("fails.dat").readIntoArray(&d);
        Byte* p = &d[0];
        int i = 0;
        while (i < d.count()) {
            Test t;
            t.read(p);
            int l = t.length();
            p += l;
            i += l;
            fails.append(t);
        }
    }
};