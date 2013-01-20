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
            AutoHandle h = File("\\\\.\\pipe\\xtserver", true).openPipe();
            h.write<int>(0);
            h.write<int>(0);
            h.write<int>(0);
            h.write<DWORD>(0);
            h.write<int>(2);

            do {
                int b = h.tryReadByte();
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