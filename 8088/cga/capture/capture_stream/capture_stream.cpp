#include "alfe/main.h"
#include <conio.h>

class Program : public ProgramBase
{
public:
    void run()
    {
        String name = "captured.raw";
        int frames = 1;
        if (_arguments.count() >= 2)
            name = _arguments[1];
        AutoHandle h = File("\\\\.\\pipe\\vbicap", true).openPipe();
        h.write<int>(1);

        AutoHandle oh = File(name, true).openWrite();

        Array<Byte> samples(450*1024);
        int k = 0;
        do {
            h.read(&samples[0], 450*1024);
            oh.write(&samples[0], 450*1024);
            if (kbhit())
                k = getch();
        } while (k != 27);
    }
};
