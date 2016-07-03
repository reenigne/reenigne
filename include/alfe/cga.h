#include "alfe/main.h"

#ifndef INCLUDED_CGA_H
#define INCLUDED_CGA_H

class CGASequencer
{
public:
    CGASequencer()
    {
        static Byte palettes[] = {
            0, 2, 4, 6, 0, 10, 12, 14, 0, 3, 5, 7, 0, 11, 13, 15,
            0, 3, 4, 7, 0, 11, 12, 15, 0, 3, 4, 7, 0, 11, 12, 15};
        memcpy(_palettes, palettes, 32);
    }
    void setROM(File rom) { _cgaROM = rom.contents(); }
    const Byte* romData() { return &_cgaROM[0x300*8]; }

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
    // cursorBlink counts from 0..3 then repeats, changes every 8 frames (low
    // bit cursor, high bit blink)
    // mode bit 6 is phase
    // input bits 0-7 are first/character byte
    // input bits 8-15 are second/attribute byte
    // input bits 24-31 are previous (latched) attribute byte
    UInt64 process(UInt32 input, UInt8 mode, UInt8 palette, int scanline,
        bool cursor, int cursorBlink)
    {
        if ((mode & 8) == 0)
            return 0;
        Character c;
        UInt64 r = 0;
        int x;
        Byte* pal;
        UInt64 fg;
        UInt64 bg;

        switch (mode & 0x53) {
            case 0x00:
            case 0x40:
                // 40-column text mode
                c = getCharacter(input, mode, scanline, cursor, cursorBlink);
                fg = (c.attribute & 0x0f) * 0x11;
                bg = (c.attribute >> 4) * 0x11;
                for (x = 0; x < 8; ++x)
                    r += ((c.bits & (0x80 >> x)) != 0 ? fg : bg) << (x*8);
                break;
            case 0x01:
            case 0x41:
                // 80-column text mode
                c = getCharacter(input, mode, scanline, cursor, cursorBlink);
                fg = c.attribute & 0x0f;
                bg = c.attribute >> 4;
                for (x = 0; x < 8; ++x)
                    r += ((c.bits & (0x80 >> x)) != 0 ? fg : bg) << (x*4);
                break;
            case 0x02:
            case 0x42:
                // 2bpp graphics mode
                pal = &_palettes[((palette & 0x30) >> 2) + ((mode & 4) << 2)];
                *pal = palette & 0xf;
                for (int x = 0; x < 4; ++x)
                    r += static_cast<UInt64>(pal[(input >> (6 - x*2)) & 3] * 0x11) << (x*8);
                for (int x = 0; x < 4; ++x)
                    r += static_cast<UInt64>(pal[(input >> (14 - x*2)) & 3] * 0x11) << (32 + x*8);
                break;
            case 0x03:
                // Improper: +HRES 2bpp graphics mode
                pal = &_palettes[((palette & 0x30) >> 2) + ((mode & 4) << 2)];
                *pal = palette & 0xf;
                // The attribute byte is not latched for odd hchars, so byte column 1's data is repeated in byte column 3
                for (int x = 0; x < 4; ++x)
                    r += static_cast<UInt64>(pal[(input >> (6 - x*2)) & 3]) << (x*4);
                for (int x = 0; x < 4; ++x)
                    r += static_cast<UInt64>(pal[(input >> (14 - x*2)) & 3]) << (16 + x*4);
                break;
            case 0x43:
                // Improper: +HRES 2bpp graphics mode
                pal = &_palettes[((palette & 0x30) >> 2) + ((mode & 4) << 2)];
                *pal = palette & 0xf;
                // The attribute byte is not latched for odd hchars, so byte column 1's data is repeated in byte column 3
                for (int x = 0; x < 4; ++x)
                    r += static_cast<UInt64>(pal[(input >> (6 - x*2)) & 3]) << (x*4);
                for (int x = 0; x < 4; ++x)
                    r += static_cast<UInt64>(pal[(input >> (30 - x*2)) & 3]) << (16 + x*4);
                break;
            case 0x10:
            case 0x50:
                // Improper: 40-column text mode with 1bpp graphics overlay
                c = getCharacter(input, mode, scanline, cursor, cursorBlink);
                fg = (c.attribute & 0x0f) * 0x11;
                bg = (c.attribute >> 4) * 0x11;
                for (x = 0; x < 8; ++x)
                    r += ((c.bits & (128 >> x)) != 0 ? fg : bg) << (x*8);
                // Shift register loaded from attribute latch before attribute latch loaded from VRAM.
                for (int x = 0; x < 8; ++x) {
                    if ((input & (0x80 >> x)) == 0)
                        r &= ~(static_cast<UInt64>(0x0f) << (x*4));
                }
                for (int x = 0; x < 8; ++x) {
                    if ((input & (0x80000000 >> x)) == 0)
                        r &= ~(static_cast<UInt64>(0x0f) << (32 + x*4));
                }
                break;
            case 0x11:
            case 0x51:
                // Improper: 80-column text mode with +HRES 1bpp graphics mode
                c = getCharacter(input, mode, scanline, cursor, cursorBlink);
                fg = c.attribute & 0x0f;
                bg = c.attribute >> 4;
                for (x = 0; x < 8; ++x)
                    r += ((c.bits & (128 >> x)) != 0 ? fg : bg) << (x*4);
                // Shift register loaded from attribute latch before attribute latch loaded from VRAM.
                for (int x = 0; x < 4; ++x) {
                    if ((input & (0x40 >> (x*2))) == 0)
                        r &= ~(static_cast<UInt64>(0x0f) << (x*4));
                }
                for (int x = 0; x < 4; ++x) {
                    if ((input & (0x40000000 >> (x*2))) == 0)
                        r &= ~(static_cast<UInt64>(0x0f) << (16 + x*4));
                }
                break;
            case 0x12:
            case 0x52:
                // 1bpp graphics mode
                for (int x = 0; x < 8; ++x) {
                    if ((input & (0x80 >> x)) != 0)
                        r += static_cast<UInt64>(palette & 0x0f) << (x*4);
                }
                for (int x = 0; x < 8; ++x) {
                    if ((input & (0x8000 >> x)) != 0)
                        r += static_cast<UInt64>(palette & 0x0f) << (32 + x*4);
                }
                break;
            case 0x13:
                // Improper: +HRES 1bpp graphics mode
                // The attribute byte is not latched for odd hchars, so byte column 1's data is repeated in byte column 3
                for (int x = 0; x < 4; ++x) {
                    if ((input & (0x40 >> (x*2))) != 0)
                        r += static_cast<UInt64>(palette & 0x0f) << (x*4);
                }
                for (int x = 0; x < 4; ++x) {
                    if ((input & (0x4000 >> (x*2))) != 0)
                        r += static_cast<UInt64>(palette & 0x0f) << (16 + x*4);
                }
                break;
            case 0x53:
                // Improper: +HRES 1bpp graphics mode
                // The attribute byte is not latched for odd hchars, so byte column 1's data is repeated in byte column 3
                for (int x = 0; x < 4; ++x) {
                    if ((input & (0x40 >> (x*2))) != 0)
                        r += static_cast<UInt64>(palette & 0x0f) << (x*4);
                }
                for (int x = 0; x < 4; ++x) {
                    if ((input & (0x40000000 >> (x*2))) != 0)
                        r += static_cast<UInt64>(palette & 0x0f) << (16 + x*4);
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

    Character getCharacter(UInt16 input, UInt8 mode, int scanline, bool cursor,
        int cursorBlink)
    {
        Character c;
        c.bits = romData()[(input & 0xff)*8 + (scanline & 7)];
        c.attribute = input >> 8;
        if (cursor && ((cursorBlink & 1) != 0))
            c.bits = 0xff;
        else {
            if ((mode & 0x20) != 0 && (c.attribute & 0x80) != 0 &&
                (cursorBlink & 2) != 0 && !cursor)
                c.bits = 0;
        }
        if ((mode & 0x20) != 0)
            c.attribute &= 0x7f;
        return c;
    }

    String _cgaROM;
    Byte _palettes[32];
};

class CGAComposite
{
public:
    CGAComposite() : _newCGA(false), _bw(false) { }
    void initChroma()
    {
        static Byte chromaData[256] = {
             65, 11, 62,  6, 121, 87, 63,  6,  60,  9,120, 65,  61, 59,129,  5,
            121,  6, 58, 58, 134, 65, 62,  6,  57,  9,108, 72, 126, 72,125, 77,
             60, 98,160,  6, 113,195,194,  8,  53, 94,218, 64,  56,152,225,  5,
            118, 90,147, 56, 115,154,156,  0,  52, 92,197, 73, 107,156,213, 62,
            119, 10, 97,122, 178, 77, 60, 87, 119, 12,174,205, 119, 58,135, 88,
            185,  6, 54,158, 194, 67, 57, 87, 114, 10,101,168, 181, 67,114,160,
             64,  8,156,109, 121, 73,177,122,  58,  8,244,207,  65, 58,251,137,
            127,  5,141,156, 126, 58,144, 97,  57,  7,189,168, 106, 55,201,162,
            163,124, 62, 10, 185,159, 59,  8, 135,104,128, 80, 119,142,140,  5,
            241,141, 59, 57, 210,160, 61,  5, 137,108,103, 61, 177,140,110, 65,
             59,107,124,  4, 180,201,122,  6,  52,104,194, 77,  55,159,197,  3,
            130,128,121, 51, 174,197,123,  3,  52,100,162, 62, 101,156,171, 51,
            173, 11, 60,113, 199, 93, 58, 77, 167, 11,118,196, 132, 63,129, 74,
            255,  9, 54,195, 192, 55, 59, 74, 183, 14,103,199, 206, 74,118,154,
            153,108,156,105, 255,202,188,123, 143,107,246,203, 164,208,250,129,
            209,103,148,157, 253,195,171,120, 163,106,196,207, 245,202,249,208
        };

        static double intensity[4] = {
            0, 0.047932237386703491, 0.15110087022185326, 0.18384206667542458};

        static const double minChroma = 0.070565;
        static const double maxChroma = 0.727546;

        for (int x = 0; x < 1024; ++x) {
            int phase = x & 3;
            int right = (x >> 2) & 15;
            int left = (x >> 6) & 15;
            int rc = right;
            int lc = left;
            if (_bw) {
                rc = (right & 8) | ((right & 7) != 0 ? 7 : 0);
                lc = (left & 8) | ((left & 7) != 0 ? 7 : 0);
            }
            double c = minChroma +
                chromaData[((lc & 7) << 5) | ((rc & 7) << 2) | phase]*
                (maxChroma-minChroma)/256.0;
            double i = intensity[(left >> 3) | ((right >> 2) & 2)];
            if (!_newCGA)
                _table[x] = byteClamp(static_cast<int>((c + i)*256));
            else {
                double r = intensity[((left >> 2) & 1) | ((right >> 1) & 2)];
                double g = intensity[((left >> 1) & 1) | (right & 2)];
                double b = intensity[(left & 1) | ((right << 1) & 1)];
                _table[x] = byteClamp(static_cast<int>(((c/0.72)*0.29 +
                    (i/0.28)*0.32 + (r/0.28)*0.1 + (g/0.28)*0.22 +
                    (b/0.28)*0.07)*256));
            }
        }
    }
    Byte simulateCGA(int left, int right, int phase)
    {
        return _table[((left & 15) << 6) | ((right & 15) << 2) | phase];
    }
    void simulateLine(const Byte* rgbi, Byte* ntsc, int length, int phase)
    {
        for (int x = 0; x < length; ++x) {
            phase = (phase + 1) & 3;
            int left = *rgbi;
            ++rgbi;
            int right = *rgbi;
            *ntsc = simulateCGA(left, right, phase);
            ++ntsc;
        }
    }
    void decode(int pixels, int* s)
    {
        int rgbi[4];
        rgbi[0] = pixels & 15;
        rgbi[1] = (pixels >> 4) & 15;
        rgbi[2] = (pixels >> 8) & 15;
        rgbi[3] = (pixels >> 12) & 15;
        for (int t = 0; t < 4; ++t)
            s[t] = simulateCGA(rgbi[t], rgbi[(t+1)&3], t);
    }

    void setNewCGA(bool newCGA)
    {
        if (newCGA == _newCGA)
            return;
        _newCGA = newCGA;
        initChroma();
    }
    bool getNewCGA() { return _newCGA; }

    void setBW(bool bw)
    {
        if (bw == _bw)
            return;
        _bw = bw;
        initChroma();
    }
private:
    bool _newCGA;
    bool _bw;
    int _table[1024];
};

#endif // INCLUDED_CGA_H
