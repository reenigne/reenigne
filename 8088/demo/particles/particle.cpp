#include <malloc.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include <memory.h>
#include <stdio.h>

typedef unsigned long int  UInt32;
typedef signed long int    SInt32;
typedef unsigned short int UInt16;
typedef signed short int   SInt16;
typedef unsigned char      UInt8;
typedef signed char        SInt8;
typedef int                Bool;

static const UInt16 width = 80;
static const UInt16 height = 200;
static const UInt16 pointCount = 16192;
static const UInt16 particleCount = 948;
static const UInt16 particleSize = 15;
static const UInt16 headerSize = 24;
static const UInt16 footerSize = 5;
static const UInt16 patterns = 21;

static const Bool true = 1;
static const Bool false = 0;

static const UInt8 particleCode[] = {
    0xbf, 0x00, 0x00,        // mov di,0000          3 0 12
    0xaa,                    // stosb                1 1  8 +~6
    0xd1, 0xe7,              // shl di,1             2 0  8
    0x8b, 0x3d,              // mov di,[di]          2 2 16
    0x26, 0x88, 0x25,        // es: mov [di],ah      3 1 16 +~6
    0x89, 0x3e, 0x00, 0x00,  // mov [0000],di        4 2 24           ~96
};

// mov di,0000        ; old particle position                      3 0 12
// stosb                                                           1 1  8 +6
// mov di,[0000]      ; particle data pointer                      4 2 24
// cs: inc b[0000]    ; address of offset in previous instruction  4 4 32
// es: mov [di],ah                                                 3 1 16 +6
// mov [0000],di      ; address of offset in first instruction     4 2 24      ~128



// mov di,0000        ; old particle position                   3 0 12
// stosb                                                        1 1  8 +6
// mov di,[si+0000]   ; table offset                            4 2 24
// es: mov [di],ah                                              3 1 16 +6
// mov [0000],di      ; address of offset in first instruction  4 2 24          ~96
//
// 1024 particles, 64 frames per track, 32768 positions, 2 particles per track



// mov di,[bp+0]      3 2 20
// lodsw              1 2 12
// es: and [di],al    3 2 20 +12
// mov di,[bp+4]      3 2 20
// es: or [di],ah     3 2 20 +12        ~124
//
// One BP value covers 256 bytes or 128 positions or 4 particles
//  add bp,cx          2 0  8           ~126

// lodsw              1 2 12
// mov di,ax          2 0  8
// lodsw              1 2 12
// es: and [di],al    3 2 20 +12
// mov di,[si]        2 2 16
// es: or [di],ah     3 2 20 +12
// add si,0000        4 0 16

// 2 bytes: Position
// 1 byte: Erase mask for previous position
// 1 byte: Draw bits for next position


// Put draw/erase bits in DS, positions in SS
//  pop di            1 2 12
//  lodsw             1 2 12
//  es: and [di],al   3 2 20 +12
//  pop di            1 2 12
//  es: or [di],ah    3 2 20 +12

// Everything in CS:
//  and b[0000],00    5 2 28 +12
//  or b[0000],00     5 2 28 +12     ~80 cycles
// 10 bytes per position = 320K for 32768 positions
// 10K per frame, repeats 2x per second!

// To avoid a short repeat, we need to distribute the stars randomly throughout the positions array
//  Need a random position in the code for each particle
//  Add this to BP which increases each frame
//  Then use the result to lookup

// lea si,[bp+0000]   4 0 16       ; particle offset
// lodsw              1 2 12
// mov di,[si]        2 2 16
// es: and [di],al    3 2 20 +12
// mov di,[si+2]      3 2 20
// es: or [di],ah     3 2 20 +12    ~128 <== 622 particles

// lea sp,[bp+0000]   4 0 16       ; particle offset
// pop ax             1 2 12
// pop di             1 2 12
// and [di],al        2 2 16 +12
// pop di             1 2 12
// or [di],ah         2 2 16 +12    ~108 <== 737 particles

// At the end of the frame we could also do a reverse lookup and update the positions for some particles which have just left the screen?

// Without proper draw/erase:

// lea sp,[bp+0000]   4 0 16       ; particle offset
// pop di             1 2 12
// stosb              1 1  8 +6
// pop di             1 2 12
// mov [di],ah        2 1 12 +6     ~72 <==  1106 particles



// In current implementation, coordinate is multiplied by 17/16 each frame, which gives about 46 frames maximum (usually much less, since most particles don't appear at the center)
// On average, particle lifetime is 16384/948 = 17 frames!





