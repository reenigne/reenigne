#include "unity/string.h"
#include "unity/file.h"

class Program
{
public:
    Program(CharacterSource source) : _source(source) { }

    void load()
    {
        _done = false;
        while (!_done)
            parseLine();
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
        switch (recordType) {
            case 0:  // data record
                for (int i = 0; i < byteCount; ++i)
                    _data[address++] = readByte();
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
    }
    int readByte()
    {
        int b = readNybble() << 4;
        b |= readNybble();
        _checkSum += b;

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
		contents.write(Handle::consoleOutput());
	}
	END_CHECKED(Exception& e) {
		e.write(Handle::consoleOutput());
	}
}