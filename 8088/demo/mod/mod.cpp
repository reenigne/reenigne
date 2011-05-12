#include <stdio.h>

typedef unsigned long int  UInt32;
typedef signed long int    SInt32;
typedef unsigned short int UInt16;
typedef signed short int   SInt16;
typedef unsigned char      UInt8;
typedef signed char        SInt8;
typedef int                Bool;

static const Bool true = 1;
static const Bool false = 0;

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Syntax: %s <filename>\n", argv[0]);
        exit(1);
    }
    FILE* in = fopen(argv[1], "rb");
    fseek(in, 0, SEEK_END);
    UInt32 s = ftell(in);
    fseek(in, 0, SEEK_SET);
    UInt8* mod = 0;
    if (s <= 0xffffL)
        mod = (UInt8*)malloc((UInt16)s);
    if (mod == 0) {
        printf("File too large - must be under %u bytes\n", coreleft());
        exit(1);
    }
    fread((void*)mod, (UInt16)s, 1, in);
    fclose(in);
    // TODO: Compute offset of start of samples
    // TODO: Compute segment for player/sample data

}

