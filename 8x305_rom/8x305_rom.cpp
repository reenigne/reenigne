#include "alfe/main.h"

int getNybble(CharacterSource* s)
{
    do {
        int b = s->get();
        if (b >= '0' && b <= '9')
            return b - '0';
        if (b >= 'A' && b <= 'F')
            return b + 10 - 'A';
        if (b == -1)
            break;
    } while (true);
    return 0;
}

int getByte(CharacterSource* s)
{
    int high = getNybble(s);
    int low = getNybble(s);
    return (high << 4) | low;
}

class Program : public ProgramBase
{
public:
    void run()
    {
        String in1 = File("rom1.txt").contents();
        String in2 = File("rom3.txt").contents();
        Array<Byte> out(2048*2);
        CharacterSource i1s(in1);
        CharacterSource i2s(in2);
        for (int y = 0; y < 2048; ++y) {
            Word o = (getByte(&i2s) << 8) | getByte(&i1s); //(getByte(&i1s) << 8) | getByte(&i2s);
            Word r = o; //((o >> 15) & 1) | ((o >> 13) & 2) | ((o >> 11) & 4) | ((o >> 9) & 8) | ((o >> 7) & 0x10) | ((o >> 5) & 0x20) | ((o >> 3) & 0x40) | ((o >> 1) & 0x80) | ((o << 1) & 0x100) | ((o << 3) & 0x200) | ((o << 5) & 0x400) | ((o << 7) & 0x800) | ((o << 9) & 0x1000) | ((o << 11) & 0x2000) | ((o << 13) & 0x4000) | ((o << 15) & 0x8000);
            out[y*2] = r;
            out[y*2 + 1] = r >> 8;
        }
        File("rom.dat").save(&out[0], 4096);
    }
};