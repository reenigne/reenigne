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
static const UInt16 pointCount = 16193;
static const UInt16 particleCount = 948;
static const UInt16 particleSize = 15;
static const UInt16 headerSize = 15;
static const UInt16 footerSize = 5;

static const UInt8 particleCode[] = {
    0xbf, 0x00, 0x00,        // mov di,0000
    0xaa,                    // stosb
    0xd1, 0xe7,              // shl di,1
    0x8b, 0x3d,              // mov di,[di]
    0x26, 0x88, 0x25,        // es: mov [di],ah
    0x89, 0x3e, 0x00, 0x00,  // mov [0000],di
};

static const UInt8 headerCode[] = {
//    0xcc,                    // int 3
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

// These arrays form a set of doubly-linked lists. At each location in the
// array is the point number of the next/previous point in the chain.
UInt16 far* pointsNext;
UInt16 far* pointsPrev;

// The following are either:
//   0xffff: The corresponding list is empty.
//   anything else: A point in the list.
UInt16 freeList;  // List of points which haven't been placed in their final locations yet.
UInt16 usedList;  // List of points which are finalized.

// Moves a point to its own list.
void remove(UInt16 point)
{
    UInt16 oldPrev = pointsPrev[point];
    UInt16 oldNext = pointsNext[point];
    pointsNext[oldPrev] = oldNext;
    pointsPrev[oldNext] = oldPrev;
    pointsNext[point] = point;
    pointsPrev[point] = point;
}

// Moves a point to (the end of) list.
void place(UInt16* list, UInt16 point)
{
    if (*list == 0xffff) {
        remove(point);
        *list = point;
    }
    else {
        remove(point);


        pointsNext[point] = *list;
    }
}

bool isVisible(UInt16 point)
{
    return (point & 1fff) < 80*100;
}

void shuffle()
{
    UInt16 visible = 0;
    UInt16 p;
    // Fill pointsPrev with the indexes of visible points
    for (p = 0; p < pointCount; ++p)
        if (isVisible(p)) {
            pointsPrev[visible] = p;
            ++visible;
        }
    // Shuffle pointsPrev
    for (p = 0; p < visible - 1; ++p) {
        UInt16 q = rand() % (visible - p) + p;
        if (q != p) {
            UInt16 t = pointsPrev[p];
            pointsPrev[p] = pointsPrev[q];
            pointsPrev[q] = t;
        }
    }
    // Make a singly linked list from the index array.
    freeList = pointsPrev[0];
    for (p = 0; p < visible - 1; ++p)
        pointsNext[pointsPrev[p]] = pointsPrev[p + 1];
    pointsNext[pointsPrev[visible - 1]] = freeList;
    // Make the list doubly linked
    for (p = 0; p <
}

int main()
{
    UInt8 far* program = (UInt8 far*)(_fmalloc(pointCount*2 +
        particleCount*particleSize + headerSize + footerSize));
    UInt32 address = ((UInt32)(FP_SEG(program))) << 4;
    address += (UInt32)(FP_OFF(program));
    UInt16 segment = (UInt16)((address + 0xf) >> 4);
    UInt16 far* points = (UInt16 far*)MK_FP(segment, 0);
    pointsPrev = (UInt16 far*)(_fmalloc(pointCount*2));

    _fmemset(points, 0, pointCount*2);

    // Initialize the code block
    UInt8 far* particles = (UInt8 far*)MK_FP(segment, pointCount*2);
    _fmemcpy(particles, headerCode, headerSize);
    particles += headerSize;
    for (UInt16 particle = 0; particle < particleCount; ++particle) {
        // Copy the code
        _fmemcpy(particles, particleCode, particleSize);

        // Find an unused random point. TODO: Don't use gap points.
        UInt16 r;
        do {
            r = rand() % (pointCount - 1);
        } while (points[r] != 0);
        points[r] = 1;

        // Fill in the operands
        UInt8 far* p = &particles[1];
        *(UInt16 far*)(p) = r;
        *(UInt16 far*)(&particles[13]) = FP_OFF(p);

        particles += particleSize;
    }
    _fmemcpy(particles, footerCode, footerSize);
    pointsNext = points + 1;

    // Initialize the points with a pattern. TODO: Make a more interesting pattern
    for (UInt16 point = 0; point < pointCount - 1; ++point) {
        UInt16 next = (point + 1) % pointCount;
        pointsNext[point] = next;
        pointsPrev[next] = point;
    }
    freeAll();

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
