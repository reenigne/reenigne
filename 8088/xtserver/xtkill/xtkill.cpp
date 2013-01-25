#include "alfe/main.h"
#include "alfe/handle.h"
#include "alfe/email.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        String process = "0";
        if (_arguments.count() > 0)
            process = _arguments[1];

        AutoHandle h = File("\\\\.\\pipe\\xtserver", true).openPipe();
        h.write<int>(0);                 // emailLength
        h.write<int>(process.length());  // fileNameLength
        h.write(process);                // fileName
        h.write<int>(0);                 // dataLength
        h.write<DWORD>(0);               // serverPId
        h.write<int>(0);                 // logFileLength
        h.write<int>(1);                 // command
    }
};