// Should we

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
//    0xb8, 0x00, 0xff,        // mov ax,0ff00
//    0xb9, 0x55, 0xaa,        // mov cx,0aa55
//    0xba, 0x11, 0x22,        // mov dx,02211
//    0xbb, 0xdd, 0xee         // mov bx,0eedd
   0xb8, 0x00, 0xc0,
   0xb9, 0x30, 0x0c,
   0xba, 0x10, 0x04,
   0xbb, 0x01, 0x02
};

static const UInt8 footerCode[] = {
    0x9d,                    // popf
    0x5f,                    // pop di
    0x1f,                    // pop ds
    0x07,                    // pop es
    0xcb                     // retf
};

static SInt16 sinTableX[] = {
 39,  39,  39,  40,  40,  40,  40,  41,  41,  41,  41,  42,  42,  42,  42,  43,
 43,  43,  43,  44,  44,  44,  44,  45,  45,  45,  45,  46,  46,  46,  46,  47,
 47,  47,  47,  48,  48,  48,  48,  48,  49,  49,  49,  49,  50,  50,  50,  50,
 51,  51,  51,  51,  52,  52,  52,  52,  52,  53,  53,  53,  53,  54,  54,  54,
 54,  55,  55,  55,  55,  55,  56,  56,  56,  56,  57,  57,  57,  57,  57,  58,
 58,  58,  58,  59,  59,  59,  59,  59,  60,  60,  60,  60,  60,  61,  61,  61,
 61,  61,  62,  62,  62,  62,  62,  63,  63,  63,  63,  63,  64,  64,  64,  64,
 64,  65,  65,  65,  65,  65,  65,  66,  66,  66,  66,  66,  67,  67,  67,  67,
 67,  67,  68,  68,  68,  68,  68,  68,  69,  69,  69,  69,  69,  69,  70,  70,
 70,  70,  70,  70,  71,  71,  71,  71,  71,  71,  71,  72,  72,  72,  72,  72,
 72,  72,  73,  73,  73,  73,  73,  73,  73,  73,  74,  74,  74,  74,  74,  74,
 74,  74,  75,  75,  75,  75,  75,  75,  75,  75,  75,  75,  76,  76,  76,  76,
 76,  76,  76,  76,  76,  76,  76,  77,  77,  77,  77,  77,  77,  77,  77,  77,
 77,  77,  77,  77,  78,  78,  78,  78,  78,  78,  78,  78,  78,  78,  78,  78,
 78,  78,  78,  78,  78,  78,  78,  79,  79,  79,  79,  79,  79,  79,  79,  79,
 79,  79,  79,  79,  79,  79,  79,  79,  79,  79,  79,  79,  79,  79,  79,  79,
 79,  79,  79,  79,  79,  79,  79,  79,  79,  79,  79,  79,  79,  79,  79,  79,
 79,  79,  79,  79,  79,  79,  79,  79,  79,  79,  78,  78,  78,  78,  78,  78,
 78,  78,  78,  78,  78,  78,  78,  78,  78,  78,  78,  78,  78,  77,  77,  77,
 77,  77,  77,  77,  77,  77,  77,  77,  77,  77,  76,  76,  76,  76,  76,  76,
 76,  76,  76,  76,  76,  75,  75,  75,  75,  75,  75,  75,  75,  75,  75,  74,
 74,  74,  74,  74,  74,  74,  74,  73,  73,  73,  73,  73,  73,  73,  73,  72,
 72,  72,  72,  72,  72,  72,  71,  71,  71,  71,  71,  71,  71,  70,  70,  70,
 70,  70,  70,  69,  69,  69,  69,  69,  69,  68,  68,  68,  68,  68,  68,  67,
 67,  67,  67,  67,  67,  66,  66,  66,  66,  66,  65,  65,  65,  65,  65,  65,
 64,  64,  64,  64,  64,  63,  63,  63,  63,  63,  62,  62,  62,  62,  62,  61,
 61,  61,  61,  61,  60,  60,  60,  60,  60,  59,  59,  59,  59,  59,  58,  58,
 58,  58,  57,  57,  57,  57,  57,  56,  56,  56,  56,  55,  55,  55,  55,  55,
 54,  54,  54,  54,  53,  53,  53,  53,  52,  52,  52,  52,  52,  51,  51,  51,
 51,  50,  50,  50,  50,  49,  49,  49,  49,  48,  48,  48,  48,  48,  47,  47,
 47,  47,  46,  46,  46,  46,  45,  45,  45,  45,  44,  44,  44,  44,  43,  43,
 43,  43,  42,  42,  42,  42,  41,  41,  41,  41,  40,  40,  40,  40,  39,  39,
 39,  39,  39,  38,  38,  38,  38,  37,  37,  37,  37,  36,  36,  36,  36,  35,
 35,  35,  35,  34,  34,  34,  34,  33,  33,  33,  33,  32,  32,  32,  32,  31,
 31,  31,  31,  30,  30,  30,  30,  30,  29,  29,  29,  29,  28,  28,  28,  28,
 27,  27,  27,  27,  26,  26,  26,  26,  26,  25,  25,  25,  25,  24,  24,  24,
 24,  23,  23,  23,  23,  23,  22,  22,  22,  22,  21,  21,  21,  21,  21,  20,
 20,  20,  20,  19,  19,  19,  19,  19,  18,  18,  18,  18,  18,  17,  17,  17,
 17,  17,  16,  16,  16,  16,  16,  15,  15,  15,  15,  15,  14,  14,  14,  14,
 14,  13,  13,  13,  13,  13,  13,  12,  12,  12,  12,  12,  11,  11,  11,  11,
 11,  11,  10,  10,  10,  10,  10,  10,   9,   9,   9,   9,   9,   9,   8,   8,
  8,   8,   8,   8,   7,   7,   7,   7,   7,   7,   7,   6,   6,   6,   6,   6,
  6,   6,   5,   5,   5,   5,   5,   5,   5,   5,   4,   4,   4,   4,   4,   4,
  4,   4,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   2,   2,   2,   2,
  2,   2,   2,   2,   2,   2,   2,   1,   1,   1,   1,   1,   1,   1,   1,   1,
  1,   1,   1,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,
  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,
  2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   4,
  4,   4,   4,   4,   4,   4,   4,   5,   5,   5,   5,   5,   5,   5,   5,   6,
  6,   6,   6,   6,   6,   6,   7,   7,   7,   7,   7,   7,   7,   8,   8,   8,
  8,   8,   8,   9,   9,   9,   9,   9,   9,  10,  10,  10,  10,  10,  10,  11,
 11,  11,  11,  11,  11,  12,  12,  12,  12,  12,  13,  13,  13,  13,  13,  13,
 14,  14,  14,  14,  14,  15,  15,  15,  15,  15,  16,  16,  16,  16,  16,  17,
 17,  17,  17,  17,  18,  18,  18,  18,  18,  19,  19,  19,  19,  19,  20,  20,
 20,  20,  21,  21,  21,  21,  21,  22,  22,  22,  22,  23,  23,  23,  23,  23,
 24,  24,  24,  24,  25,  25,  25,  25,  26,  26,  26,  26,  26,  27,  27,  27,
 27,  28,  28,  28,  28,  29,  29,  29,  29,  30,  30,  30,  30,  30,  31,  31,
 31,  31,  32,  32,  32,  32,  33,  33,  33,  33,  34,  34,  34,  34,  35,  35,
 35,  35,  36,  36,  36,  36,  37,  37,  37,  37,  38,  38,  38,  38,  39,  39};

