#include "alfe/file.h"

int main()
{
    FileStream in =
        File("/t/projects/emulation/pc/ibm5150.net/f/fire/fire.ech").
        openRead();
    FileStream out = File("fire.raw").openWrite();

    int s = in.size();
    Array<Byte> four(s);
    in.read(&four[0], s);

    Array<Byte> eight(s*2);
    for (int i = 0; i < s; ++i) {
        eight[i*2] = (four[i]>>4) * 0x11;
        eight[i*2+1] = (four[i] & 0xf) * 0x11;
    }
    out.write(&eight[0], s*2);
}
