#include "alfe/main.h"
#include "alfe/handle.h"
#include "alfe/email.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        console.write("Status: 200 OK\n"
            "Content-Type: text/html; charset=utf-8'\n\n"
            "<!DOCTYPE html PUBLIC '-//W3C//DTD XHTML 1.0 Strict//EN' "
                "'http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd'>\n"
            "<html xmlns='http://www.w3.org/1999/xhtml' dir='ltr' "
                "lang='en-US'>\n"
                "<head>\n"
                    "<meta http-equiv='Content-Type' content='text/html; "
                        "charset=UTF-8' />\n"
                    "<title>XT Server - cancellation</title>\n"
                "</head>\n"
                "<body><h1>XT Server</h1>\n<p>");
        try {
            Stream in(GetStdHandle(STD_INPUT_HANDLE),
                String("standard input"), false);

            bool eof;
            String process = in.readString(&eof, 80);

            Stream s = File("\\\\.\\pipe\\xtserver", true).openPipe();
            s.write<int>(0);                 // emailLength
            s.write<int>(process.length());  // fileNameLength
            s.write(process);                // fileName
            s.write<int>(0);                 // dataLength
            s.write<DWORD>(0);               // serverPId
            s.write<int>(0);                 // logFileLength
            s.write<int>(3);                 // command

            do {
                int b = s.tryReadByte();
                if (b == -1)
                    break;
                console.write<Byte>(b);
            } while (true);
        }
        catch (...)
        {
            console.write("An error occurred during cancellation.\n");
        }
        console.write("</p>\n");
    }
};
