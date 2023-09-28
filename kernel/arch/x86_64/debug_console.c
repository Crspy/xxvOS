#include <kernel/arch/x86_64/debug_console.h>
#include <kernel/arch/x86_64/ports.h>
#include <misc/kprintf.h>
void debugcon_init()
{
    kprintf_set_putchar(debugcon_putchar);
}

void debugcon_putchar(char ch)
{
    enum {
        DEBUG_CONSOLE_PORT = 0xE9,
    };
    outportb(DEBUG_CONSOLE_PORT, ch);
}

