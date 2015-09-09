static void OS_Write0(char* s)
{
    register char* r0 __asm("r0") = s;
    __asm volatile("swi #0x2" :: "r" (r0) : "cc");
}

static void OS_Exit()
{
    register int r0 __asm("r0") = 0;
    register int r1 __asm("r1") = 0x58454241;
    register int r2 __asm("r2") = 0;
    __asm volatile("swi #0x11" :: "r" (r0), "r" (r1), "r" (r2) : "cc");
}

int main(void)
{
    OS_Write0("Hello, World!\n");
    OS_Exit();
}
