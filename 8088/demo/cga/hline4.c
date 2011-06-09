typedef unsigned long int  UInt32;
typedef signed long int    SInt32;
typedef unsigned short int UInt16;
typedef signed short int   SInt16;
typedef unsigned char      UInt8;
typedef signed char        SInt8;
typedef int                Bool;

void hLine(UInt16 xStart, UInt16 xEnd, UInt16 y, UInt8 c)
{
    static UInt8 leftData[8] = {0x00, 0xc0, 0xf0, 0xfc};
    static UInt8 rightData[8] = {0x3f, 0x0f, 0x03, 0x00};
    static UInt16 yTable[200] = {
      0x0000, 0x2000, 0x0050, 0x2050, 0x00a0, 0x20a0, 0x00f0, 0x20f0,
      0x0140, 0x2140, 0x0190, 0x2190, 0x01e0, 0x21e0, 0x0230, 0x2230,
      0x0280, 0x2280, 0x02d0, 0x22d0, 0x0320, 0x2320, 0x0370, 0x2370,
      0x03c0, 0x23c0, 0x0410, 0x2410, 0x0460, 0x2460, 0x04b0, 0x24b0,
      0x0500, 0x2500, 0x0550, 0x2550, 0x05a0, 0x25a0, 0x05f0, 0x25f0,
      0x0640, 0x2640, 0x0690, 0x2690, 0x06e0, 0x26e0, 0x0730, 0x2730,
      0x0780, 0x2780, 0x07d0, 0x27d0, 0x0820, 0x2820, 0x0870, 0x2870,
      0x08c0, 0x28c0, 0x0910, 0x2910, 0x0960, 0x2960, 0x09b0, 0x29b0,
      0x0a00, 0x2a00, 0x0a50, 0x2a50, 0x0aa0, 0x2aa0, 0x0af0, 0x2af0,
      0x0b40, 0x2b40, 0x0b90, 0x2b90, 0x0be0, 0x2be0, 0x0c30, 0x2c30,
      0x0c80, 0x2c80, 0x0cd0, 0x2cd0, 0x0d20, 0x2d20, 0x0d70, 0x2d70,
      0x0dc0, 0x2dc0, 0x0e10, 0x2e10, 0x0e60, 0x2e60, 0x0eb0, 0x2eb0,
      0x0f00, 0x2f00, 0x0f50, 0x2f50, 0x0fa0, 0x2fa0, 0x0ff0, 0x2ff0,
      0x1040, 0x3040, 0x1090, 0x3090, 0x10e0, 0x30e0, 0x1130, 0x3130,
      0x1180, 0x3180, 0x11d0, 0x31d0, 0x1220, 0x3220, 0x1270, 0x3270,
      0x12c0, 0x32c0, 0x1310, 0x3310, 0x1360, 0x3360, 0x13b0, 0x33b0,
      0x1400, 0x3400, 0x1450, 0x3450, 0x14a0, 0x34a0, 0x14f0, 0x34f0,
      0x1540, 0x3540, 0x1590, 0x3590, 0x15e0, 0x35e0, 0x1630, 0x3630,
      0x1680, 0x3680, 0x16d0, 0x36d0, 0x1720, 0x3720, 0x1770, 0x3770,
      0x17c0, 0x37c0, 0x1810, 0x3810, 0x1860, 0x3860, 0x18b0, 0x38b0,
      0x1900, 0x3900, 0x1950, 0x3950, 0x19a0, 0x39a0, 0x19f0, 0x39f0,
      0x1a40, 0x3a40, 0x1a90, 0x3a90, 0x1ae0, 0x3ae0, 0x1b30, 0x3b30,
      0x1b80, 0x3b80, 0x1bd0, 0x3bd0, 0x1c20, 0x3c20, 0x1c70, 0x3c70,
      0x1cc0, 0x3cc0, 0x1d10, 0x3d10, 0x1d60, 0x3d60, 0x1db0, 0x3db0,
      0x1e00, 0x3e00, 0x1e50, 0x3e50, 0x1ea0, 0x3ea0, 0x1ef0, 0x3ef0};

    y = yTable[y];
    UInt8 leftBits = leftData[xStart & 3];
    UInt8 rightBits = rightData[xEnd & 3];
    UInt16 p = (xStart >> 3) + y;
    UInt8 far* s = (UInt8 far*)MK_FP(0xb800, p);
    UInt16 endP = (xEnd >> 3) + y;
    UInt8 byte = *s;
    if (p == endP)
        rightBits |= leftBits;
    else {
        *(s++) = (byte & leftBits) | (c & ~leftBits);
        UInt16 bytes = endP - FP_OFF(s);
        while (bytes > 0) {
            *(s++) = c;
            --bytes;
        }
        byte = *s;
    }
    *s = (byte & rightBits) | (c & ~rightBits);
}