static SInt16 sinTableY[] = {
  99,  100,  100,  101,  101,  102,  103,  103,  104,  105,  105,  106,
 106,  107,  108,  108,  109,  109,  110,  111,  111,  112,  112,  113,
 114,  114,  115,  115,  116,  117,  117,  118,  119,  119,  120,  120,
 121,  122,  122,  123,  123,  124,  124,  125,  126,  126,  127,  127,
 128,  129,  129,  130,  130,  131,  132,  132,  133,  133,  134,  134,
 135,  136,  136,  137,  137,  138,  138,  139,  140,  140,  141,  141,
 142,  142,  143,  143,  144,  145,  145,  146,  146,  147,  147,  148,
 148,  149,  149,  150,  150,  151,  151,  152,  152,  153,  154,  154,
 155,  155,  156,  156,  157,  157,  158,  158,  159,  159,  160,  160,
 161,  161,  161,  162,  162,  163,  163,  164,  164,  165,  165,  166,
 166,  167,  167,  168,  168,  168,  169,  169,  170,  170,  171,  171,
 171,  172,  172,  173,  173,  174,  174,  174,  175,  175,  176,  176,
 176,  177,  177,  177,  178,  178,  179,  179,  179,  180,  180,  180,
 181,  181,  181,  182,  182,  182,  183,  183,  183,  184,  184,  184,
 185,  185,  185,  186,  186,  186,  187,  187,  187,  187,  188,  188,
 188,  189,  189,  189,  189,  190,  190,  190,  190,  191,  191,  191,
 191,  192,  192,  192,  192,  193,  193,  193,  193,  193,  194,  194,
 194,  194,  194,  195,  195,  195,  195,  195,  195,  196,  196,  196,
 196,  196,  196,  196,  197,  197,  197,  197,  197,  197,  197,  197,
 198,  198,  198,  198,  198,  198,  198,  198,  198,  198,  198,  198,
 199,  199,  199,  199,  199,  199,  199,  199,  199,  199,  199,  199,
 199,  199,  199,  199,  199,  199,  199,  199,  199,  199,  199,  199,
 199,  199,  199,  199,  199,  199,  199,  199,  199,  198,  198,  198,
 198,  198,  198,  198,  198,  198,  198,  198,  198,  197,  197,  197,
 197,  197,  197,  197,  197,  196,  196,  196,  196,  196,  196,  196,
 195,  195,  195,  195,  195,  195,  194,  194,  194,  194,  194,  193,
 193,  193,  193,  193,  192,  192,  192,  192,  191,  191,  191,  191,
 190,  190,  190,  190,  189,  189,  189,  189,  188,  188,  188,  187,
 187,  187,  187,  186,  186,  186,  185,  185,  185,  184,  184,  184,
 183,  183,  183,  182,  182,  182,  181,  181,  181,  180,  180,  180,
 179,  179,  179,  178,  178,  177,  177,  177,  176,  176,  176,  175,
 175,  174,  174,  174,  173,  173,  172,  172,  171,  171,  171,  170,
 170,  169,  169,  168,  168,  168,  167,  167,  166,  166,  165,  165,
 164,  164,  163,  163,  162,  162,  161,  161,  161,  160,  160,  159,
 159,  158,  158,  157,  157,  156,  156,  155,  155,  154,  154,  153,
 152,  152,  151,  151,  150,  150,  149,  149,  148,  148,  147,  147,
 146,  146,  145,  145,  144,  143,  143,  142,  142,  141,  141,  140,
 140,  139,  138,  138,  137,  137,  136,  136,  135,  134,  134,  133,
 133,  132,  132,  131,  130,  130,  129,  129,  128,  127,  127,  126,
 126,  125,  124,  124,  123,  123,  122,  122,  121,  120,  120,  119,
 119,  118,  117,  117,  116,  115,  115,  114,  114,  113,  112,  112,
 111,  111,  110,  109,  109,  108,  108,  107,  106,  106,  105,  105,
 104,  103,  103,  102,  101,  101,  100,  100,   99,   98,   98,   97,
  97,   96,   95,   95,   94,   93,   93,   92,   92,   91,   90,   90,
  89,   89,   88,   87,   87,   86,   86,   85,   84,   84,   83,   83,
  82,   81,   81,   80,   79,   79,   78,   78,   77,   76,   76,   75,
  75,   74,   74,   73,   72,   72,   71,   71,   70,   69,   69,   68,
  68,   67,   66,   66,   65,   65,   64,   64,   63,   62,   62,   61,
  61,   60,   60,   59,   58,   58,   57,   57,   56,   56,   55,   55,
  54,   53,   53,   52,   52,   51,   51,   50,   50,   49,   49,   48,
  48,   47,   47,   46,   46,   45,   44,   44,   43,   43,   42,   42,
  41,   41,   40,   40,   39,   39,   38,   38,   37,   37,   37,   36,
  36,   35,   35,   34,   34,   33,   33,   32,   32,   31,   31,   30,
  30,   30,   29,   29,   28,   28,   27,   27,   27,   26,   26,   25,
  25,   24,   24,   24,   23,   23,   22,   22,   22,   21,   21,   21,
  20,   20,   19,   19,   19,   18,   18,   18,   17,   17,   17,   16,
  16,   16,   15,   15,   15,   14,   14,   14,   13,   13,   13,   12,
  12,   12,   11,   11,   11,   11,   10,   10,   10,    9,    9,    9,
   9,    8,    8,    8,    8,    7,    7,    7,    7,    6,    6,    6,
   6,    5,    5,    5,    5,    5,    4,    4,    4,    4,    4,    3,
   3,    3,    3,    3,    3,    2,    2,    2,    2,    2,    2,    2,
   1,    1,    1,    1,    1,    1,    1,    1,    0,    0,    0,    0,
   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   0,    0,    0,    0,    0,    1,    1,    1,    1,    1,    1,    1,
   1,    2,    2,    2,    2,    2,    2,    2,    3,    3,    3,    3,
   3,    3,    4,    4,    4,    4,    4,    5,    5,    5,    5,    5,
   6,    6,    6,    6,    7,    7,    7,    7,    8,    8,    8,    8,
   9,    9,    9,    9,   10,   10,   10,   11,   11,   11,   11,   12,
  12,   12,   13,   13,   13,   14,   14,   14,   15,   15,   15,   16,
  16,   16,   17,   17,   17,   18,   18,   18,   19,   19,   19,   20,
  20,   21,   21,   21,   22,   22,   22,   23,   23,   24,   24,   24,
  25,   25,   26,   26,   27,   27,   27,   28,   28,   29,   29,   30,
  30,   30,   31,   31,   32,   32,   33,   33,   34,   34,   35,   35,
  36,   36,   37,   37,   37,   38,   38,   39,   39,   40,   40,   41,
  41,   42,   42,   43,   43,   44,   44,   45,   46,   46,   47,   47,
  48,   48,   49,   49,   50,   50,   51,   51,   52,   52,   53,   53,
  54,   55,   55,   56,   56,   57,   57,   58,   58,   59,   60,   60,
  61,   61,   62,   62,   63,   64,   64,   65,   65,   66,   66,   67,
  68,   68,   69,   69,   70,   71,   71,   72,   72,   73,   74,   74,
  75,   75,   76,   76,   77,   78,   78,   79,   79,   80,   81,   81,
  82,   83,   83,   84,   84,   85,   86,   86,   87,   87,   88,   89,
  89,   90,   90,   91,   92,   92,   93,   93,   94,   95,   95,   96,
  97,   97,   98,   98};

