
#include <multiboot.h>
#include <multiboot2.h>

.code32

.section .multiboot
/* Multiboot 1 header */
.set MULTIBOOT_HEADER_FLAGS, (MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE)
.align MULTIBOOT_HEADER_ALIGN
multiboot_header:
/* magic */
.long   MULTIBOOT_HEADER_MAGIC
/* flags */
.long   MULTIBOOT_HEADER_FLAGS
/* checksum */
.long   -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
.long   0
.long   0
.long   0
.long   0
.long   0 
/* Request linear graphics mode? */
.long 0  /* any */
.long 800
.long 600
.long 32

/* Multiboot 2 header */
.set MB2_MAGIC,    MULTIBOOT2_HEADER_MAGIC
.set MB2_ARCH,     MULTIBOOT2_ARCHITECTURE_I386
.set MB2_LENGTH,   (multiboot2_header_end - multiboot2_header)
.set MB2_CHECKSUM, -(MB2_MAGIC + MB2_ARCH + MB2_LENGTH)
.align MULTIBOOT2_HEADER_ALIGN
multiboot2_header:
.long MULTIBOOT2_HEADER_MAGIC
.long MULTIBOOT2_ARCHITECTURE_I386
.long MB2_LENGTH
.long MB2_CHECKSUM

/* Framebuffer tag */
.align MULTIBOOT2_HEADER_ALIGN
.word MULTIBOOT2_HEADER_TAG_FRAMEBUFFER
.word 0     /* (don't ignore) don't boot if PC doesn't support framebuffer */
.long 20    /* size */
.long 800     /* width */
.long 600     /* height */
.long 32     /* depth */

.align MULTIBOOT2_HEADER_ALIGN
.word MULTIBOOT2_HEADER_TAG_CONSOLE_FLAGS
/* Flags */
.word 1  /* bit ‘0’ of ‘flags’ (also known as ‘optional’) (the bootloader may ignore this tag) */
/* size */
.long 12 
/* console_flags: if bit 0 of ‘console_flags’ is set at least one of supported consoles must be present 
and information about it must be available in mbi ,
If bit ‘1’ of ‘console_flags’ is set it indicates that the OS image has EGA */
.long 2 

/* End tag */
.align MULTIBOOT2_HEADER_ALIGN
.word MULTIBOOT2_HEADER_TAG_END
.word 0
.long 8
multiboot2_header_end: