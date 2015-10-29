#include "alfe/main.h"
#include "alfe/file.h"

class Program : public ProgramBase
{
protected:
    void run()
    {
        String inputData = File("f000.dat").contents();
        FileStream outputData = File("1501512.u18").openWrite();
        int j = 0;
        for (int i = 0; i < 0x10000; ++i) {
            if (inputData[j] == 0) {
                if (i >= 0x8000)
                    if (inputData[j+1] == 0)
                        outputData.write<Byte>(0);
                    else
                        outputData.write<Byte>(26);
                j += 2;
            }
            else {
                if (i >= 0x8000)
                    outputData.write<Byte>(inputData[j]);
                ++j;
            }
        }
    }
};