void xorHLine(UInt16 xStart, UInt16 xEnd, UInt16 y, UInt8 c)
{
    static UInt8 leftData[8] = {0x00, 0xc0, 0xf0, 0xfc};
    static UInt8 rightData[8] = {0x3f, 0x0f, 0x03, 0x00};
    static UInt16 yTable[200] = {
      0x0000, 0x2000, 0x0050, 0x2050, 0x00a0, 0x20a0, 0x00f0, 0x20f0,
      0x0140, 0x2140, 0x0190, 0x2190, 0x01e0, 0x21e0, 0x0230, 0x2230,
      0x0280, 0x2280, 0x02d0, 0x22d0, 0x0320, 0x2320, 0x0370, 0x2370,
      0x03c0, 0x23c0, 0x0410, 0x2410, 0x0460, 0x2460, 0x04b0, 0x24b0,
      0x0500, 0x2500, 0x0550, 0x2550, 0x05a0, 0x25a0, 0x05f0, 0x25f0,
      0x0640, 0x2640, 0x0690, 0x2690, 0x06e0, 0x26e0, 0x0730, 0x2730,
      0x0780, 0x2780, 0x07d0, 0x27d0, 0x0820, 0x2820, 0x0870, 0x2870,
      0x08c0, 0x28c0, 0x0910, 0x2910, 0x0960, 0x2960, 0x09b0, 0x29b0,
      0x0a00, 0x2a00, 0x0a50, 0x2a50, 0x0aa0, 0x2aa0, 0x0af0, 0x2af0,
      0x0b40, 0x2b40, 0x0b90, 0x2b90, 0x0be0, 0x2be0, 0x0c30, 0x2c30,
      0x0c80, 0x2c80, 0x0cd0, 0x2cd0, 0x0d20, 0x2d20, 0x0d70, 0x2d70,
      0x0dc0, 0x2dc0, 0x0e10, 0x2e10, 0x0e60, 0x2e60, 0x0eb0, 0x2eb0,
      0x0f00, 0x2f00, 0x0f50, 0x2f50, 0x0fa0, 0x2fa0, 0x0ff0, 0x2ff0,
      0x1040, 0x3040, 0x1090, 0x3090, 0x10e0, 0x30e0, 0x1130, 0x3130,
      0x1180, 0x3180, 0x11d0, 0x31d0, 0x1220, 0x3220, 0x1270, 0x3270,
      0x12c0, 0x32c0, 0x1310, 0x3310, 0x1360, 0x3360, 0x13b0, 0x33b0,
      0x1400, 0x3400, 0x1450, 0x3450, 0x14a0, 0x34a0, 0x14f0, 0x34f0,
      0x1540, 0x3540, 0x1590, 0x3590, 0x15e0, 0x35e0, 0x1630, 0x3630,
      0x1680, 0x3680, 0x16d0, 0x36d0, 0x1720, 0x3720, 0x1770, 0x3770,
      0x17c0, 0x37c0, 0x1810, 0x3810, 0x1860, 0x3860, 0x18b0, 0x38b0,
      0x1900, 0x3900, 0x1950, 0x3950, 0x19a0, 0x39a0, 0x19f0, 0x39f0,
      0x1a40, 0x3a40, 0x1a90, 0x3a90, 0x1ae0, 0x3ae0, 0x1b30, 0x3b30,
      0x1b80, 0x3b80, 0x1bd0, 0x3bd0, 0x1c20, 0x3c20, 0x1c70, 0x3c70,
      0x1cc0, 0x3cc0, 0x1d10, 0x3d10, 0x1d60, 0x3d60, 0x1db0, 0x3db0,
      0x1e00, 0x3e00, 0x1e50, 0x3e50, 0x1ea0, 0x3ea0, 0x1ef0, 0x3ef0};

    y = yTable[y];
    UInt8 leftBits = leftData[xStart & 3];
    UInt8 rightBits = rightData[xEnd & 3];
    UInt16 p = (xStart >> 3) + y;
    UInt8 far* s = (UInt8 far*)MK_FP(0xb800, p);
    UInt16 endP = (xEnd >> 3) + y;
    if (p == endP)
        rightBits &= leftBits;
    else {
        *(s++) ^= c & ~leftBits;
        UInt16 bytes = endP - FP_OFF(s);
        while (bytes > 0) {
            *(s++) ^= c;
            --bytes;
        }
    }
    *s = c & ~rightBits;
}


