#include <cpu.h>





// Halt and catch fire function.
void arch_hcf(void)
{
    asm("cli");
    for (;;) {
        asm("hlt");
    }
}

