#include "unity/string.h"
#include "unity/file.h"

#include <stdio.h>

class Program
{
public:
    Program(CharacterSource source) : _source(source) { }

    void load()
    {
        _done = false;
        while (!_done)
            parseLine();
        for (int i = 0; i < 0x400; i += 2) {
            int op =  _data[i] | (_data[i+1] << 8);
                 if ((op & 0xfc0) == 0x1c0) printf("ADDWF 0x%02x, %c\n", op & 0x1f, (op & 0x20) != 0 ? 'f' : 'W');
            else if ((op & 0xfc0) == 0x140) printf("ANDWF 0x%02x, %c\n", op & 0x1f, (op & 0x20) != 0 ? 'f' : 'W');
            else if ((op & 0xfe0) == 0x060) printf("CLRF 0x%02x\n", op & 0x1f, (op & 0x20) != 0 ? 'f' : 'W');
            else if ((op & 0xfff) == 0x040) printf("CLRW\n", op & 0x1f);
            else if ((op & 0xfc0) == 0x240) printf("COMF 0x%02x, %c\n", op & 0x1f, (op & 0x20) != 0 ? 'f' : 'W');
            else if ((op & 0xfc0) == 0x0c0) printf("DECF 0x%02x, %c\n", op & 0x1f, (op & 0x20) != 0 ? 'f' : 'W');
            else if ((op & 0xfc0) == 0x2c0) printf("DECFSZ 0x%02x, %c\n", op & 0x1f, (op & 0x20) != 0 ? 'f' : 'W');
            else if ((op & 0xfc0) == 0x280) printf("INCF 0x%02x, %c\n", op & 0x1f, (op & 0x20) != 0 ? 'f' : 'W');
            else if ((op & 0xfc0) == 0x3c0) printf("INCFSF 0x%02x, %c\n", op & 0x1f, (op & 0x20) != 0 ? 'f' : 'W');
            else if ((op & 0xfc0) == 0x100) printf("IORWF 0x%02x, %c\n", op & 0x1f, (op & 0x20) != 0 ? 'f' : 'W');
            else if ((op & 0xfc0) == 0x200) printf("MOVF 0x%02x, %c\n", op & 0x1f, (op & 0x20) != 0 ? 'f' : 'W');
            else if ((op & 0xfe0) == 0x020) printf("MOVWF 0x%02x\n", op & 0x1f);
            else if ((op & 0xfff) == 0x000) printf("NOP\n");
            else if ((op & 0xfc0) == 0x320) printf("RLF 0x%02x, %c\n", op & 0x1f, (op & 0x20) != 0 ? 'f' : 'W');
            else if ((op & 0xfc0) == 0x300) printf("RRF 0x%02x, %c\n", op & 0x1f, (op & 0x20) != 0 ? 'f' : 'W');
            else if ((op & 0xfc0) == 0x080) printf("SUBWF 0x%02x, %c\n", op & 0x1f, (op & 0x20) != 0 ? 'f' : 'W');
            else if ((op & 0xfc0) == 0x380) printf("SWAPF 0x%02x, %c\n", op & 0x1f, (op & 0x20) != 0 ? 'f' : 'W');
            else if ((op & 0xfc0) == 0x180) printf("XORWF 0x%02x, %c\n", op & 0x1f, (op & 0x20) != 0 ? 'f' : 'W');
            

        }
    }
    void parseLine()
    {
        _source.assert(':');
        _checkSum = 0;
        CharacterSource byteCountLocation = _source;
        int byteCount = readByte();
        int address = readByte() << 8;
        address |= readByte();
        CharacterSource recordTypeLocation = _source;
        int recordType = readByte();
        for (int i = 0; i < byteCount; ++i) {
            int b = readByte();
            if (recordType == 0 && address < 0x400)
                _data[address++] = b;
        }
        switch (recordType) {
            case 0:  // data record - handled above
                break;
            case 1:  // end of file record
                if (byteCount != 0) {
                    static String error("End of file marker incorrect. Expected no data, found ");
                    static String bytes(" bytes.");
                    byteCountLocation.throwError(error + String::decimal(byteCount) + bytes);
                }
                _done = true;
                break;
            case 4:  // extended linear address record
                break;
            default:
                {
                    static String error("Don't know what to do with record type ");
                    recordTypeLocation.throwError(error + String::decimal(recordType));
                }
        }
        CharacterSource checkSumLocation = _source;
        int checkSum = readByte();
        if ((_checkSum & 0xff) != 0) {
            static String error("Checksum incorrect. Expected ");
            static String found(", found ");
            checkSumLocation.throwError(error + String::hexadecimal((checkSum - _checkSum) & 0xff, 2) + found + String::hexadecimal(checkSum, 2));
        }
        _source.assert(10);
    }
    int readByte()
    {
        int b = readNybble() << 4;
        b |= readNybble();
        _checkSum += b;
        return b;
    }
    int readNybble()
    {
        CharacterSource start = _source;
        int n = _source.get();
        if (n >= '0' && n <= '9')
            return n - '0';
        if (n >= 'a' && n <= 'f')
            return n + 10 - 'a';
        if (n >= 'A' && n <= 'F')
            return n + 10 - 'A';
        static String expected("0-9 or A-F");
        start.throwUnexpected(expected, String::codePoint(n));
    }
private:
    UInt8 _data[0x400];
    bool _done;
    int _checkSum;
    CharacterSource _source;
};

int main()
{
	BEGIN_CHECKED {
		String fileName("../intervals.HEX");
		File file(fileName);
		String contents = file.contents();
        CharacterSource s = contents.start();
        Program program(s);
        program.load();
		//contents.write(Handle::consoleOutput());
	}
	END_CHECKED(Exception& e) {
		e.write(Handle::consoleOutput());
	}
}