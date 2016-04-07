#include "alfe/main.h"

#ifndef INCLUDED_CGA_H
#define INCLUDED_CGA_H

class CGASequencer
{
public:
    CGASequencer()
    {
        _cgaROM = File("5788005.u33").contents();
        static Byte palettes[] = {
            0, 2, 4, 6, 0, 10, 12, 14, 0, 3, 5, 7, 0, 11, 13, 15,
            0, 3, 4, 7, 0, 11, 12, 15, 0, 3, 4, 7, 0, 11, 12, 15};
        memcpy(_palettes, palettes, 32);
    }

    //  ; Mode                                                2c  28  2d  29  2a  2e  1a  29
    //  ;      1 +HRES                                         0   0   1   1   0   0   0   1
    //  ;      2 +GRPH                                         0   0   0   0   2   2   2   0
    //  ;      4 +BW                                           4   0   4   0   0   4   0   0
    //  ;      8 +VIDEO ENABLE                                 8   8   8   8   8   8   8   8
    //  ;   0x10 +1BPP                                         0   0   0   0   0   0  10   0
    //  ;   0x20 +ENABLE BLINK                                20  20  20  20  20  20   0  20
    //
    //  ; Palette
    //  ;      1 +OVERSCAN B
    //  ;      2 +OVERSCAN G
    //  ;      4 +OVERSCAN R
    //  ;      8 +OVERSCAN I
    //  ;   0x10 +BACKGROUND I
    //  ;   0x20 +COLOR SEL

//    +HRES +GRPH gives a0 cded ghih in startup phase 0 odd       Except all start at same pixel, so output byte depends on more than 4 bytes of input data
//    +HRES +GRPH gives abcb efgf ij in other   phase 1 even
//    with 1bpp +HRES, odd bits are ignored (76543210 = -0-1-2-3)


    // renders a 16 hdot by 1 scanline region of CGA VRAM data into RGBI data
    // cursor is cursor output pin from CRTC
    // cursor_blink counts from 0..3 then repeats, changes every 8 frames (low bit cursor, high bit blink)
    UInt64 process(UInt32 input, UInt8 mode, UInt8 palette, int scanline,
        bool cursor, int cursorBlink)
    {
        if ((mode & 8) == 0)
            return 0;
        Character c;
        UInt64 r = 0;
        int x;
        int* pal;

        switch (mode & 0x13) {
            case 0:
                // 40-column text mode
                c = getCharacter(input, mode, scanline, cursor, cursorBlink);
                for (x = 0; x < 8; ++x)
                    r += static_cast<UInt64>(((c.attribute >> ((c.bits & (128 >> x)) != 0 ? 0 : 4)) & 0xf) * 0x11) << (x * 8);
                break;
            case 1:
                // 80-column text mode
                c = getCharacter(input, mode, scanline, cursor, cursorBlink);
                for (x = 0; x < 8; ++x)
                    r += static_cast<UInt64>((c.attribute >> ((c.bits & (128 >> x)) != 0 ? 0 : 4)) & 0xf) << (x * 4);
                c = getCharacter(input >> 16, mode, scanline, cursor, cursorBlink);
                for (x = 0; x < 8; ++x)
                    r += static_cast<UInt64>((c.attribute >> ((c.bits & (128 >> x)) != 0 ? 0 : 4)) & 0xf) << (x * 4 + 32);
                break;
            case 2:
                // 2bpp graphics mode
                pal = &_palettes[((palette & 0x30) >> 2) + ((mode & 4) << 2];
                *pal = palette & 0xf;
                for (x = 0; x < 2; ++x) {
                    Byte b = input >> (x * 8);
                    for (int xx = 0; xx < 4; ++xx)
                        r += static_cast<UInt64>(pal[(b >> (6 - x * 2)) & 3] * 0x11) << (x*32 + xx*8);
                }
                break;
            case 3:
                // Improper: +HRES 2bpp graphics mode
                pal = &_palettes[((palette & 0x30) >> 2) + ((mode & 4) << 2];
                *pal = palette & 0xf;
                for (x = 0; x < 4; ++x) {
                    Byte b = input >> (x * 8);
                    for (int xx = 0; xx < 4; ++xx)
                        r += static_cast<UInt64>(pal[(b >> (6 - x * 2)) & 3] * 0x11) << (x*32 + xx*8);
                }
                break;
            case 0x10:
                // Improper: 40-column text mode with 1bpp graphics overlay
                break;
            case 0x11:
                // Improper: 80-column text mode with +HRES 1bpp graphics mode
                break;
            case 0x12:
                // 1bpp graphics mode
                break;
            case 0x13:
                // Improper: +HRES 1bpp graphics mode
                break;
        }


        if (_config < 64) {
            static int palettes[4] = {0, 8, 1, 9};
            for (int x = 0; x < 4; x += 2) {
                int c = _config & 15;
                int b = ((pattern >> (2 - x)) & 3);
                if (b != 0)
                    c = b + palettes[_config >> 4];
                rgbi[x] = c;
                rgbi[x + 1] = c;
            }
            return;
        }
        if (_config < 80) {
            for (int x = 0; x < 4; ++x)
                rgbi[x] = (pattern & (8 >> x)) != 0 ? (_config & 15) : 0;
            return;
        }
    }
private:
    struct Character
    {
        int bits;
        int attribute;
    };

    Character getCharacter(UInt16 input, UInt8 mode, int scanline, bool cursor, int cursorBlink)
    {
        Character c;
        c.bits = _cgaROM[(0x300 + (input & 0xff))*8 + (scanline & 7)];
        c.attribute = input >> 8;
        if (cursor && ((cursor_blink & 1) != 0))
            c.bits = 0xff;
        else {
            if ((mode & 0x20) != 0 && (attribute & 0x80) != 0 && (cursor_blink & 2) != 0 && !cursor)
                c.bits = 0;
        }
        if ((mode & 0x20) != 0)
            c.attribute &= 0x7f;
        return c;
    }

    String _cgaROM;
    Byte _palettes[32];
};

class CGA

#endif // INCLUDED_CGA_H