// These arrays form a set of doubly-linked lists. At each location in the
// array is the point number of the next/previous point in the chain.
UInt16 far* pointsNext;
UInt16 far* pointsPrev;

// The following are either:
//   0xffff: The corresponding list is empty.
//   anything else: A point in the list.
UInt16 freeList;  // List of points which haven't been placed in their final locations yet.
UInt16 usedList;  // List of points which are finalized.
UInt16 oddList;   // List of points which we can't find anywhere else for.

// Moves a point to its own list.
//void remove(UInt16 point)
//{
//    UInt16 oldPrev = pointsPrev[point];
//    UInt16 oldNext = pointsNext[point];
//    if (freeList == point)
//        if (oldNext == freeList)
//            freeList == 0xffff;
//        else
//            freeList = oldNext;
//    pointsNext[oldPrev] = oldNext;
//    pointsPrev[oldNext] = oldPrev;
//    pointsNext[point] = point;
//    pointsPrev[point] = point;
//}

// Moves a point to (the end of) usedList.
void place(UInt16 point, UInt16* list = &usedList)
{
    UInt16 oldPrev = pointsPrev[point];
    UInt16 oldNext = pointsNext[point] & 0x7fff;
//    printf("Placing point 0x%04x ",point);
//    if (pointsNext[oldPrev] != point || pointsPrev[oldNext] != point) {
//        printf("Point not in list correctly!\n");
//        exit(1);
//    }
    if (freeList == point)
        if (oldNext == freeList)
            freeList = 0xffff;
        else
            freeList = oldNext;
    if (usedList == point)
        if (oldNext == usedList)
            usedList = 0xffff;
        else
            usedList = oldNext;
    if (oddList == point)
        if (oldNext == oddList)
            oddList = 0xffff;
        else
            oddList = oldNext;
    pointsNext[oldPrev] = (pointsNext[oldPrev] & 0x8000) | oldNext;
    pointsPrev[oldNext] = oldPrev;

//    printf("Placing point 0x%04x",point);
    if (*list == 0xffff) {
//        printf(", first on list\n");
        *list = point;
        pointsNext[point] = point;
        pointsPrev[point] = point;
    }
    else {
//        printf("\n");
        pointsNext[point] = *list;
        pointsPrev[point] = pointsPrev[*list];
        pointsNext[pointsPrev[*list]] = point;
        pointsPrev[*list] = point;
    }
//    printf("between 0x%04x and 0x%04x\n",pointsPrev[point], pointsNext[point]);
}

