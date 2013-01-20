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
            process = _arguments[0];

        AutoHandle h = File("\\\\.\\pipe\\xtserver", true).openPipe();
        h.write<int>(0);
        h.write<int>(process.length());
        h.write(process);
        h.write<int>(0);
        h.write<DWORD>(0);
        h.write<int>(1);
    }
};