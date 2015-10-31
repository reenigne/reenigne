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

        AutoStream s = File("\\\\.\\pipe\\xtserver", true).openPipe();
        s.write<int>(0);                        // emailLength
        s.write(String(""));                    // email
        s.write<int>(fileName.length());        // fileNameLength
        s.write(fileName);                      // fileName
        s.write<int>(contents.length());        // dataLength
        s.write(contents);                      // data
        s.write<DWORD>(GetCurrentProcessId());  // serverPId
        s.write<int>(logName.length());         // logFileLength
        s.write(logName);                       // logFile
        s.write<int>(4);                        // command

        do {
            int b = s.tryReadByte();
            if (b == -1)
                break;
            console.write<Byte>(b);
        } while (true);
    }
};