Bool haveFree()
{
    return freeList != 0xffff;
}

Bool isVisible(UInt16 point)
{
    return (point & 0x1fff) < width*height/2;
}

int xCoordinate(UInt16 point)
{
    return (point & 0x1fff) % width;
}

int yCoordinate(UInt16 point)
{
    return ((point & 0x1fff) / width)*2 + (point >> 13);
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
    for (p = 0; p < pointCount; ++p)
        if (isVisible(p))
            pointsPrev[pointsNext[p]] = p;

//    printf("Visible = %i\n",visible);
//    for (p = 0; p < pointCount; ++p)
//        if (isVisible(p)) {
//            if (pointsPrev[pointsNext[p]] != p) {
//                printf("pointsPrev[pointsNext[0x%04x]] = 0x%04x, pointsNext[0x%04x] = 0x%04x\n",p,pointsPrev[pointsNext[p]],p,pointsNext[p]);
//                exit(1);
//            }
//            if (pointsNext[pointsPrev[p]] != p) {
//                printf("pointsNext[pointsPrev[0x%04x]] = 0x%04x, pointsPrev[0x%04x] = 0x%04x\n",p,pointsNext[pointsPrev[p]],p,pointsPrev[p]);
//                exit(1);
//            }
//        }
}

UInt16 pointFromCoordinates(UInt16 x, UInt16 y)
{
    return ((y & 1) << 13) + (y >> 1)*width + x;
}

UInt16 leftOf(UInt16 point)
{
    if (xCoordinate(point) == 0)
        return point + width - 1;
    return point - 1;
}

