#include "unity/main.h"
#include "unity/file.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() == 1) {
            String("Usage: bin_to_hex <name of file>\n").
                write(Handle::consoleOutput());
            return;
        }
        String fileName = _arguments[1];
        String data = File(fileName).contents();
        int l = data.length();

        Handle console = Handle::consoleOutput();
        
        // Write length bytes
        String("  .byte ").write(console);
        (String("0x") + String::hexadecimal(l & 0xff, 2)).write(console);
        commaSpace.write(console);
        (String("0x") + String::hexadecimal(l >> 8, 2)).write(console);
        String("\n").write(console);

        for (int i = 0; i < l; ++i) {
            int c = i & 7;
            if (c == 0)
                String("  .byte ").write(console);
            (String("0x") + String::hexadecimal(data[i], 2)).write(console);
            if (c < 7 && i < l - 1)
                commaSpace.write(console);
            else
                String("\n").write(console);
        }
    }
};