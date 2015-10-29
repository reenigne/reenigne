#include "alfe/main.h"
#include "alfe/handle.h"
#include "alfe/email.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        String process = "0";
        if (_arguments.count() > 1)
            process = _arguments[1];

        AutoStream s = File("\\\\.\\pipe\\xtserver", true).openPipe();
        s.write<int>(0);                 // emailLength
        s.write<int>(process.length());  // fileNameLength
        s.write(process);                // fileName
        s.write<int>(0);                 // dataLength
        s.write<DWORD>(0);               // serverPId
        s.write<int>(0);                 // logFileLength
        s.write<int>(1);                 // command
    }
};