UInt16 rightOf(UInt16 point)
{
    if (xCoordinate(point) == width - 1)
        return point + 1 - width;
    return point + 1;
}

UInt16 above(UInt16 point)
{
    UInt16 y = yCoordinate(point);
    if (y == 0)
        return point + 0x2000 + width*(height - 2)/2;
    if ((y & 1) == 0)
        return point + 0x2000 - width;
    return point - 0x2000;
}

UInt16 below(UInt16 point)
{
    UInt16 y = yCoordinate(point);
    if (y == height - 1)
        return point - 0x2000 - width*(height - 2)/2;
    if ((y & 1) == 0)
        return point + 0x2000;
    return point + width - 0x2000;
}

UInt16 isqrt(UInt32 x2)
{
    UInt16 x = 1;
    UInt16 rem;
    UInt16 div;
    UInt16 x2b = 30;
    if (x2 == 0)
        return 0;
    while ((rem = (UInt16)(x2>>x2b)) == 0)
        x2b -= 2;
    if (x2b == 0)
        return 1;
    --rem;
    while (x2b != 0) {
        rem = (rem<<2) | ((UInt16)(x2>>(x2b -= 2))&3);
        div = (x<<2) + 1;
        x <<= 1;
        if (rem >= div) {
            rem -= div;
            x |= 1;
        }
    }
    return x;
}

