#include "alfe/main.h"

#ifndef INCLUDED_CGA_H
#define INCLUDED_CGA_H

#include "alfe/user.h"
#include "alfe/ntsc_decode.h"
#include "alfe/scanlines.h"
#include "alfe/wrap.h"
#include "alfe/timer.h"
#include "alfe/bitmap_png.h"

static const SRGB rgbiPalette[16] = {
	SRGB(0x00, 0x00, 0x00), SRGB(0x00, 0x00, 0xaa),
	SRGB(0x00, 0xaa, 0x00), SRGB(0x00, 0xaa, 0xaa),
	SRGB(0xaa, 0x00, 0x00), SRGB(0xaa, 0x00, 0xaa),
	SRGB(0xaa, 0x55, 0x00), SRGB(0xaa, 0xaa, 0xaa),
	SRGB(0x55, 0x55, 0x55), SRGB(0x55, 0x55, 0xff),
	SRGB(0x55, 0xff, 0x55), SRGB(0x55, 0xff, 0xff),
	SRGB(0xff, 0x55, 0x55), SRGB(0xff, 0x55, 0xff),
	SRGB(0xff, 0xff, 0x55), SRGB(0xff, 0xff, 0xff)};

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
    Byte simulateRightHalfCGA(Byte left, int right, int phase)
    {
        int b = _table[((right & 15) << 2) + phase];
        int w = _table[0x3c0 + ((right & 15) << 2) + phase];
        int bb = _table[phase];
        int ww = _table[0x3fc + phase];
        return (left - bb)*(w - b)/(ww - bb) + b;
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

class CGAData : Uncopyable
{
public:
    CGAData() : _total(1) { reset(); }
    void reset()
    {
        _root.reset();
        _endAddress = 0;
    }
    // Output RGBI values:
    //   0-15: normal active data
    //   16-54: blanking
    //     bit 0: CRT hsync
    //     bit 1: CRT vsync
    //     bit 2: composite sync (CRT hsync ^ CRT vsync)
    //     bit 3: colour burst
    //     bit 4: CRTC hsync
    //     bit 5: CRTC vsync
    //     bit 6: CRTC hsync | CRTC vsync
    //   Only the following blanking values actually occur:
    //     0x50: CRTC hsync
    //     0x55: CRT hsync (composite sync)
    //     0x58: Colour burst (suppressed during CRTC vsync)
    //     0x60: CRTC vsync
    //     0x66: CRT vsync (composite sync)
    //     0x70: CRTC hsync + CRTC vsync
    //     0x73: CRT hsync + CRT vsync (no composite sync)
    //     0x75: CRT hsync + CRTC vsync (composite sync)
    //     0x76: CRTC hsync + CRT vsync (composite sync)
    void output(int t, int n, Byte* rgbi, CGASequencer* sequencer, int phase)
    {
        Lock lock(&_mutex);

        State state;
        state._n = n;
        state._rgbi = rgbi;
        state._t = t;
        state._addresses = _endAddress - registerLogCharactersPerBank;
        state._data.allocate(state._addresses);
        state._sequencer = sequencer;
        state._phase = phase != 0 ? 0x40 : 0;
        for (const auto& c : _root._changes)
            c.getData(&state._data, 0, registerLogCharactersPerBank,
                state._addresses, 0);
        state.reset();
        if (_root._right != 0)
            _root._right->output(0, 0, _total, &state);
        state.runTo(t + n);
    }
    enum {
        registerLogCharactersPerBank = -26,  // log(characters per bank)/log(2)
        registerScanlinesRepeat,
        registerHorizontalTotalHigh,
        registerHorizontalDisplayedHigh,
        registerHorizontalSyncPositionHigh,
        registerVerticalTotalHigh,
        registerVerticalDisplayedHigh,
        registerVerticalSyncPositionHigh,
        registerMode,                        // port 0x3d8
        registerPalette,                     // port 0x3d9
        registerHorizontalTotal,             // CRTC register 0x00
        registerHorizontalDisplayed,         // CRTC register 0x01
        registerHorizontalSyncPosition,      // CRTC register 0x02
        registerHorizontalSyncWidth,         // CRTC register 0x03
        registerVerticalTotal,               // CRTC register 0x04
        registerVerticalTotalAdjust,         // CRTC register 0x05
        registerVerticalDisplayed,           // CRTC register 0x06
        registerVerticalSyncPosition,        // CRTC register 0x07
        registerInterlaceMode,               // CRTC register 0x08
        registerMaximumScanline,             // CRTC register 0x09
        registerCursorStart,                 // CRTC register 0x0a
        registerCursorEnd,                   // CRTC register 0x0b
        registerStartAddressHigh,            // CRTC register 0x0c
        registerStartAddressLow,             // CRTC register 0x0d
        registerCursorAddressHigh,           // CRTC register 0x0e
        registerCursorAddressLow             // CRTC register 0x0f
    };                                       // 0 onwards: VRAM
    void change(int t, int address, Byte data)
    {
        change(t, address, 1, &data);
    }
    void change(int t, int address, int count, const Byte* data)
    {
        Lock lock(&_mutex);
        changeNoLock(t, address, count, data);
    }
    void remove(int t, int address, int count = 1)
    {
        Lock lock(&_mutex);
        _root.remove(t, address, count, 0, _total);
    }
    void setTotals(int total, int pllWidth, int pllHeight)
    {
        Lock lock(&_mutex);
        if (_root._right != 0)
            _root._right->resize(_total, total);
        _total = total;
        _pllWidth = pllWidth;
        _pllHeight = pllHeight;
    }
    int getTotal()
    {
        Lock lock(&_mutex);
        return _total;
    }
    int getPLLWidth() { return _pllWidth; }
    int getPLLHeight() { return _pllHeight; }
    void save(File file)
    {
        Lock lock(&_mutex);
        AppendableArray<Byte> data;
        data.append(reinterpret_cast<const Byte*>("CGAD"), 4);
        DWord version = 0;
        data.append(reinterpret_cast<const Byte*>(&version), 4);
        DWord total = _total;
        data.append(reinterpret_cast<const Byte*>(&total), 4);
        DWord pllWidth = _pllWidth;
        data.append(reinterpret_cast<const Byte*>(&pllWidth), 4);
        DWord pllHeight = _pllHeight;
        data.append(reinterpret_cast<const Byte*>(&pllHeight), 4);
        _root.save(&data, 0, 0, _total);
        file.openWrite().write(data);
    }
    void load(File file)
    {
        Lock lock(&_mutex);
        _root.reset();
        Array<Byte> data;
        file.readIntoArray(&data);
        if (deserialize(&data, 0) != *reinterpret_cast<const DWord*>("CGAD"))
            throw Exception(file.path() + " is not a CGAData file.");
        if (deserialize(&data, 4) != 0)
            throw Exception(file.path() + " is too new for this program.");
        _total = deserialize(&data, 8);
        _pllWidth = deserialize(&data, 12);
        _pllHeight = deserialize(&data, 16);
        int offset = 20;
        do {
            if (offset == data.count())
                return;
            int t = deserialize(&data, offset);
            int address = deserialize(&data, offset + 4);
            int count = deserialize(&data, offset + 8);
            int length = 12 + count;
            if (offset + length > data.count())
                throw Exception(file.path() + " is truncated.");
            changeNoLock(t, address, count, &data[offset + 12]);
            offset += (length + 3) & ~3;
        } while (true);
    }
    void saveVRAM(File file)
    {
        file.openWrite().write(getData(0, _endAddress, 0));
    }
    void loadVRAM(File file)
    {
        Lock lock(&_mutex);
        _root.reset();
        Array<Byte> data;
        file.readIntoArray(&data);
        changeNoLock(0, 0, data.count(), &data[0]);
    }
    Byte getDataByte(int address, int t = 0)
    {
        return getData(address, 1, t)[0];
    }
    Array<Byte> getData(int address, int count, int t = 0)
    {
        Lock lock(&_mutex);
        Array<Byte> result(count);
        Array<bool> gotResult(count);
        for (int i = 0; i < count; ++i)
            gotResult[i] = false;
        _root.getData(&result, &gotResult, t, address, count, 0, _total, 0);
        return result;
    }

private:
    void changeNoLock(int t, int address, int count, const Byte* data)
    {
        _root.change(t, address, count, data, 0, _total);
        if (address + count > _endAddress) {
            _endAddress = address + count;
            _root.ensureAddresses(registerLogCharactersPerBank, _endAddress);
        }
    }
    int deserialize(Array<Byte>* data, int offset)
    {
        if (data->count() < offset + 4)
            return -1;
        return *reinterpret_cast<DWord*>(&(*data)[offset]);
    }

    struct Change
    {
        Change() { }
        Change(int address, const Byte* data, int count)
            : _address(address), _data(count)
        {
            memcpy(&_data[0], data, count);
        }
        int count() const { return _data.count(); }
        int start() const { return _address; }
        int end() const { return _address + count(); }
        int getData(Array<Byte>* result, Array<bool>* gotResult, int address,
            int count, int gotCount) const
        {
            int s = max(address, start());
            int e = min(address + count, end());
            for (int a = s; a < e; ++a) {
                int i = a - address;
                if (gotResult == 0 || !(*gotResult)[i]) {
                    (*result)[i] = _data[a - _address];
                    if (gotResult != 0)
                        (*gotResult)[i] = true;
                    ++gotCount;
                }
            }
            return gotCount;
        }

        int _address;
        Array<Byte> _data;
    };
    struct State
    {
        void latch()
        {
            int vRAMAddress = _memoryAddress << 1;
            int bytesPerBank = 2 << dat(registerLogCharactersPerBank);
            if ((dat(registerMode) & 2) != 0) {
                if ((_rowAddress & 1) != 0)
                    vRAMAddress |= bytesPerBank;
                else
                    vRAMAddress &= ~bytesPerBank;
            }
            vRAMAddress &= (bytesPerBank << 1) - 1;
            _latch = (_latch << 16) + dat(vRAMAddress) +
                (dat(vRAMAddress + 1) << 8);
        }
        void startOfFrame()
        {
            _memoryAddress = (dat(registerStartAddressHigh) << 8) +
                dat(registerStartAddressLow);
            _leftMemoryAddress = _memoryAddress;
            _rowAddress = 0;
            _row = 0;
            _scanlineIteration = 0;
        }
        void reset()
        {
            startOfFrame();
            _character = 0;
            _hdot = 0;
            _state = 0;
            latch();
        }
        void runTo(int t)
        {
            Byte mode = dat(registerMode);
            int hdots = (mode & 1) != 0 ? 8 : 16;
            while (_t < t) {
                int c = min(hdots, _hdot + t - _t);
                if (_state == 0) {
                    UInt64 r = _sequencer->process(_latch, mode | _phase,
                        dat(registerPalette), _rowAddress, false, 0);
                    for (; _hdot < c; ++_hdot) {
                        *_rgbi = (r >> (_hdot * 4)) & 0x0f;
                        ++_rgbi;
                    }
                }
                else {
                    int v = 0;
                    if ((_state & 0x18) == 0) {
                        v = (mode & 0x10) != 0 ? 0 :
                            dat(registerPalette) & 0xf;
                    }
                    else {
                        v = (_state & 0x10) + ((_state & 0x20) >> 1);
                        static Byte sync[48] = {
                            0x50, 0x50, 0x55, 0x55, 0x55, 0x55, 0x50, 0x58,
                            0x58, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50,

                            0x70, 0x70, 0x75, 0x75, 0x75, 0x75, 0x70, 0x70,
                            0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70,

                            0x76, 0x76, 0x73, 0x73, 0x73, 0x73, 0x76, 0x76,
                            0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76};
                        if ((_state & 8) != 0) {
                            if ((mode & 1) != 0) {
                                v = sync[((_hSync + (_phase >> 6)) >> 1) + v];
                            }
                            else
                                v = sync[_hSync + v];
                        }
                        else
                            v = (_state & 0x20) != 0 ? 0x66 : 0x60;
                    }
                    memset(_rgbi, v, c);
                    _rgbi += c;
                    _hdot += c;
                }
                _t += c;
                if (_t == t)
                    break;
                _hdot = 0;

                // Emulate CRTC
                if (_character == dat(registerHorizontalTotal) +
                    (dat(registerHorizontalTotalHigh) << 8)) {
                    _state &= ~1;
                    _character = 0;
                    if ((_state & 0x10) != 0) {
                        // Vertical sync active
                        ++_vSync;
                        if ((_vSync & 0x0f) == 0) {
                            // End of vertical sync
                            _state &= ~0x30;
                        }
                    }
                    ++_scanlineIteration;
                    if (_scanlineIteration == dat(registerScanlinesRepeat)) {
                        _scanlineIteration = 0;
                        if (_rowAddress == dat(registerMaximumScanline)) {
                            // End of row
                            _rowAddress = 0;
                            if (_row == dat(registerVerticalTotal) +
                                (dat(registerVerticalTotalHigh) << 8)) {
                                _state |= 4;
                                _adjust = 0;
                                _latch = 0;
                            }
                            ++_row;
                            if (_row == dat(registerVerticalDisplayed) +
                                (dat(registerVerticalDisplayedHigh) << 8)) {
                                _state |= 2;
                                _latch = 0;
                            }
                            if (_row == dat(registerVerticalSyncPosition) +
                                (dat(registerVerticalSyncPositionHigh) << 8)) {
                                _state |= 0x10;
                                _vSync = 0;
                            }
                            _memoryAddress = _nextRowMemoryAddress;
                            _leftMemoryAddress = _nextRowMemoryAddress;
                        }
                        else {
                            ++_rowAddress;
                            _memoryAddress = _leftMemoryAddress;
                        }
                    }
                    else
                        _memoryAddress = _leftMemoryAddress;
                    if ((_state & 4) != 0) {
                        // Vertical total adjust active
                        if (_adjust == dat(registerVerticalTotalAdjust)) {
                            startOfFrame();
                            _state &= ~4;
                        }
                        else
                            ++_adjust;
                    }
                }
                else {
                    ++_character;
                    ++_memoryAddress;
                }
                _phase ^= 0x40;
                if (_character == dat(registerHorizontalDisplayed) +
                    (dat(registerHorizontalDisplayedHigh) << 8)) {
                    _state |= 1;
                    _nextRowMemoryAddress = _memoryAddress;
                    _latch = 0;
                }
                if ((_state & 8) != 0) {
                    // Horizontal sync active
                    ++_hSync;
                    bool crtSync = _hSync == 2;
                    if ((mode & 1) != 0)
                        crtSync = (((_hSync + (_phase >> 6)) >> 1) == 2);
                    if (crtSync && (_state & 0x10) != 0) {
                        if (_vSync == 0)
                            _state |= 0x20;
                        else {
                            if (_vSync == 3)
                                _state &= ~0x20;
                        }
                    }
                    if ((_hSync & 0x0f) == dat(registerHorizontalSyncWidth))
                        _state &= ~8;
                }
                if (_character == dat(registerHorizontalSyncPosition) +
                    (dat(registerHorizontalSyncPositionHigh) << 8)) {
                    _state |= 8;
                    _hSync = 0;
                }
                if (_state == 0)
                    latch();
            }
        }
        Byte dat(int address)
        {
            return _data[address - registerLogCharactersPerBank];
        }

        int _memoryAddress;
        int _leftMemoryAddress;
        int _nextRowMemoryAddress;
        int _rowAddress;
        int _character;
        int _adjust;
        int _hSync;
        int _vSync;
        int _row;
        int _hdot;
        int _n;
        int _t;
        int _phase;
        int _addresses;
        int _scanlineIteration;

        // _state bits:
        //    0 = we are in horizontal overscan
        //    1 = we are in vertical overscan
        //    2 = we are in vertical total adjust
        //    3 = we are in horizontal sync
        //    4 = we are in vertical CRTC sync
        //    5 = we are in vertical CRT sync
        int _state;
        UInt32 _latch;
        Byte* _rgbi;
        Array<Byte> _data;
        CGASequencer* _sequencer;
    };
    struct Node
    {
        Node() : _left(0), _right(0) { }
        void reset()
        {
            if (_left != 0)
                delete _left;
            if (_right != 0)
                delete _right;
        }
        ~Node() { reset(); }
        void findChanges(int address, int count, int* start, int* end)
        {
            int lowStart = 0;
            int lowEnd = _changes.count() - 1;
            while (lowStart < lowEnd) {
                int lowTest = lowStart + (lowEnd - lowStart)/2;
                if (_changes[lowTest].end() <= address) {
                    lowStart = lowTest + 1;
                    continue;
                }
                if (_changes[lowTest].start() >= address + count) {
                    lowEnd = lowTest - 1;
                    continue;
                }
                lowStart = lowTest;
                break;
            }
            int highStart = lowStart;
            int highEnd = _changes.count() - 1;
            while (highStart < highEnd) {
                int highTest = highStart + (highEnd - highStart)/2;
                if (_changes[highTest].start() >= address + count) {
                    highStart = highTest - 1;
                    continue;
                }
                if (_changes[highTest].end() <= address) {
                    highEnd = highTest + 1;
                    continue;
                }
                highStart = highTest;
                break;
            }
            *start = lowStart;
            *end = highEnd;
        }
        void change(int t, int address, int count, const Byte* data,
            int leftTotal, int rightTotal)
        {
            if (t > 0) {
                if (_right == 0)
                    _right = new Node();
                int rlTotal = roundUpToPowerOf2(rightTotal) / 2;
                _right->change(t - rlTotal, address, count, data, rlTotal,
                    rightTotal - rlTotal);
                return;
            }
            if (t == 0) {
                int start, end;
                findChanges(address, count, &start, &end);
                if (start == end) {
                    Change* c = &_changes[start];
                    if (address >= c->start() && address + count <= c->end()) {
                        memcpy(&c->_data[0] + address - c->start(), data,
                            count);
                        return;
                    }
                }
                if (start <= end) {
                    int startAddress = min(address, _changes[start].start());
                    Change e = _changes[end];
                    int e2 = address + count;
                    int endAddress = max(e2, e.end());
                    Change c;
                    c._data.allocate(endAddress - startAddress);
                    c._address = startAddress;
                    int a = 0;
                    if (startAddress < address) {
                        a = address - startAddress;
                        memcpy(&c._data[0], &_changes[start]._data[0],
                            min(a, _changes[start].count()));
                    }
                    memcpy(&c._data[a], data, count);
                    if (endAddress > e2) {
                        int offset = e2 - e.start();
                        if (offset >= 0) {
                            memcpy(&c._data[a + count],
                                &e._data[e2 - e.start()], endAddress - e2);
                        }
                        else {
                            memcpy(&c._data[a + count - offset],
                                &e._data[0], endAddress - e.start());
                        }
                    }
                    if (start < end) {
                        Array<Change> changes(_changes.count() + start - end);
                        for (int i = 0; i < start; ++i)
                            changes[i] = _changes[i];
                        changes[start] = c;
                        for (int i = start + 1; i < changes.count(); ++i)
                            changes[i] = _changes[i + end - start];
                        _changes = changes;
                    }
                    else
                        _changes[start] = c;
                }
                else {
                    Array<Change> changes(_changes.count() + 1);
                    for (int i = 0; i < start; ++i)
                        changes[i] = _changes[i];
                    changes[start] = Change(address, data, count);
                    for (int i = start; i < _changes.count(); ++i)
                        changes[i + 1] = _changes[i];
                    _changes = changes;
                }
                return;
            }
            if (_left == 0)
                _left = new Node();
            int llTotal = roundUpToPowerOf2(leftTotal) / 2;
            int lrTotal = leftTotal - llTotal;
            _left->change(t + lrTotal, address, count, data, llTotal, lrTotal);
        }
        void remove(int t, int address, int count, int leftTotal,
            int rightTotal)
        {
            if (t > 0) {
                if (_right != 0) {
                    int rlTotal = roundUpToPowerOf2(rightTotal) / 2;
                    _right->remove(t - rlTotal, address, count, rlTotal,
                        rightTotal - rlTotal);
                }
                return;
            }
            if (t == 0) {
                int start, end;
                findChanges(address, count, &start, &end);
                int deleteStart = end, deleteEnd = start;
                for (int i = start; i < end; ++i) {
                    int newCount;
                    Change c = _changes[i];
                    int e2 = address + count;
                    if (address < c._address) {
                        newCount = max(0, c.end() - e2);
                        int offset = c.count() - newCount;
                        _changes[i] = Change(c._address + offset,
                            &c._data[offset], newCount);
                    }
                    else {
                        newCount = address - c.start();
                        if (newCount < c.count()) {
                            _changes[i] =
                                Change(c._address, &c._data[0], newCount);
                            if (e2 < c.end()) {
                                Array<Change> changes(_changes.count() + 1);
                                for (int j = 0; j <= i; ++j)
                                    changes[j] = _changes[j];
                                changes[i + 1] =
                                    Change(e2, &c._data[newCount + count],
                                        c.end() - e2);
                                for (int j = i + 1; j < _changes.count(); ++j)
                                    changes[j + 1] = _changes[j];
                                _changes = changes;
                            }
                        }
                    }
                    if (_changes[i].count() == 0) {
                        deleteStart = min(deleteStart, i);
                        deleteEnd = max(deleteStart, i + 1);
                    }
                }
                int deleteCount = deleteEnd - deleteStart;
                if (deleteCount > 0) {
                    Array<Change> changes(_changes.count() - deleteCount);
                    for (int i = 0; i < deleteStart; ++i)
                        changes[i] = _changes[i];
                    for (int i = deleteEnd; i < _changes.count(); ++i)
                        changes[i - deleteCount] = _changes[i];
                    _changes = changes;
                }
                return;
            }
            if (_left != 0) {
                int llTotal = roundUpToPowerOf2(leftTotal) / 2;
                int lrTotal = leftTotal - llTotal;
                _left->remove(t + lrTotal, address, count, llTotal, lrTotal);
            }
        }
        int getData(Array<Byte>* result, Array<bool>* gotResult, int t,
            int address, int count, int leftTotal, int rightTotal,
            int gotCount)
        {
            if (t > 0 && _right != 0) {
                int rlTotal = roundUpToPowerOf2(rightTotal) / 2;
                int c = _right->getData(result, gotResult, t - rlTotal,
                    address, count, rlTotal, rightTotal - rlTotal, gotCount);
                if (c == gotCount)
                    return c;
                gotCount = c;
            }
            if (t >= 0 && _changes.count() != 0) {
                int start, end;
                findChanges(address, count, &start, &end);
                for (int i = start; i <= end; ++i) {
                    gotCount = _changes[i].getData(result, gotResult, address,
                        count, gotCount);
                }
            }
            if (_left != 0) {
                int llTotal = roundUpToPowerOf2(leftTotal) / 2;
                int lrTotal = leftTotal - llTotal;
                gotCount = _left->getData(result, gotResult, t + lrTotal,
                    address, count, llTotal, lrTotal, gotCount);
            }
            return gotCount;
        }
        void resize(int oldTotal, int newTotal)
        {
            int lTotal = roundUpToPowerOf2(oldTotal) / 2;
            do {
                if (lTotal < newTotal)
                    break;
                if (_right != 0)
                    delete _right;
                Node* left = _left;
                *this = *left;
                left->_left = 0;
                left->_right = 0;
                delete left;
                oldTotal = lTotal;
                lTotal /= 2;
            } while (true);
            do {
                int rTotal = oldTotal - lTotal;
                int newLTotal = roundUpToPowerOf2(newTotal) / 2;
                if (lTotal >= newLTotal)
                    break;
                Node* newNode = new Node();
                newNode->_left = this;
                *this = *newNode;
                oldTotal += lTotal;
                lTotal *= 2;
            } while (true);
            if (_right != 0)
                _right->resize(oldTotal - lTotal, newTotal - lTotal);
        }
        void save(AppendableArray<Byte>* array, int t, int leftTotal,
            int rightTotal)
        {
            if (_left != 0) {
                int llTotal = roundUpToPowerOf2(leftTotal) / 2;
                int lrTotal = leftTotal - llTotal;
                _left->save(array, t - lrTotal, llTotal, lrTotal);
            }
            for (auto c : _changes) {
                DWord tt = t;
                array->append(reinterpret_cast<const Byte*>(&tt), 4);
                array->append(reinterpret_cast<const Byte*>(&c._address), 4);
                DWord count = c._data.count();
                array->append(reinterpret_cast<const Byte*>(&count), 4);
                array->append(&c._data[0], count);
                DWord zero = 0;
                array->append(reinterpret_cast<const Byte*>(&zero),
                    ((~count) + 1) & 3);
            }
            if (_right != 0) {
                int rlTotal = roundUpToPowerOf2(rightTotal) / 2;
                _right->
                    save(array, t + rlTotal, rlTotal, rightTotal - rlTotal);
            }
        }
        void output(int t, int leftTotal, int rightTotal, State* state)
        {
            if (_left != 0) {
                int llTotal = roundUpToPowerOf2(leftTotal) / 2;
                int lrTotal = leftTotal - llTotal;
                _left->output(t - lrTotal, llTotal, lrTotal, state);
            }
            if (t >= state->_t + state->_n) {
                state->runTo(state->_t + state->_n);
                return;
            }
            state->runTo(t);
            for (const auto& c : _changes) {
                c.getData(&state->_data, 0, registerLogCharactersPerBank,
                    state->_addresses, 0);
            }
            if (_right != 0) {
                int rlTotal = roundUpToPowerOf2(rightTotal) / 2;
                _right->output(t + rlTotal, rlTotal, rightTotal - rlTotal,
                    state);
            }
        }
        void ensureAddresses(int startAddress, int endAddress)
        {
            if (_changes.count() == 0)
                _changes.allocate(1);
            int count = endAddress - startAddress;
            if (_changes[0]._data.count() < count) {
                Array<Byte> data(count);
                memcpy(&data[0] + _changes[0]._address - startAddress,
                    &_changes[0]._data[0], _changes[0]._data.count());
                _changes[0]._data = data;
                _changes[0]._address = startAddress;
            }
        }

        Node* _left;
        Array<Change> _changes;
        Node* _right;
    };
    // The root of the tree always has a 0 _left branch.
    Node _root;
    int _total;
    int _pllWidth;
    int _pllHeight;
    int _endAddress;
    Mutex _mutex;
};

class CGAOutput : public ThreadTask
{
public:
    CGAOutput(CGAData* data, CGASequencer* sequencer, BitmapWindow* window)
      : _data(data), _sequencer(sequencer), _window(window), _zoom(0),
        _aspectRatio(1), _inputTL(0, 0), _outputSize(0, 0), _active(false)
    { }
    void run()
    {
        int connector;
        Vector outputSize;
        int combFilter;
        bool showClipping;
        float brightness;
        float contrast;
        Vector2<float> inputTL;
        float overscan;
        double zoom;
        double aspectRatio;
        static const int decoderPadding = 32;
        Vector2<float> zoomVector;
        bool bw;
        {
            Lock lock(&_mutex);
            if (!_active)
                return;

            int total = _data->getTotal();
            _rgbi.ensure(total);
            _data->output(0, total, &_rgbi[0], _sequencer, _phase);

            connector = _connector;
            combFilter = _combFilter;
            showClipping = _showClipping;
            outputSize = _outputSize;
            inputTL = _inputTL;
            overscan = static_cast<float>(_overscan);
            zoom = _zoom;
            aspectRatio = _aspectRatio;
            Byte mode = _data->getDataByte(CGAData::registerMode);
            bw = (mode & 4) != 0;
            _composite.setBW(false);
            bool newCGA = connector == 2;
            _composite.setNewCGA(newCGA);
            _composite.initChroma();
            double black = _composite.black();
            double white = _composite.white();
            _decoder.setHue(_hue + ((mode & 1) != 0 ? 14 : 4) +
                (combFilter == 2 ? 180 : 0));
            _decoder.setSaturation(_saturation*1.45*(newCGA ? 1.5 : 1.0)/100);
            contrast = static_cast<float>(_contrast/100);
            static const int combDivisors[3] = {1, 2, 4};
            int scaling = combDivisors[combFilter];
            _decoder.setInputScaling(scaling);
            double c = _contrast*256*(newCGA ? 1.2 : 1)/((white - black)*100);
            _decoder.setContrast(c/scaling);
            brightness = static_cast<float>(_brightness/100);
            _decoder.setBrightness((-black*c +
                _brightness*5 + (newCGA ? -50 : 0))/256.0);
            _decoder.setChromaBandwidth(_chromaBandwidth);
            _decoder.setLumaBandwidth(_lumaBandwidth);
            _decoder.setRollOff(_rollOff);
            _decoder.setLobes(_lobes);
            _scaler.setProfile(_scanlineProfile);
            _scaler.setHorizontalProfile(_horizontalProfile);
            _scaler.setWidth(static_cast<float>(_scanlineWidth));
            _scaler.setBleeding(_scanlineBleeding);
            _scaler.setHorizontalBleeding(_horizontalBleeding);
            _scaler.setHorizontalRollOff(
                static_cast<float>(_horizontalRollOff));
            _scaler.setVerticalRollOff(static_cast<float>(_verticalRollOff));
            _scaler.setHorizontalLobes(static_cast<float>(_horizontalLobes));
            _scaler.setVerticalLobes(static_cast<float>(_verticalLobes));
            _scaler.setSubPixelSeparation(
                static_cast<float>(_subPixelSeparation));
            _scaler.setPhosphor(_phosphor);
            _scaler.setMask(_mask);
            _scaler.setMaskSize(static_cast<float>(_maskSize));
            zoomVector = scale();
            _scaler.setZoom(zoomVector);
        }

        int srgbSize = _data->getTotal();
        _srgb.ensure(srgbSize*3);
        int pllWidth = _data->getPLLWidth();
        int pllHeight = _data->getPLLHeight();
        static const int driftHorizontal = 8;
        static const int driftVertical = 14*pllWidth;
        _scanlines.clear();
        _fields.clear();
        _fieldOffsets.clear();

        Byte hSync = 0x41;
        Byte vSync = 0x42;
        if (connector != 0) {
            hSync = 0x44;
            vSync = 0x44;
        }
        int lastScanline = 0;
        int i;
        Vector2<float> activeSize(0, 0);
        do {
            int offset = (lastScanline + pllWidth - driftHorizontal) %
                srgbSize;
            Byte* p = &_rgbi[offset];
            int n = driftHorizontal*2;
            if (offset + n <= srgbSize) {
                for (i = 0; i < n; ++i) {
                    if ((p[i] & hSync) == hSync)
                        break;
                }
            }
            else {
                for (i = 0; i < n; ++i) {
                    if ((_rgbi[(offset + i) % srgbSize] & hSync) == hSync)
                        break;
                }
            }
            i = (i + offset) % srgbSize;
            activeSize.x = max(activeSize.x,
                static_cast<float>(wrap(i - lastScanline, srgbSize)));
            if ((_rgbi[i] & 0x80) != 0)
                break;
            _rgbi[i] |= 0x80;
            _scanlines.append(i);
            lastScanline = i;
        } while (true);
        int firstScanline = _scanlines.count() - 1;
        for (; firstScanline > 0; --firstScanline) {
            if (_scanlines[firstScanline] == i)
                break;
        }
        int scanlines = _scanlines.count() - firstScanline;

        for (auto& s : _scanlines)
            _rgbi[s] &= ~0x80;
        int lastField = 0;
        do {
            int offset = (lastField + pllHeight - driftVertical) %
                srgbSize;
            Byte* p = &_rgbi[offset];
            int n = driftVertical*2;
            int j;
            int s = 0;
            if (offset + n <= srgbSize) {
                for (j = 0; j < n; j += 57) {
                    if ((p[j] & vSync) == vSync) {
                        ++s;
                        if (s == 3)
                            break;
                    }
                    else
                        s = 0;
                }
            }
            else {
                for (j = 0; j < n; j += 57) {
                    if ((_rgbi[(offset + j) % srgbSize] & vSync) == vSync) {
                        ++s;
                        if (s == 3)
                            break;
                    }
                    else
                        s = 0;
                }
            }
            j = (j + offset) % srgbSize;
            lastField = j;
            int s0;
            int s1;
            float fieldOffset;
            for (i = 0; i < scanlines; ++i) {
                s0 = _scanlines[
                    (firstScanline + scanlines + i - 1) % scanlines];
                if (s0 < 0)
                    s0 = -1 - s0;
                s1 = _scanlines[firstScanline + i];
                if (s1 < 0)
                    s1 = -1 - s1;
                if (s0 < s1) {
                    if (j >= s0 && j < s1) {
                        fieldOffset = static_cast<float>(j - s0) / (s1 - s0);
                        break;
                    }
                }
                else {
                    if (j >= s0) {
                        fieldOffset =
                            static_cast<float>(j - s0) / (s1 + srgbSize - s0);
                        break;
                    }
                    else {
                        if (j < s1) {
                            fieldOffset = static_cast<float>(j + srgbSize - s0)
                                / (s1 + srgbSize - s0);
                            break;
                        }
                    }
                }
            }
            int fo = static_cast<int>(fieldOffset * 8 + 0.5);
            if (fo == 8) {
                fo = 0;
                i = (i + 1) % scanlines;
            }
            float f = fo/8.0f;
            int c = _fields.count() - 1;
            if (c >= 0) {
                int iLast = _fields[c];
                float fLast = _fieldOffsets[c];
                int lines = (i - iLast + scanlines - 1) % scanlines + 1;
                activeSize.y = max(activeSize.y,
                    static_cast<float>(lines) + f - fLast);
            }
            if (_scanlines[firstScanline + i] < 0)
                break;
            _fields.append(i);
            _fieldOffsets.append(f);
            _scanlines[firstScanline + i] = -1 - _scanlines[firstScanline + i];
        } while (true);
        int firstField = _fields.count() - 1;
        for (; firstField > 0; --firstField) {
            if (_fields[firstField] == i)
                break;
        }
        for (auto& f : _fields)
            _scanlines[firstScanline + f] = -1 - _scanlines[firstScanline + f];

        // Assume standard overscan/blank/sync areas
        activeSize -= Vector2<float>(272, 62);

        if (outputSize.zeroArea()) {
            inputTL = Vector2<float>(160.5f, 38) - overscan*activeSize;
            double o = 1 + 2*overscan;
            double y = zoom*activeSize.y*o;
            double x = zoom*activeSize.x*o*aspectRatio/2;
            outputSize = Vector(static_cast<int>(x + 0.5),
                static_cast<int>(y + 0.5));

            Lock lock(&_mutex);
            _inputTL = inputTL;
            _outputSize = outputSize;
        }
        Vector2<float> offset(0, 0);
        if (connector != 0) {
            offset = Vector2<float>(-decoderPadding - 0.5f, 0);
            if (combFilter == 2)
                offset += Vector2<float>(2, -1);
        }
        _scaler.setOffset(inputTL + offset +
            Vector2<float>(0.5f, 0.5f)/zoomVector);
        _scaler.setOutputSize(outputSize);

        _bitmap.ensure(outputSize);
        _scaler.init();
        _unscaled = _scaler.input();
        _scaled = _scaler.output();
        Vector tl = _scaler.inputTL();
        Vector br = _scaler.inputBR();
        _unscaledSize = br - tl;
        Vector activeTL = Vector(0, 0) - tl;

        if (connector == 0) {
            // Convert from RGBI to 9.7 fixed-point sRGB
            Byte levels[4];
            for (int i = 0; i < 4; ++i) {
                int l = static_cast<int>(255.0f*brightness + 85.0f*i*contrast);
                if (showClipping) {
                    if (l < 0)
                        l = 255;
                    else {
                        if (l > 255)
                            l = 0;
                    }
                }
                else
                    l = clamp(0, l, 255);
                levels[i] = l;
            }
            // On the RGBI connector we don't show composite sync, colour
            // burst, or blanking since these are not output on the actual
            // connector. Blanking is visible as black if the overscan is a
            // non-black colour.
            int palette[3*0x78] = {
                0, 0, 0,  0, 0, 2,  0, 2, 0,  0, 2, 2,
                2, 0, 0,  2, 0, 2,  2, 1, 0,  2, 2, 2,
                1, 1, 1,  1, 1, 3,  1, 3, 1,  1, 3, 3,
                3, 1, 1,  3, 1, 3,  3, 3, 1,  3, 3, 3};
            static int overscanPalette[3*4] = {
                0, 0, 0,  0, 1, 0,  1, 0, 0,  1, 1, 0};
            for (int i = 3*0x10; i < 3*0x78; i += 3*4)
                memcpy(palette + i, overscanPalette, 3*4*sizeof(int));
            Byte srgbPalette[3*0x77];
            for (int i = 0; i < 3*0x77; ++i)
                srgbPalette[i] = levels[palette[i]];
            const Byte* rgbi = &_rgbi[0];
            Byte* srgb = &_srgb[0];
            for (int x = 0; x < srgbSize; ++x) {
                Byte* p = &srgbPalette[3 * *rgbi];
                ++rgbi;
                srgb[0] = p[0];
                srgb[1] = p[1];
                srgb[2] = p[2];
                srgb += 3;
            }
        }
        else {
            // Change to 1 to use FIR decoding for output
#define FIR_DECODING 0
#if FIR_DECODING
            _decoder.setLength(512 - 2*decoderPadding);
#else
            _decoder.setPadding(decoderPadding);
#endif
            Byte burst[4];
            for (int i = 0; i < 4; ++i)
                burst[i] = _composite.simulateCGA(6, 6, (i + 3) & 3);
            _decoder.calculateBurst(burst);
            _composite.setBW(bw);
            _composite.initChroma();
#if FIR_DECODING
#if FIR_FP
            float* input = _decoder.inputData();
#else
            UInt16* input = _decoder.inputData();
#endif
            int inputLeft = _decoder.inputLeft();
            int inputRight = _decoder.inputRight();
#endif

            int combedSize = srgbSize + 2*decoderPadding;
            Vector combTL = Vector(2, 1)*combFilter;
            int ntscSize = combedSize + combTL.y*pllWidth;
            _ntsc.ensure(ntscSize);
            int rgbiSize = ntscSize + 1;
            if (_rgbi.count() < rgbiSize) {
                Array<Byte> rgbi(rgbiSize);
                memcpy(&rgbi[0], &_rgbi[0], srgbSize);
                _rgbi = rgbi;
            }
            memcpy(&_rgbi[srgbSize], &_rgbi[0], rgbiSize - srgbSize);

            // Convert from RGBI to composite
            const Byte* rgbi = &_rgbi[0];
            Byte* ntsc = &_ntsc[0];
            for (int x = 0; x < ntscSize; ++x) {
                *ntsc = _composite.simulateCGA(*rgbi, rgbi[1], x & 3);
                ++rgbi;
                ++ntsc;
            }
            // Apply comb filter and decode to sRGB.
            ntsc = &_ntsc[0];
            Byte* srgb = &_srgb[0];
            static const int fftLength = 512;
            int stride = fftLength - 2*decoderPadding;
            Byte* ntscBlock = &_ntsc[0];
            Timer decodeTimer;

#if FIR_DECODING
            switch (combFilter) {
                case 0:
                    // No comb filter
                    for (int j = 0; j < srgbSize; j += stride) {
                        if (j + stride > srgbSize) {
                            // The last block is a small one, so we'll decode
                            // it by overlapping the previous one.
                            j = srgbSize - stride;
                            ntscBlock = &_ntsc[j];
                            srgb = &_srgb[3*j];
                        }
                        Byte* ip = ntscBlock + decoderPadding + inputLeft;

#if FIR_FP
                        float* p = input;
                        for (int i = inputLeft; i < inputRight; ++i) {
                            p[0] = ip[0];
                            p[1] = ip[0];
                            p += 2;
                            ++ip;
                        }
#else
                        UInt16* p = input;
                        for (int i = inputLeft; i < inputRight; ++i) {
                            p[0] = ip[0] - 128;
                            p[1] = ip[0] - 128;
                            p += 2;
                            ++ip;
                        }
#endif
                        _decoder.decodeBlock(reinterpret_cast<SRGB*>(srgb));
                        srgb += stride*3;
                        ntscBlock += stride;
                    }
                    break;
                case 1:
                    // 1 line. Standard NTSC comb filters will have a delay of
                    // 227.5 color carrier cycles (1 standard scanline) but a
                    // CGA scanline is 228 color carrier cycles, so instead of
                    // sharpening vertical detail a comb filter applied to CGA
                    // will sharpen 1-ldot-per-scanline diagonals.
                    for (int j = 0; j < srgbSize; j += stride) {
                        if (j + stride > srgbSize) {
                            // The last block is a small one, so we'll decode
                            // it by overlapping the previous one.
                            j = srgbSize - stride;
                            ntscBlock = &_ntsc[j];
                            srgb = &_srgb[3*j];
                        }

                        Byte* ip0 = ntscBlock + decoderPadding + inputLeft;
                        Byte* ip1 = ip0 + pllWidth;
#if FIR_FP
                        float* p = input;
                        for (int i = inputLeft; i < inputRight; ++i) {
                            p[0] = static_cast<float>(2*ip0[0]);
                            p[1] = static_cast<float>(ip0[0]-ip1[0]);
                            p += 2;
                            ++ip0;
                            ++ip1;
                        }
#else
                        UInt16* p = input;
                        for (int i = inputLeft; i < inputRight; ++i) {
                            p[0] = 2*ip0[0] - 256;
                            p[1] = ip0[0]-ip1[0];
                            p += 2;
                            ++ip0;
                            ++ip1;
                        }
#endif

                        _decoder.decodeBlock(reinterpret_cast<SRGB*>(srgb));
                        srgb += stride*3;
                        ntscBlock += stride;
                    }
                    break;
                case 2:
                    // 2 line.
                    for (int j = 0; j < srgbSize; j += stride) {
                        if (j + stride > srgbSize) {
                            // The last block is a small one, so we'll decode
                            // it by overlapping the previous one.
                            j = srgbSize - stride;
                            ntscBlock = &_ntsc[j];
                            srgb = &_srgb[3*j];
                        }

                        Byte* ip0 = ntscBlock + decoderPadding + inputLeft;
                        Byte* ip1 = ip0 + pllWidth;
                        Byte* ip2 = ip1 + pllWidth;
#if FIR_FP
                        float* p = input;
                        for (int i = inputLeft; i < inputRight; ++i) {
                            p[0] = static_cast<float>(4*ip1[0]);
                            p[1] = static_cast<float>(2*ip1[0]-ip0[0]-ip2[0]);
                            p += 2;
                            ++ip0;
                            ++ip1;
                            ++ip2;
                        }
#else
                        UInt16* p = input;
                        for (int i = inputLeft; i < inputRight; ++i) {
                            p[0] = 4*ip1[0] - 512;
                            p[1] = 2*ip1[0]-ip0[0]-ip2[0];
                            p += 2;
                            ++ip0;
                            ++ip1;
                            ++ip2;
                        }
#endif

                        _decoder.decodeBlock(reinterpret_cast<SRGB*>(srgb));
                        srgb += stride*3;
                        ntscBlock += stride;
                    }
                    break;
            }
#else
            switch (combFilter) {
                case 0:
                    // No comb filter
                    for (int j = 0; j < srgbSize; j += stride) {
                        if (j + stride > srgbSize) {
                            // The last block is a small one, so we'll decode
                            // it by overlapping the previous one.
                            j = srgbSize - stride;
                            ntscBlock = &_ntsc[j];
                            srgb = &_srgb[3*j];
                        }
                        _decoder.decodeNTSC(ntscBlock,
                            reinterpret_cast<SRGB*>(srgb));
                        srgb += stride*3;
                        ntscBlock += stride;
                    }
                    break;
                case 1:
                    // 1 line. Standard NTSC comb filters will have a delay of
                    // 227.5 color carrier cycles (1 standard scanline) but a
                    // CGA scanline is 228 color carrier cycles, so instead of
                    // sharpening vertical detail a comb filter applied to CGA
                    // will sharpen 1-ldot-per-scanline diagonals.
                    for (int j = 0; j < srgbSize; j += stride) {
                        if (j + stride > srgbSize) {
                            // The last block is a small one, so we'll decode
                            // it by overlapping the previous one.
                            j = srgbSize - stride;
                            ntscBlock = &_ntsc[j];
                            srgb = &_srgb[3*j];
                        }
                        Byte* n0 = ntscBlock;
                        Byte* n1 = n0 + pllWidth;
                        float* y = _decoder.yData();
                        float* i = _decoder.iData();
                        float* q = _decoder.qData();
                        for (int x = 0; x < fftLength; x += 4) {
                            y[0] = static_cast<float>(2*n0[0]);
                            y[1] = static_cast<float>(2*n0[1]);
                            y[2] = static_cast<float>(2*n0[2]);
                            y[3] = static_cast<float>(2*n0[3]);
                            i[0] = -static_cast<float>(n0[1] - n1[1]);
                            i[1] = static_cast<float>(n0[3] - n1[3]);
                            q[0] = static_cast<float>(n0[0] - n1[0]);
                            q[1] = -static_cast<float>(n0[2] - n1[2]);
                            n0 += 4;
                            n1 += 4;
                            y += 4;
                            i += 2;
                            q += 2;
                        }
                        _decoder.decodeBlock(reinterpret_cast<SRGB*>(srgb));
                        srgb += stride*3;
                        ntscBlock += stride;
                    }
                    break;
                case 2:
                    // 2 line.
                    for (int j = 0; j < srgbSize; j += stride) {
                        if (j + stride > srgbSize) {
                            // The last block is a small one, so we'll decode
                            // it by overlapping the previous one.
                            j = srgbSize - stride;
                            ntscBlock = &_ntsc[j];
                            srgb = &_srgb[3*j];
                        }
                        Byte* n0 = ntscBlock;
                        Byte* n1 = n0 + pllWidth;
                        Byte* n2 = n1 + pllWidth;
                        float* y = _decoder.yData();
                        float* i = _decoder.iData();
                        float* q = _decoder.qData();
                        for (int x = 0; x < fftLength; x += 4) {
                            y[0] = static_cast<float>(4*n1[0]);
                            y[1] = static_cast<float>(4*n1[1]);
                            y[2] = static_cast<float>(4*n1[2]);
                            y[3] = static_cast<float>(4*n1[3]);
                            i[0] = static_cast<float>(n0[1] + n2[1] - 2*n1[1]);
                            i[1] = static_cast<float>(2*n1[3] - n0[3] - n2[3]);
                            q[0] = static_cast<float>(2*n1[0] - n0[0] - n2[0]);
                            q[1] = static_cast<float>(n0[2] + n2[2] - 2*n1[2]);
                            n0 += 4;
                            n1 += 4;
                            n2 += 4;
                            y += 4;
                            i += 2;
                            q += 2;
                        }
                        _decoder.decodeBlock(reinterpret_cast<SRGB*>(srgb));
                        srgb += stride*3;
                        ntscBlock += stride;
                    }
                    break;
            }
#endif
            decodeTimer.output("Decoder: ");
        }
        // Shift, clip, show clipping and linearization
        _linearizer.setShowClipping(showClipping && _connector != 0);
        tl.y = wrap(tl.y + _fields[firstField], scanlines);
        Byte* unscaledRow = _unscaled.data();
        int scanlineChannels = _unscaledSize.x*3;
        for (int y = 0; y < _unscaledSize.y; ++y) {
            int offsetTL = wrap(
                tl.x + _scanlines[(tl.y + y)%scanlines + firstScanline],
                srgbSize);
            const Byte* srgbRow = &_srgb[offsetTL*3];
            float* unscaled = reinterpret_cast<float*>(unscaledRow);
            const Byte* srgb = srgbRow;
            if (offsetTL + _unscaledSize.x > srgbSize) {
                int endChannels = max(0, (srgbSize - offsetTL)*3);
                for (int x = 0; x < endChannels; ++x)
                    unscaled[x] = _linearizer.linear(srgb[x]);
                for (int x = 0; x < scanlineChannels - endChannels; ++x)
                    unscaled[x + endChannels] = _linearizer.linear(_srgb[x]);
            }
            else {
                for (int x = 0; x < scanlineChannels; ++x)
                    unscaled[x] = _linearizer.linear(srgb[x]);
            }
            unscaledRow += _unscaled.stride();
        }

        // Scale to desired size and apply scanline filter
        _scaler.render();

        // Delinearization and float-to-byte conversion
        const Byte* scaledRow = _scaled.data();
        Byte* outputRow = _bitmap.data();
        for (int y = 0; y < outputSize.y; ++y) {
            const float* scaled = reinterpret_cast<const float*>(scaledRow);
            DWORD* output = reinterpret_cast<DWORD*>(outputRow);
            for (int x = 0; x < outputSize.x; ++x) {
                SRGB srgb =
                    _linearizer.srgb(Colour(scaled[0], scaled[1], scaled[2]));
                *output = (srgb.x << 16) | (srgb.y << 8) | srgb.z;
                ++output;
                scaled += 3;
            }
            scaledRow += _scaled.stride();
            outputRow += _bitmap.stride();
        }
        _lastBitmap = _bitmap;
        _bitmap = _window->setNextBitmap(_bitmap);
    }

    void save(String outputFileName)
    {
        setOutputSize(Vector(0, 0));
        join();
        _lastBitmap.save(PNGFileFormat<DWORD>(),
            File(outputFileName + ".png", true));

        if (_connector != 0) {
            FileStream s = File(outputFileName + ".ntsc", true).openWrite();
            s.write(_ntsc);
        }
    }

    void setConnector(int connector)
    {
        {
            Lock lock(&_mutex);
            _connector = connector;
        }
        restart();
    }
    int getConnector() { return _connector; }
    void setScanlineProfile(int profile)
    {
        {
            Lock lock(&_mutex);
            _scanlineProfile = profile;
        }
        restart();
    }
    int getScanlineProfile() { return _scanlineProfile; }
    void setHorizontalProfile(int profile)
    {
        {
            Lock lock(&_mutex);
            _horizontalProfile = profile;
        }
        restart();
    }
    int getHorizontalProfile() { return _horizontalProfile; }
    void setScanlineWidth(double width)
    {
        {
            Lock lock(&_mutex);
            _scanlineWidth = width;
        }
        restart();
    }
    double getScanlineWidth() { return _scanlineWidth; }
    void setScanlineBleeding(int bleeding)
    {
        {
            Lock lock(&_mutex);
            _scanlineBleeding = bleeding;
        }
        restart();
    }
    int getScanlineBleeding() { return _scanlineBleeding; }
    void setHorizontalBleeding(int bleeding)
    {
        {
            Lock lock(&_mutex);
            _horizontalBleeding = bleeding;
        }
        restart();
    }
    int getHorizontalBleeding() { return _horizontalBleeding; }
    void setZoom(double zoom)
    {
        if (zoom == 0)
            zoom = 1.0;
        {
            Lock lock(&_mutex);
            if (_window->hWnd() != 0) {
                Vector mousePosition = _window->mousePosition();
                Vector size = _outputSize;
                Vector2<float> position = Vector2Cast<float>(size)/2.0f;
                if (_dragging || (mousePosition.inside(size) &&
                    (GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0 &&
                    (GetAsyncKeyState(VK_RBUTTON) & 0x8000) == 0))
                    position = Vector2Cast<float>(mousePosition);
                _inputTL += position*static_cast<float>(zoom - _zoom)/(
                    Vector2<float>(static_cast<float>(_aspectRatio)/2.0f, 1.0f)
                    *static_cast<float>(_zoom*zoom));
            }
            _zoom = zoom;
        }
        restart();
    }
    double getZoom() { return _zoom; }
    void setHorizontalRollOff(double rollOff)
    {
        {
            Lock lock(&_mutex);
            _horizontalRollOff = rollOff;
        }
        restart();
    }
    double getHorizontalRollOff() { return _horizontalRollOff; }
    void setHorizontalLobes(double lobes)
    {
        {
            Lock lock(&_mutex);
            _horizontalLobes = lobes;
        }
        restart();
    }
    double getHorizontalLobes() { return _horizontalLobes; }
    void setVerticalRollOff(double rollOff)
    {
        {
            Lock lock(&_mutex);
            _verticalRollOff = rollOff;
        }
        restart();
    }
    double getVerticalRollOff() { return _verticalRollOff; }
    void setVerticalLobes(double lobes)
    {
        {
            Lock lock(&_mutex);
            _verticalLobes = lobes;
        }
        restart();
    }
    double getVerticalLobes() { return _verticalLobes; }
    void setSubPixelSeparation(double separation)
    {
        {
            Lock lock(&_mutex);
            _subPixelSeparation = separation;
        }
        restart();
    }
    double getSubPixelSeparation() { return _subPixelSeparation; }
    void setPhosphor(int phosphor)
    {
        {
            Lock lock(&_mutex);
            _phosphor = phosphor;
        }
        restart();
    }
    int getPhosphor() { return _phosphor; }
    void setMask(int mask)
    {
        {
            Lock lock(&_mutex);
            _mask = mask;
        }
        restart();
    }
    int getMask() { return _mask; }
    void setMaskSize(double size)
    {
        {
            Lock lock(&_mutex);
            _maskSize = size;
        }
        restart();
    }
    double getMaskSize() { return _maskSize; }
    void setAspectRatio(double ratio)
    {
        if (ratio == 0)
            ratio = 1.0;
        {
            Lock lock(&_mutex);
            if (_window->hWnd() != 0) {
                Vector mousePosition = _window->mousePosition();
                Vector size = _outputSize;
                Vector2<float> position = Vector2Cast<float>(size)/2.0f;
                if (_dragging || (mousePosition.inside(size) &&
                    (GetAsyncKeyState(VK_LBUTTON) & 0x8000) == 0 &&
                    (GetAsyncKeyState(VK_RBUTTON) & 0x8000) == 0))
                    position = Vector2Cast<float>(mousePosition);
                _inputTL.x += position.x*2.0f*static_cast<float>(
                    (ratio - _aspectRatio)/(_zoom*ratio*_aspectRatio));
            }
            _aspectRatio = ratio;
        }
        restart();
    }
    double getAspectRatio() { return _aspectRatio; }
    void setOverscan(double overscan)
    {
        {
            Lock lock(&_mutex);
            _overscan = overscan;
        }
        restart();
    }
    void setOutputSize(Vector outputSize)
    {
        {
            Lock lock(&_mutex);
            _outputSize = outputSize;
            _active = true;
        }
        restart();
    }
    void setCombFilter(int combFilter)
    {
        {
            Lock lock(&_mutex);
            _combFilter = combFilter;
        }
        restart();
    }
    int getCombFilter() { return _combFilter; }

    void setHue(double hue)
    {
        {
            Lock lock(&_mutex);
            _hue = hue;
        }
        restart();
    }
    double getHue() { return _hue; }
    void setSaturation(double saturation)
    {
        {
            Lock lock(&_mutex);
            _saturation = saturation;
        }
        restart();
    }
    double getSaturation() { return _saturation; }
    void setContrast(double contrast)
    {
        {
            Lock lock(&_mutex);
            _contrast = contrast;
        }
        restart();
    }
    double getContrast() { return _contrast; }
    void setBrightness(double brightness)
    {
        {
            Lock lock(&_mutex);
            _brightness = brightness;
        }
        restart();
    }
    double getBrightness() { return _brightness; }
    void setShowClipping(bool showClipping)
    {
        {
            Lock lock(&_mutex);
            _showClipping = showClipping;
        }
        restart();
    }
    bool getShowClipping() { return _showClipping; }
    void setChromaBandwidth(double chromaBandwidth)
    {
        {
            Lock lock(&_mutex);
            _chromaBandwidth = chromaBandwidth;
        }
        restart();
    }
    double getChromaBandwidth() { return _chromaBandwidth; }
    void setLumaBandwidth(double lumaBandwidth)
    {
        {
            Lock lock(&_mutex);
            _lumaBandwidth = lumaBandwidth;
        }
        restart();
    }
    double getLumaBandwidth() { return _lumaBandwidth; }
    void setRollOff(double rollOff)
    {
        {
            Lock lock(&_mutex);
            _rollOff = rollOff;
        }
        restart();
    }
    double getRollOff() { return _rollOff; }
    void setLobes(double lobes)
    {
        {
            Lock lock(&_mutex);
            _lobes = lobes;
        }
        restart();
    }
    double getLobes() { return _lobes; }
    void setPhase(int phase)
    {
        {
            Lock lock(&_mutex);
            _phase = phase;
        }
        restart();
    }

    Vector requiredSize()
    {
        {
            Lock lock(&_mutex);
            if (!_outputSize.zeroArea())
                return _outputSize;
            _active = true;
        }
        restart();
        join();
        {
            Lock lock(&_mutex);
            return _outputSize;
        }
    }
    void mouseInput(Vector position, bool button)
    {
        _mousePosition = position;
        if (button) {
            if (!_dragging) {
                _dragStart = position;
                _dragStartInputPosition = _inputTL +
                    Vector2Cast<float>(position)/scale();
            }
            _inputTL =
                _dragStartInputPosition - Vector2Cast<float>(position)/scale();
            restart();
        }
        _dragging = button;
    }
    void saveRGBI(File outputFile)
    {
        outputFile.openWrite().write(static_cast<const void*>(&_rgbi[0]),
            _data->getTotal());
    }

private:
    // Output pixels per input pixel
    Vector2<float> scale()
    {
        return Vector2<float>(static_cast<float>(_aspectRatio)/2.0f, 1.0f)*
            static_cast<float>(_zoom);
    }

    CGAData* _data;
    CGASequencer* _sequencer;
    AppendableArray<int> _scanlines;    // hdot positions of scanline starts
    AppendableArray<int> _fields;       // scanline numbers of field starts
    AppendableArray<float> _fieldOffsets; // fractional scanline numbers

    int _connector;
    int _phase;
    int _scanlineProfile;
    int _horizontalProfile;
    double _scanlineWidth;
    int _scanlineBleeding;
    int _horizontalBleeding;
    double _horizontalRollOff;
    double _verticalRollOff;
    double _horizontalLobes;
    double _verticalLobes;
    double _subPixelSeparation;
    int _phosphor;
    int _mask;
    double _maskSize;
    double _zoom;
    double _aspectRatio;
    double _overscan;
    Vector _outputSize;
    Vector2<float> _inputTL;  // input position of top-left of output
    double _hue;
    double _saturation;
    double _contrast;
    double _brightness;
    double _chromaBandwidth;
    double _lumaBandwidth;
    double _rollOff;
    double _lobes;
    int _combFilter;
    bool _showClipping;
    bool _active;

    Bitmap<DWORD> _bitmap;
    Bitmap<DWORD> _lastBitmap;
    CGAComposite _composite;
#if FIR_DECODING
    MatchingNTSCDecoder _decoder;
#else
    NTSCDecoder _decoder;
#endif
    Linearizer _linearizer;
    BitmapWindow* _window;
    Mutex _mutex;

    ScanlineRenderer _scaler;
    Vector _combedSize;
    Array<Byte> _rgbi;
    Array<Byte> _ntsc;
    Array<Byte> _srgb;
    Vector _unscaledSize;
    AlignedBuffer _unscaled;
    AlignedBuffer _scaled;

    bool _dragging;
    Vector _dragStart;
    Vector2<float> _dragStartInputPosition;
    Vector _mousePosition;
};

#endif // INCLUDED_CGA_H
