typedef struct
{
} workspace;

workspace** private_word_pointer(void)
{
    register workspace** r12 __asm("r12");
    __asm volatile("" : "=r" (r12));
    return r12;
}

void* error;

inline void error_return(void) __attribute__((always_inline));

void error_return(void)
{
    register void* r0 __asm("r0") = error;
    __asm volatile("orr pc, r14, #0x10000000" :: "r" (r0));
}

void* OS_Module_Claim(int size)
{
    error = 0;
    register int r0 __asm("r0") = 6;
    register void* r2 __asm("r2");
    register int r3 __asm("r3") = size;
    __asm volatile("swi #0x1e\n\t" : "=r" (r0), "=r" (r2) : "r" (r0), "r" (r3) : "cc");
    if (r0 != 6)
        error = (void*)r0;
    return r2;
}

void module_init(void)
{
    workspace** pwp = private_word_pointer();
    workspace* w;
    if (*pwp == 0) {
        w = (workspace*)OS_Module_Claim(sizeof(workspace));
        *pwp = w;
    }
    if (error)
        error_return();
}

void module_finalize(void)
{
}

void module_service_call_handler(void)
{
}

void oscli_serialfs(void)
{
}