void setMotion(int pattern)
{
    UInt16 point;
    switch (pattern) {
        case 0:
            // Rightwards
            for (point = 0; point < pointCount; ++point)
                pointsNext[point] = rightOf(point);
            break;
        case 1:
            // Leftwards
            for (point = 0; point < pointCount; ++point)
                pointsNext[point] = leftOf(point);
            break;
        case 2:
            // Split vertically 2x100
            for (point = 0; point < pointCount; ++point)
                if (yCoordinate(point) < 100)
                    pointsNext[point] = leftOf(point);
                else
                    pointsNext[point] = rightOf(point);
            break;
        case 3:
            // Split vertically 4x50
            for (point = 0; point < pointCount; ++point)
                if (yCoordinate(point)%100 < 50)
                    pointsNext[point] = leftOf(point);
                else
                    pointsNext[point] = rightOf(point);
            break;
        case 4:
            // Split vertically 8x25
            for (point = 0; point < pointCount; ++point)
                if (yCoordinate(point)%50 < 25)
                    pointsNext[point] = leftOf(point);
                else
                    pointsNext[point] = rightOf(point);
            break;
        case 5:
            // Split vertically 25x8
            for (point = 0; point < pointCount; ++point)
                if ((yCoordinate(point) & 15) < 8)
                    pointsNext[point] = leftOf(point);
                else
                    pointsNext[point] = rightOf(point);
            break;
        case 6:
            // Split vertically 50x4
            for (point = 0; point < pointCount; ++point)
                if ((yCoordinate(point) & 7) < 4)
                    pointsNext[point] = leftOf(point);
                else
                    pointsNext[point] = rightOf(point);
            break;
        case 7:
            // Split vertically 100x2
            for (point = 0; point < pointCount; ++point)
                if ((yCoordinate(point) & 3) < 2)
                    pointsNext[point] = leftOf(point);
                else
                    pointsNext[point] = rightOf(point);
            break;
        case 8:
            // Split vertically 200x1
            for (point = 0; point < pointCount; ++point)
                if ((yCoordinate(point) & 1) == 0)
                    pointsNext[point] = leftOf(point);
                else
                    pointsNext[point] = rightOf(point);
            break;
        case 9:
            // Split horizontally 2x40
            for (point = 0; point < pointCount; ++point)
                if (xCoordinate(point) < 40)
                    pointsNext[point] = above(point);
                else
                    pointsNext[point] = below(point);
            break;
        case 10:
            // Split horizontally 4x20
            for (point = 0; point < pointCount; ++point)
                if (xCoordinate(point)%40 < 20)
                    pointsNext[point] = above(point);
                else
                    pointsNext[point] = below(point);
            break;
        case 11:
            // Split horizontally 8x10
            for (point = 0; point < pointCount; ++point)
                if (xCoordinate(point)%20 < 10)
                    pointsNext[point] = above(point);
                else
                    pointsNext[point] = below(point);
            break;
        case 12:
            // Split horizontally 10x8
            for (point = 0; point < pointCount; ++point)
                if ((xCoordinate(point) & 15) < 8)
                    pointsNext[point] = above(point);
                else
                    pointsNext[point] = below(point);
            break;
        case 13:
            // Split horizontally 20x4
            for (point = 0; point < pointCount; ++point)
                if ((xCoordinate(point) & 7) < 4)
                    pointsNext[point] = above(point);
                else
                    pointsNext[point] = below(point);
            break;
        case 14:
            // Split horizontally 40x2
            for (point = 0; point < pointCount; ++point)
                if ((xCoordinate(point) & 3) < 2)
                    pointsNext[point] = above(point);
                else
                    pointsNext[point] = below(point);
            break;
        case 15:
            // Split horizontally 80x1
            for (point = 0; point < pointCount; ++point)
                if ((xCoordinate(point) & 1) == 0)
                    pointsNext[point] = above(point);
                else
                    pointsNext[point] = below(point);
            break;
        case 16:
            // Square rotation
            for (point = 0; point < pointCount; ++point) {
                SInt16 x = xCoordinate(point)*2 - 79;
                SInt16 y = yCoordinate(point)*2 - 199;
                SInt16 a = x+y/3;
                SInt16 b = x-y/3;
                if (a > 0)
                    if (b > 0)
                        pointsNext[point] = below(below(below(point)));
                    else
                        pointsNext[point] = leftOf(point);
                else
                    if (a < 0)
                        if (b >= 0)
                            pointsNext[point] = rightOf(point);
                        else
                            pointsNext[point] = above(above(above(point)));
                    else
                        if (b > 0)
                            pointsNext[point] = below(point);
                        else
                            pointsNext[point] = above(point);
            }
            break;
        case 17:
            // Vortex rotation
            {
                shuffle();
                UInt16 first = freeList;
                for (int i = 0; i < pointCount; ++i)
                    pointsNext[i] |= 0x8000;
                oddList = 0xffff;
                do {
                    point = freeList;
                    freeList = pointsNext[point] & 0x7fff;
                    usedList = 0xffff;
                    Bool placed = false;
                    place(point);
                    do {
//                        ++*(UInt8 far*)MK_FP(0xb800, point);
                        SInt16 ox = (xCoordinate(point) - 40)<<8;
                        SInt16 oy = (yCoordinate(point) - 100)<<8;
                        SInt32 oxl = (ox * 16L)/5;
                        SInt32 oyl = (oy * 24L)/25;
                        UInt16 d = isqrt(oxl*oxl + oyl*oyl);
                        d >>= 8;
                        if (d == 0)
                            d = 1;
                        SInt32 vx = 450L*oyl/d;
                        SInt32 vy = -1500L*oxl/d;
                        SInt16 x = ox + (short)(vx>>8);
                        SInt16 y = oy + (short)(vy>>8);
                        if (x >= 0x2800 || x < -0x2800 || y >= 0x6400 || y < -0x6400)
                            break;
                        UInt16 p = pointFromCoordinates((x >> 8) + 40, (y >> 8) + 100);
                        if ((pointsNext[p] & 0x8000) == 0)
                            break;
                        place(p);
                        point = p;
                        placed = true;
                    } while (true);
                    if (!placed)
                        place(point, &oddList);
                } while (freeList != 0xffff);
            }
            break;
        case 18:
            // Body rotation
            {
                shuffle();
                UInt16 first = freeList;
                for (int i = 0; i < pointCount; ++i)
                    pointsNext[i] |= 0x8000;
                oddList = 0xffff;
                do {
                    point = freeList;
                    freeList = pointsNext[point] & 0x7fff;
                    usedList = 0xffff;
                    Bool placed = false;
                    place(point);
                    do {
//                        ++*(UInt8 far*)MK_FP(0xb800, point);
                        SInt16 ox = (xCoordinate(point) - 40)<<8;
                        SInt16 oy = (yCoordinate(point) - 100)<<8;
                        SInt32 vx = oy*0x180L/5;
                        SInt32 vy = -ox*0xa00L/3;
                        SInt16 x = ox + (short)((vx/10)>>8);
                        SInt16 y = oy + (short)((vy/10)>>8);
                        if (x >= 0x2800 || x < -0x2800 || y >= 0x6400 || y < -0x6400)
                            break;
                        UInt16 p = pointFromCoordinates((x >> 8) + 40, (y >> 8) + 100);
                        if ((pointsNext[p] & 0x8000) == 0)
                            break;
                        place(p);
                        point = p;
                        placed = true;
                    } while (true);
                    if (!placed)
                        place(point, &oddList);
                } while (freeList != 0xffff);
            }
            break;
        case 19:
            // Lissajous
            {
                shuffle();
                UInt32 xa = 0;
                UInt32 ya = 0;
                usedList = 0xffff;
                do {
                    SInt16 x = sinTableX[(xa >> 16) & 0x3ff];
                    SInt16 y = sinTableY[(ya >> 16) & 0x3ff];
                    xa += 0x10000;
                    ya += 48219;
                    UInt16 p = pointFromCoordinates(x, y);
                    ++*(UInt8 far*)MK_FP(0xb800, p);
                    place(p);
                } while (freeList != 0xffff);
            }
            break;
        case 20:
            // Starfield
            {
                shuffle();
                SInt16* factors;

                // Compute amount of space needed
                UInt32 f = 0x10000L;
                int n = 0;
                UInt16 ff = 0x110;
                do {
                    f = (f << 8) / ff;
                    if (f < 0x28f)
                        break;
                    ++n;
                } while (true);
                int m = 0;
                f = 0x10000L;
                do {
                    f = (f * ff) >> 8;
                    if (f >= 0x640000L)
                        break;
                    ++m;
                } while (true);
                factors = (SInt16*)malloc((1+n+m)*sizeof(SInt16));
                if (factors == 0) {
                    printf("Not enough memory for factors array!\n");
                    exit(1);
                }

                // Fill in factors array
                f = 0x10000L;
                int i = n;
                while (i >= 0) {
                    factors[i--] = (SInt16)(f >> 8);
                    f = (f << 8) / ff;
                }
                f = 0x10000L;
                i = n + 1;
                while (i < 1+n+m) {
                    f = (f * ff) >> 8;
                    factors[i++] = (SInt16)(f >> 8);
                }
                n = n + m + 1;

                // Compute trails
                usedList = 0xffff;
                do {
                    point = freeList;
                    if (point == 0xffff)
                        break;
                    SInt16 ox = (xCoordinate(point) - 40)<<8;
                    SInt16 oy = (yCoordinate(point) - 100)<<8;
                    for (int i = 0; i < n; ++i) {
                        SInt16 x = (short)(((long)factors[i]*(long)ox) >> 8);
                        SInt16 y = (short)(((long)factors[i]*(long)oy) >> 8);
                        UInt16 p = pointFromCoordinates((x >> 8) + 40, (y >> 8) + 100);
                        ++*(UInt8 far*)MK_FP(0xb800, p);
                        if (x >= 0x2800 || x < -0x2800 || y >= 0x6400 || y < -0x6400)
                            break;
                        place(p);
                    }
                } while (true);
                free(factors);
            }
            break;
    }
}

