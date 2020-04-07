#include "alfe/main.h"
#include "alfe/handle.h"
#include "alfe/email.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        console.write("Status: 200 OK\n"
            "Content-Type: text/javascript; charset=utf-8'\n\n");
        console.write("updateStatus('");
        try {
            Stream s = File("\\\\.\\pipe\\xtserver", true).openPipe();
            s.write<int>(0);    // emailLength
            s.write<int>(0);    // fileNameLength
            s.write<int>(0);    // dataLength
            s.write<DWORD>(0);  // serverPId
            s.write<int>(0);    // logFileLength
            s.write<int>(2);    // command

            do {
                int b = s.tryReadByte();
                if (b == -1)
                    break;
                console.write<Byte>(b);
            } while (true);
        }
        catch (...) {
            console.write("offline");
        }
        console.write("');\n");
    }
};
