#include "alfe/main.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        float span = 200-16;

        String asmOutput;
        asmOutput += "sinTable:\n";
        for (int i = 0; i < 512; ++i) {
            if ((i & 15) == 0)
                asmOutput += "  dw ";
            asmOutput += "startAddresses + " + hex(static_cast<int>((sin(i*tau/512.0) + 1)*span/2 + 0.5)*2, 3);
            if ((i & 15) != 15)
                asmOutput += ", ";
            else
                asmOutput += "\n";
        }
        File("sinTable.inc").openWrite().write(asmOutput);
    }
};
