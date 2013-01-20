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
            Handle in(GetStdHandle(STD_INPUT_HANDLE));

            bool eof;
            String process = in.readString(&eof, 80);
            
            AutoHandle h = File("\\\\.\\pipe\\xtserver", true).openPipe();
            h.write<int>(0);
            h.write<int>(process.length());
            h.write(process);
            h.write<int>(0);
            h.write<DWORD>(0);
            h.write<int>(3);

            do {
                int b = h.tryReadByte();
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