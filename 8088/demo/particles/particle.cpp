#include <malloc.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include <memory.h>

typedef unsigned long int  UInt32;
typedef signed long int    SInt32;
typedef unsigned short int UInt16;
typedef signed short int   SInt16;
typedef unsigned char      UInt8;
typedef signed char        SInt8;

static const UInt16 width = 80;
static const UInt16 height = 200;
static const UInt16 pointCount = 16192;
static const UInt16 particleCount = 989;
static const UInt16 particleSize = 13;
static const UInt16 headerSize = 15;
static const UInt16 footerSize = 5;

static const UInt8 particleCode[] = {
    0xbf, 0x00, 0x00,        // mov di,0000
    0xaa,                    // stosb
    0x8b, 0x1d,              // mov bx,[di]
    0x26, 0x88, 0x27,        // es: mov [bx],ah
    0x89, 0x1e, 0x00, 0x00,  // mov [0000],bx
};

static const UInt8 headerCode[] = {
    0x06,                    // push es
    0x1e,                    // push ds
    0x57,                    // push di
    0x9c,                    // pushf
    0xfc,                    // cld
    0xb8, 0x00, 0xb8,        // mov ax,0b800
    0x8e, 0xc0,              // mov es,ax
    0x0e,                    // push cs
    0x1f,                    // pop ds
    0xb8, 0x00, 0xff         // mov ax,0ff00
};

static const UInt8 footerCode[] = {
    0x9d,                    // popf
    0x5f,                    // pop di
    0x1f,                    // pop ds
    0x07,                    // pop es
    0xcb                     // retf
};


int main()
{
    UInt8 huge* program = (UInt8 huge*)(halloc(
        pointCount*2 + particleCount*particleSize + 9, 1));
    UInt32 address = ((UInt32)(FP_SEG(program))) << 4;
    address += (UInt32)(FP_OFF(program));
    UInt16 segment = (UInt16)((address + 0xf) >> 4);
    UInt16 far* points = (UInt16 far*)MK_FP(segment, 0);

    UInt16 point;
    for (point = 0; point < pointCount; ++point)
        points[point] = 0;

    // Initialize the code block
    UInt8 far* particles = (UInt8 far*)MK_FP(segment, pointCount*2);
    _fmemcpy(particles, headerCode, headerSize);
    for (UInt16 particle = 0; particle < particleCount; ++particle) {
        // Copy the code
        UInt8 far* p = particles + particle*particleSize + headerSize;
        _fmemcpy(p, particleCode, particleSize);

        // Find an unused random point
        UInt16 r = particle;
        do {
            r = rand() % pointCount;
        } while (points[r] != 0);
        points[r] = 1;

        // Fill in the operands
        *(UInt16 far*)(&p[1]) = r;
        *(UInt16 far*)(&p[11]) = pointCount*2 + particle*particleSize + headerSize + 1;
    }
    _fmemcpy(particles + particleCount*particleSize + headerSize, footerCode, footerSize);

    // Initialize the points with a pattern. TODO: Make a more interesting pattern
    for (point = 0; point < pointCount; ++point)
        points[point] = (point + 19) % pointCount;

    // Set screen mode to 4 (CGA 320x200 2bpp)
    union REGS regs;
    regs.x.ax = 0x04;
    int86(0x10, &regs, &regs);

    // Call code repeatedly. TODO: jmp instead of retf in the code and use interrupt for keyboard
    void (far* code)() = (void (far*)())MK_FP(segment, pointCount*2);
    int k = 0;
    do {
        code();
        if (kbhit())
            k = getch();
    } while (k != 27);
    regs.x.ax = 0x03;
    int86(0x10, &regs, &regs);

    hfree(program);
    return 0;
}
