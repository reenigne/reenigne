#include "alfe/main.h"
#include "alfe/file.h"
#include <stdio.h>

class Program : public ProgramBase
{
    int hexDigit(int n) { return n < 10 ? n + '0' : n + 'a' - 10; }

public:
    void run()
    {
#if 0
        // Dump entire ROM so we can see how it's laid out.
        int fileSize = 8*16 * 4*16*8;
        Array<Byte> buffer(fileSize);

        for (int set = 0; set < 4; ++set) {
            for (int ch = 0; ch < 256; ++ch) {
                for (int y = 0; y < 8; ++y) {
                    int bits = data[(set*256 + ch)*8 + y];
                    for (int x = 0; x < 8; ++x)
                        buffer[
                            ((set*16 + (ch >> 4))*8 + y)*8*16 + (ch & 15)*8 + x
                            ] = ((bits & (128 >> x)) != 0 ? 255 : 0);
                }
            }
        }
        // Set 0: MDA characters rows 0-7
        // Set 1: MDA characters rows 8-13
        // Set 2: CGA narrow characters
        // Set 3: CGA normal characters
#endif
#if 1
        // Dump the top rows to a text file
        int fileSize = 13*256;
        Array<Byte> buffer(fileSize);
        File file("/t/projects/emulation/pc/roms/5788005.u33");
        String cgaROM = file.contents();

        Array<UInt16> patterns(0x200);
        int pattern = 0;

        for (int ch = 0; ch < 512; ++ch) {
            int bits0 = cgaROM[(3*256 + (ch & 0xff))*8];
            if ((ch & 0x100) != 0)
                bits0 = ~bits0;
            int bits1 = cgaROM[(3*256 + (ch & 0xff))*8 + 1];
            if ((ch & 0x100) != 0)
                bits1 = ~bits1;
            UInt16 bits = ((bits0 & 0xff) << 8) + (bits1 & 0xff);
            int p;
            for (p = 0; p < pattern; ++p)
                if (bits == patterns[p])
                    break;
            if (p == pattern)
                patterns[pattern++] = bits;
            else
                continue;

            //printf("%04x\n",bits);

            for (int x = 0; x < 8; ++x)
                printf("%c", (bits0 & (128 >> x)) != 0 ? '*' : '.');
            printf("\n");

            for (int x = 0; x < 8; ++x)
                printf("%c", (bits1 & (128 >> x)) != 0 ? '*' : '.');
            printf("\n\n");
        }
        File("top_rows.txt").save(buffer);
#endif

#if 0
        // Try to reproduce each character at 100% size
        File file("/t/projects/emulation/pc/roms/5788005.u33");
        String cgaROM = file.contents();
        for (int target = 0; target < 256; ++target) {
            int totalScore = 0;
            int best[4];
            printf("0x%02x ('%c'): ", target, target >= 32 && target <= 126 ? target : ' ');
            for (int slice = 0; slice < 4; ++slice) {
                int bestCh = 0;
                int bestScore = 16;
                UInt8 target0 = cgaROM[(3*256 + target)*8 + slice*2];
                UInt8 target1 = cgaROM[(3*256 + target)*8 + slice*2 + 1];
                for (int ch = 0; ch < 512; ++ch) {
                    UInt8 t0 = cgaROM[(3*256 + (ch & 0xff))*8] ^ target0;
                    UInt8 t1 = cgaROM[(3*256 + (ch & 0xff))*8 + 1] ^ target1;
                    if ((ch & 0x100) != 0) {
                        t0 = ~t0;
                        t1 = ~t1;
                    }
                    int s = 0;
                    for (int i = 0; i < 8; ++i) {
                        if ((t0 & (1<<i)) != 0)
                            ++s;
                        if ((t1 & (1<<i)) != 0)
                            ++s;
                    }
                    if (s < bestScore) {
                        bestCh = ch;
                        bestScore = s;
                    }
                }
                printf("%c%02x", (bestCh & 0x100) != 0 ? '-' : '+', bestCh & 0xff);
                totalScore += bestScore;
                best[slice] = bestCh;
            }
            printf(": %2x\n", totalScore);
            for (int y = 0; y < 8; ++y) {
                int ch = best[y >> 1];
                for (int x = 0; x < 8; ++x) {
                    UInt8 bits = cgaROM[(3*256 + (ch & 0xff))*8 + (y & 1)];
                    if ((ch & 0x100) != 0)
                        bits = ~bits;
                    if ((bits & (128 >> x)) != 0)
                        printf("*");
                    else
                        printf(" ");
                }
                printf("\n");
            }
        }
#endif

#if 0
        // Try to reproduce each character at 200% size
        File file("/t/projects/emulation/pc/roms/5788005.u33");
        String cgaROM = file.contents();
        for (int target = 0; target < 256; ++target) {
            int totalScore = 0;
            int best[16];
            printf("0x%02x ('%c'): ", target, target >= 32 && target <= 126 ? target : ' ');
            for (int slice = 0; slice < 16; ++slice) {
                int bestCh = 0;
                int bestScore = 16;
                UInt8 target0 = cgaROM[(3*256 + target)*8 + (slice>>1)];
                UInt8 target1;
                if ((slice & 1) == 0)
                    target1 =
                        ((target0 & 0x80) != 0 ? 0xc0 : 0x00) +
                        ((target0 & 0x40) != 0 ? 0x30 : 0x00) +
                        ((target0 & 0x20) != 0 ? 0x0c : 0x00) +
                        ((target0 & 0x10) != 0 ? 0x03 : 0x00);
                else
                    target1 =
                        ((target0 & 0x08) != 0 ? 0xc0 : 0x00) +
                        ((target0 & 0x04) != 0 ? 0x30 : 0x00) +
                        ((target0 & 0x02) != 0 ? 0x0c : 0x00) +
                        ((target0 & 0x01) != 0 ? 0x03 : 0x00);
                for (int ch = 0; ch < 512; ++ch) {
                    UInt8 t0 = cgaROM[(3*256 + (ch & 0xff))*8] ^ target1;
                    UInt8 t1 = cgaROM[(3*256 + (ch & 0xff))*8 + 1] ^ target1;
                    if ((ch & 0x100) != 0) {
                        t0 = ~t0;
                        t1 = ~t1;
                    }
                    int s = 0;
                    for (int i = 0; i < 8; ++i) {
                        if ((t0 & (1<<i)) != 0)
                            ++s;
                        if ((t1 & (1<<i)) != 0)
                            ++s;
                    }
                    if (s < bestScore) {
                        bestCh = ch;
                        bestScore = s;
                    }
                }
                printf("%c%02x", (bestCh & 0x100) != 0 ? '-' : '+', bestCh & 0xff);
                totalScore += bestScore;
                best[slice] = bestCh;
            }
            printf(": %2x\n", totalScore);
            for (int y = 0; y < 16; ++y) {
                for (int x = 0; x < 16; ++x) {
                int ch = best[(y & 0x0e) + (x >> 3)];
                    UInt8 bits = cgaROM[(3*256 + (ch & 0xff))*8 + (y & 1)];
                    if ((ch & 0x100) != 0)
                        bits = ~bits;
                    if ((bits & (128 >> (x & 7))) != 0)
                        printf("*");
                    else
                        printf(" ");
                }
                printf("\n");
            }
        }
#endif
    }
};
