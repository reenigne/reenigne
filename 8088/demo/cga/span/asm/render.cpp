void fillTrapezoid1(int colour, int yStart, int yEnd, int xL, int xR)
{
    asm volatile ("call fillTrapezoid"
        : "+S" (colour), "+b" (yStart), "+c" (yEnd), "+d" (xL), "+a" (xR)
        :
        : "D" );
}
