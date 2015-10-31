#include "alfe/main.h"
#include "alfe/file.h"

class Program : public ProgramBase
{
public:
    int run()
    {
        String inputData = File("capture.txt").contents();
        FileStream outputStatus = File("status.txt").openWrite();
        FileStream outputData = File("hd.dat").openWrite();
        int i = 7*64 + 24;
        int fails = 0;
        do {
            if (i + 18 + 0x200 >= inputData.length())
                return;
            int f = inputData[i + 16];
            bool fail;
            if (f >= 'A' && f <= 'F')
                fail = (10 + f - 'A')&1;
            else
                fail = (f - '0')&1;
            if (fail) {
                ++fails;
                if (fails == 10) {
                    for (int j = 0; j < 0x200; ++j)
                        outputData.write<Byte>('*');
                    for (int j = 0; j < 8; ++j)
                        outputStatus.write<Byte>(inputData[j + i]);
                    outputStatus.write(" gave up\n");
                    fails = 0;
                }
                i += 18;
            }
            else {
                for (int j = 0; j < 0x200; ++j)
                    outputData.write<Byte>(inputData[j + i + 18]);
                for (int j = 0; j < 8; ++j)
                    outputStatus.write<Byte>(inputData[j + i]);
                outputStatus.write(String(" read after ") + fails +
                    " failures\n");
                fails = 0;
                i += 18 + 0x200;
            }
        } while (true);
    }
};
