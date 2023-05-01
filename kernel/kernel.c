#include <stdint.h>
#include <kernel/string.h>
#include <kernel/misc.h>
#include <kernel/arch/x86_64/ports.h>
#include <multiboot.h>
#include <cpu.h>



/**
 * @brief x86-64 multiboot C entrypoint.
 *
 * Called by the x86-64 longmode bootstrap.
 */
void kmain(struct multiboot* mboot, uint32_t mboot_mag, void* esp) {
#define SERIAL_PORT0 0x3F8
    outportb(SERIAL_PORT0 + 3, 0x03); /* Initialization: Disable divisor mode, set parity */
    const char* welcome_msg = "Hello from Port0\n";
    outportbm(SERIAL_PORT0, welcome_msg,strlen(welcome_msg));
    

    arch_hcf();
}
