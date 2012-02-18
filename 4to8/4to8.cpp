#include "alfe/file.h"

int main()
{
    File inFile("/t/projects/emulation/pc/ibm5150.net/f/fire/fire.ech");
    FileHandle in(inFile);
    in.openRead();
    File outFile("fire.raw");
    FileHandle out(outFile);
    out.openWrite();
    Array<Byte> four;
    int s = in.size();
    four.allocate(s);
    in.read(&four[0], s);
    Array<Byte> eight;
    eight.allocate(s*2);
    for (int i = 0; i < s; ++i) {
        eight[i*2] = (four[i]>>4) * 0x11;
        eight[i*2+1] = (four[i] & 0xf) * 0x11;
    }
    out.write(&eight[0], s*2);
}