int main()
{
    UInt8 far* program = (UInt8 far*)(_fmalloc((pointCount + 1)*2 +
        particleCount*particleSize + headerSize + footerSize));
    UInt32 address = ((UInt32)(FP_SEG(program))) << 4;
    address += (UInt32)(FP_OFF(program));
    UInt16 segment = (UInt16)((address + 0xf) >> 4);
    UInt16 far* points = (UInt16 far*)MK_FP(segment, 0);
    pointsPrev = (UInt16 far*)(_fmalloc(pointCount*2));

    _fmemset(points, 0, (pointCount + 1)*2);

    // Initialize the code block
    UInt8 far* particles = (UInt8 far*)MK_FP(segment, (pointCount + 1)*2);
    _fmemcpy(particles, headerCode, headerSize);
    particles += headerSize;
    for (UInt16 particle = 0; particle < particleCount; ++particle) {
        // Copy the code
        _fmemcpy(particles, particleCode, particleSize);

        // Find an unused random point.
        UInt16 r;
        do {
            r = rand() % pointCount;
        } while (points[r] != 0 || !isVisible(r));
        points[r] = 1;

        // Fill in the operands
        UInt8 far* p = &particles[1];
        *(UInt16 far*)(p) = r;
        *(UInt16 far*)(&particles[13]) = FP_OFF(p);
        particles[10] = ((rand()%7 + 1) << 3) + 5;

        particles += particleSize;
    }
    _fmemcpy(particles, footerCode, footerSize);
    pointsNext = points + 1;

    // Set screen mode to 4 (CGA 320x200 2bpp)
    union REGS regs;
    regs.x.ax = 0x04;
    int86(0x10, &regs, &regs);

    // Call code repeatedly. TODO: jmp instead of retf in the code and use interrupt for keyboard
    void (far* code)() = (void (far*)())MK_FP(segment, (pointCount + 1)*2);
    int k = 0;
    int pattern = 0;
    setMotion(pattern);

    do {
        code();
        if (kbhit()) {
            k = getch();
            if (k == 27)
                break;
            if (k == ' ')
                pattern = (pattern + 1) % patterns;
            else
                if (k >= '0' && k <= '9')
                    pattern = k - '0';
                else
                    if (k >= 'a' && k <= 'z')
                        pattern = k + 10 - 'a';
                    else
                        if (k >= 'A' && k <= 'Z')
                            pattern = k + 10 - 'A';
            setMotion(pattern);
        }
    } while (k != 27);
    regs.x.ax = 0x03;
    int86(0x10, &regs, &regs);

    _ffree(pointsPrev);
    _ffree(program);
    return 0;
}
