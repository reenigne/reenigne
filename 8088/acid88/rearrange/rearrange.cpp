#include "alfe/main.h"

class Program : public ProgramBase
{
public:
    void run()
    {
        File("M:\\Program Files (x86)\\Apache Software Foundation\\Apache24\\htdocs\\ZZchGHqnWGZ0tI-W0.dat", true).readIntoArray(&_input);
        _output.allocate(_input.count());
        _ip = 0;
        _op = 0;
        copyByte();
        copyByte();
        int moved = copyTests(false);
        _ip = 2;
        copyTests(true);
        File("tests_rearranged.bin").openWrite().write(_output);
        console.write("Tests moved: " + decimal(moved) + "\n");
    }
    void copyByte()
    {
        Byte b = _input[_ip];
        ++_ip;
        _output[_op] = b;
        ++_op;
    }
    int copyTests(bool endOnes)
    {
        int skipped = 0;
        do {
            int cycles = _input[_ip + 0] + (_input[_ip + 1] << 8);
            if (cycles == 32768)
                console.write("Zero cycle count found.\n");
            console.write(String(decimal((cycles + 1000) & 0xffff)).alignRight(5) + "\n");
            int preambleBytes = _input[_ip + 5];
            int instructionBytes = _input[_ip + 6 + preambleBytes];
            Byte instructionByte0 = _input[_ip + 7 + preambleBytes];
            Byte instructionByte1 = _input[_ip + 8 + preambleBytes];
            bool endOne = ((instructionByte0 /*& 0xfe*/) == 0xfe && (instructionByte1 == 0xd8 || instructionByte1 == 0xe8));
            int fixupBytes = _input[_ip + 7 + preambleBytes + instructionBytes];
            int length = 8 + preambleBytes + instructionBytes + fixupBytes;
            if (endOne == endOnes) {
                for (int i = 0; i < length; ++i)
                    copyByte();
            }
            else {
                _ip += length;
                ++skipped;
            }
        } while (_ip != _input.count());
        return skipped;
    }
private:
    Array<Byte> _input;
    Array<Byte> _output;
    int _ip;
    int _op;
};