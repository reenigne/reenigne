#include <alloc.h>
#include <stdio.h>
#include <dos.h>

int main()
{
    char huge* program = (char huge*)farmalloc(0x1000fl);
    unsigned long address = (((unsigned long)FP_SEG(program))<<4) +
        (unsigned long)FP_OFF(program);
    unsigned int segment = (unsigned int)((address + 0xf) >> 4);
    printf("segment = 0x%04x\n",segment);
    void (far* code)() = MK_FP(segment, 32000);
    int* points = MK_FP(segment, 0);
    for (int point = 0; point < 16000; ++point)
        points[point] = 0;
    for (int particle = 0; particle <

    for (int point = 0; point < 16000; ++point)
        points[point] = (point + 1) % 16000;

}