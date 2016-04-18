#include "alfe/main.h"

#ifndef INCLUDED_CGA_H
#define INCLUDED_CGA_H

class CGASequencer
{
public:
    CGASequencer(File cgaROM)
    {
        _cgaROM = rom.contents();
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
//    +HRES +GRPH gives abcb efgf ij in other   phase 1 even  <- use this one for compatibility with -HRES modes
//    with 1bpp +HRES, odd bits are ignored (76543210 = -0-1-2-3)

// Can we do all the graphics modes with tables?
// 1bpp: 16 pixel positions * 2 colours * 16 palettes = 512 UInt64 elements (4kB)
// 2bpp: 8 pixel positions * 4 colours * 128 palettes = 4096 UInt64 element (32kB)
//   Pixel positions * colour is same for 1bpp and 2bpp so we could use 2bpp code for 1bpp as well (excluding table initialization)
// Would need to do some profiling to see if it's actually faster or if it uses too much cache
//   Usually we'd only care about one palette row = 256 bytes
//   Or do it bytewise?


    // renders a 16 hdot by 1 scanline region of CGA VRAM data into RGBI data
    // cursor is cursor output pin from CRTC
    // cursor_blink counts from 0..3 then repeats, changes every 8 frames (low bit cursor, high bit blink)
    UInt64 process(UInt32 input, UInt8 mode, UInt8 palette, int scanline,
        bool cursor, int cursorBlink, UInt8* latch)
    {
        if ((mode & 8) == 0)
            return 0;
        Character c;
        UInt64 r = 0;
        int x;
        int* pal;
        UInt8 temp;

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
                // The attribute byte is not latched for odd hchars, so byte column 1's data is repeated in byte column 3
                input = (input & 0x00ffffff) | ((input << 16) & 0xff000000);
                for (x = 0; x < 4; ++x) {
                    Byte b = input >> (x * 8);
                    for (int xx = 0; xx < 4; ++xx)
                        r += static_cast<UInt64>(pal[(b >> (6 - x * 2)) & 3] * 0x11) << (x*32 + xx*8);
                }
                break;
            case 0x10:
                // Improper: 40-column text mode with 1bpp graphics overlay
                c = getCharacter(input, mode, scanline, cursor, cursorBlink);
                for (x = 0; x < 8; ++x)
                    r += static_cast<UInt64>(((c.attribute >> ((c.bits & (128 >> x)) != 0 ? 0 : 4)) & 0xf) * 0x11) << (x * 8);
                // Shift register loaded from attribute latch before attribute latch loaded from VRAM.
                temp = input >> 8;
                input = (input & 0xff) + (*latch)*256;
                *latch = temp;
                for (x = 0; x < 2; ++x) {
                    Byte b = input >> (x * 8);
                    for (int xx = 0; xx < 8; ++xx) {
                        if ((b & (128 >> xx)) == 0)
                            r &= ~(static_cast<UInt64>(1) << (x*32 + xx*4));
                    }
                }
                break;
            case 0x11:
                // Improper: 80-column text mode with +HRES 1bpp graphics mode
                c = getCharacter(input, mode, scanline, cursor, cursorBlink);
                for (x = 0; x < 8; ++x)
                    r += static_cast<UInt64>((c.attribute >> ((c.bits & (128 >> x)) != 0 ? 0 : 4)) & 0xf) << (x * 4);
                c = getCharacter(input >> 16, mode, scanline, cursor, cursorBlink);
                for (x = 0; x < 8; ++x)
                    r += static_cast<UInt64>((c.attribute >> ((c.bits & (128 >> x)) != 0 ? 0 : 4)) & 0xf) << (x * 4 + 32);
                // Shift register loaded from attribute latch before attribute latch loaded from VRAM.
                temp = input >> 24;
                input = (input & 0x00ff00ff) + ((input & 0xff00)*65536) + (*latch)*256;
                *latch = temp;
                for (x = 0; x < 4; ++x) {
                    Byte b = input >> (x * 8);
                    for (int xx = 0; xx < 4; ++xx) {
                        if ((b & (64 >> (xx*2))) == 0)
                            r &= ~(static_cast<UInt64>(1) << (x*16 + xx*4));
                    }
                }
                break;
            case 0x12:
                // 1bpp graphics mode
                for (x = 0; x < 2; ++x) {
                    Byte b = input >> (x * 8);
                    for (int xx = 0; xx < 8; ++xx) {
                        if ((b & (128 >> xx)) != 0)
                            r += static_cast<UInt64>(palette & 0x0f) << (x*32 + xx*4);
                    }
                }
                break;
            case 0x13:
                // Improper: +HRES 1bpp graphics mode
                // The attribute byte is not latched for odd hchars, so byte column 1's data is repeated in byte column 3
                input = (input & 0x00ffffff) | ((input << 16) & 0xff000000);
                for (x = 0; x < 4; ++x) {
                    Byte b = input >> (x * 8);
                    for (int xx = 0; xx < 4; ++xx) {
                        if ((b & (64 >> (xx*2))) != 0)
                            r += static_cast<UInt64>(palette & 0x0f) << (x*16 + xx*4);
                    }
                }
                break;
        }
        return r;
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
