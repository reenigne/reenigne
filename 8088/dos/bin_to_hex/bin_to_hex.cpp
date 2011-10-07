#include "unity/main.h"
#include "unity/file.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        if (_arguments.count() == 1) {
            _consoel.write(String("Usage: bin_to_hex <name of file>\n"));
            return;
        }
        String fileName = _arguments[1];
        String data = File(fileName).contents();
        int l = data.length();

        // Write length bytes
        _console.write(String("  .byte "));
        _console.write(String("0x") + String::hexadecimal(l & 0xff, 2));
        _console.write(commaSpace);
        _console.write(String("0x") + String::hexadecimal(l >> 8, 2));
        _console.write(newLine);

        for (int i = 0; i < l; ++i) {
            int c = i & 7;
            if (c == 0)
                _console.write(String("  .byte "));
            _console.write(String("0x") + String::hexadecimal(data[i], 2));
            if (c < 7 && i < l - 1)
                _console.write(commaSpace);
            else
                _console.write(newLine);
        }
    }
};
