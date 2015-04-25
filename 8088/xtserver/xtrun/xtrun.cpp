#include "alfe/main.h"
#include "alfe/handle.h"
#include "alfe/email.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() < 2) {
            console.write("Syntax: " + _arguments[0] +
                " <input file name>\n");
            return;
        }

        String fileName = _arguments[1];
        Array<Byte> data;
        String contents = File(fileName, true).contents();

        String logName("nul");

        AutoHandle h = File("\\\\.\\pipe\\xtserver", true).openPipe();
        h.write<int>(0);                        // emailLength
        h.write(String(""));                    // email
        h.write<int>(fileName.length());        // fileNameLength
        h.write(fileName);                      // fileName
        h.write<int>(contents.length());        // dataLength
        h.write(contents);                      // data
        h.write<DWORD>(GetCurrentProcessId());  // serverPId
        h.write<int>(logName.length());         // logFileLength
        h.write(logName);                       // logFile
        h.write<int>(0);                        // command

        do {
            int b = h.tryReadByte();
            if (b == -1)
                break;
            console.write<Byte>(b);
        } while (true);
    }
};
