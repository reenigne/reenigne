#include "alfe/main.h"
#include "alfe/file.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() == 1) {
            console.write("Usage: bin_to_hex <name of file>\n");
            return;
        }
        String fileName = _arguments[1];
        String data = File(fileName, CurrentDirectory(), true).contents();
        int l = data.length();

        // Write length bytes
        console.write("  .byte " + hex(l & 0xff, 2) + ", " + hex(l >> 8, 2) +
            "\n");

        for (int i = 0; i < l; ++i) {
            int c = i & 7;
            if (c == 0)
                console.write("  .byte ");
            console.write(String(hex(data[i], 2)));
            if (c < 7 && i < l - 1)
                console.write(", ");
            else
                console.write("\n");
        }
    }
};

