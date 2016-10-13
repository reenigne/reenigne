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


    // renders a 1 character by 1 scanline region of CGA VRAM data into RGBI
    // data.
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
                for (int x = 0; x < 4; ++x) {
                    r += static_cast<UInt64>(
                        pal[(input >> (6 - x*2)) & 3] * 0x11) << (x*8);
                }
                for (int x = 0; x < 4; ++x) {
                    r += static_cast<UInt64>(
                        pal[(input >> (14 - x*2)) & 3] * 0x11) << (32 + x*8);
                }
                break;
            case 0x03:
                // Improper: +HRES 2bpp graphics mode
                pal = &_palettes[((palette & 0x30) >> 2) + ((mode & 4) << 2)];
                *pal = palette & 0xf;
                for (int x = 0; x < 4; ++x) {
                    r += static_cast<UInt64>(
                        pal[(input >> (6 - x*2)) & 3]) << (x*4);
                }
                for (int x = 0; x < 4; ++x) {
                    r += static_cast<UInt64>(
                        pal[(input >> (14 - x*2)) & 3]) << (16 + x*4);
                }
                break;
            case 0x43:
                // Improper: +HRES 2bpp graphics mode
                pal = &_palettes[((palette & 0x30) >> 2) + ((mode & 4) << 2)];
                *pal = palette & 0xf;
                // The attribute byte is not latched for odd hchars, so the
                // second column uses the previously latched value.
                for (int x = 0; x < 4; ++x) {
                    r += static_cast<UInt64>(
                        pal[(input >> (6 - x*2)) & 3]) << (x*4);
                }
                for (int x = 0; x < 4; ++x) {
                    r += static_cast<UInt64>(
                        pal[(input >> (30 - x*2)) & 3]) << (16 + x*4);
                }
                break;
            case 0x10:
            case 0x50:
                // Improper: 40-column text mode with 1bpp graphics overlay
                c = getCharacter(input, mode, scanline, cursor, cursorBlink);
                fg = (c.attribute & 0x0f) * 0x11;
                bg = (c.attribute >> 4) * 0x11;
                for (x = 0; x < 8; ++x)
                    r += ((c.bits & (128 >> x)) != 0 ? fg : bg) << (x*8);
                // Shift register loaded from attribute latch before attribute
                // latch loaded from VRAM, so the second column uses the
                // previously latched value.
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
                // Shift register loaded from attribute latch before attribute
                // latch loaded from VRAM, so the second column uses the
                // previously latched value on both odd and even hchars.
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
                // Only the even bits have an effect.
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
                // Only the even bits have an effect.
                // The attribute byte is not latched for odd hchars, so the
                // second column uses the previously latched value.
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
        double maxV = 0;
        for (int x = 0; x < 1024; ++x)
            maxV = max(maxV, tableValue(x));
        double vBlack = tableValue(0);
        double vWhite = tableValue(1023);
        maxV = max(maxV, (vWhite - vBlack)*4/3 + vBlack);

        // v                p          q       f
        //
        //                  0          0
        // tableValue(0)    0.416              0
        // tableValue(1023) 1.46               1
        // maxV                      255       >= 4/3

        // p = n*v + m
        // 0.416 = n*vBlack + m
        // 1.46 = n*vWhite + m
        // 0.416*vWhite = n*vBlack*vWhite + m*vWhite
        // 1.46*vBlack = n*vBlack*vWhite + m*vBlack
        // 0.416*vWhite - 1.46*vBlack = m*(vWhite - vBlack)
        double m = (0.416*vWhite - 1.46*vBlack)/(vWhite - vBlack);
        double n = (1.46 - m)/vWhite;

        // q = v*a + b
        // q = p*c = (n*v + m)*c = n*c*v + m*c
        // a = n*c, b = m*c
        // 255 = (n*maxV + m)*c
        double c = 255.0/(n*maxV + m);
        double a = n*c;
        double b = m*c;
        for (int x = 0; x < 1024; ++x)
            _table[x] = byteClamp(tableValue(x)*a + b);
        _black = vBlack*a + b;
        _white = vWhite*a + b;

        // v                 p
        //
        // tableValue(0)     0.291
        // tableValue(0x1df) 1.04
        if (!_newCGA) {
            double vGrey = tableValue((0x77 << 2) + 3);
            m = (0.291*vGrey - 1.04*vBlack)/(vGrey - vBlack);
            n = (1.04 - m)/vGrey;
            a = n*c;
            b = m*c;
        }

        for (int x = 0; x < 0x77*4; ++x) {
            double q;
            if ((x & 0x10) != 0) {
                // Sync
                q = 0;
            }
            else {
                if ((x & 0x20) != 0) {
                    // Burst
                    q = tableValue((0x66 << 2) + (x & 3))*a + b;
                }
                else {
                    // Blank
                    q = tableValue(x & 3)*a + b;
                }
            }
            _syncTable[x] = byteClamp(q);
        }
    }
    Byte simulateCGA(int left, int right, int phase)
    {
        if ((left | right) < 16)
            return _table[((left & 15) << 6) + ((right & 15) << 2) + phase];
        return _syncTable[(left << 2) + phase];
    }
    Byte simulateHalfCGA(int left, Byte right, int phase)
    {
        int b = _table[((left & 15) << 6) + phase];
        int w = _table[((left & 15) << 6) + 0x3c + phase];
        int bb = _table[phase];
        int ww = _table[0x3fc + phase];
        return (right - bb)*(w - b)/(ww - bb) + b;
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

    // A real monitor will figure out appropriate black and white levels from
    // the sync and blanking levels. However, CGA voltages are somewhat outside
    // of NTSC spec, so there is a lot of variation between monitors in terms
    // of black and white levels. We return the actual CGA black and white
    // levels here so that emulated output devices can make sensible defaults
    // (setting black and white levels to sRGB (0, 0, 0) and (255, 255, 255)
    // respectively).
    double black() { return _black; }
    double white() { return _white; }
private:
    double tableValue(int x)
    {
        static unsigned char chromaData[256] = {
              2,  2,  2,  2, 114,174,  4,  3,   2,  1,133,135,   2,113,150,  4,
            133,  2,  1, 99, 151,152,  2,  1,   3,  2, 96,136, 151,152,151,152,
              2, 56, 62,  4, 111,250,118,  4,   0, 51,207,137,   1,171,209,  5,
            140, 50, 54,100, 133,202, 57,  4,   2, 50,153,149, 128,198,198,135,
             32,  1, 36, 81, 147,158,  1, 42,  33,  1,210,254,  34,109,169, 77,
            177,  2,  0,165, 189,154,  3, 44,  33,  0, 91,197, 178,142,144,192,
              4,  2, 61, 67, 117,151,112, 83,   4,  0,249,255,   3,107,249,117,
            147,  1, 50,162, 143,141, 52, 54,   3,  0,145,206, 124,123,192,193,
             72, 78,  2,  0, 159,208,  4,  0,  53, 58,164,159,  37,159,171,  1,
            248,117,  4, 98, 212,218,  5,  2,  54, 59, 93,121, 176,181,134,130,
              1, 61, 31,  0, 160,255, 34,  1,   1, 58,197,166,   0,177,194,  2,
            162,111, 34, 96, 205,253, 32,  1,   1, 57,123,125, 119,188,150,112,
             78,  4,  0, 75, 166,180, 20, 38,  78,  1,143,246,  42,113,156, 37,
            252,  4,  1,188, 175,129,  1, 37, 118,  4, 88,249, 202,150,145,200,
             61, 59, 60, 60, 228,252,117, 77,  60, 58,248,251,  81,212,254,107,
            198, 59, 58,169, 250,251, 81, 80, 100, 58,154,250, 251,252,252,252
        };
        static double intensity[4] = {
            77.175381, 88.654656, 166.564623, 174.228438};
        int phase = x & 3;
        int right = (x >> 2) & 15;
        int left = (x >> 6) & 15;
        int rc = right;
        int lc = left;
        if (_bw) {
            rc = (right & 8) | ((right & 7) != 0 ? 7 : 0);
            lc = (left & 8) | ((left & 7) != 0 ? 7 : 0);
        }
        double c = chromaData[((lc & 7) << 5) | ((rc & 7) << 2) | phase];
        double i = intensity[(left >> 3) | ((right >> 2) & 2)];
        if (!_newCGA)
            return c + i;
        double r = intensity[((left >> 2) & 1) | ((right >> 1) & 2)];
        double g = intensity[((left >> 1) & 1) | (right & 2)];
        double b = intensity[(left & 1) | ((right << 1) & 1)];
        return (c/0.72)*0.29 + (i/0.28)*0.32 + (r/0.28)*0.1 + (g/0.28)*0.22 +
            (b/0.28)*0.07;
    }

    bool _newCGA;
    bool _bw;
    Byte _table[1024];
    Byte _syncTable[0x77*4];
    double _black;
    double _white;
};

#endif // INCLUDED_CGA_H
