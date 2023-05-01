.code32

/* .stack resides in .bss */
.section .stack, "aw", @nobits
stack_bottom:
.skip 16384 /* 16KiB */
.global stack_top
stack_top:

.section .bootstrap
.code32
.align 4

.extern jmp_to_long
.type jmp_to_long, @function

.extern kmain
.type kmain, @function

.global start
.type start, @function

start:
    /* Setup our stack */
    mov $stack_top, %esp
    /* Make sure our stack is 16-byte aligned */
    and $-16, %esp

    pushl $0
    pushl %esp
    pushl $0
    pushl %eax /* Multiboot header magic */
    pushl $0
    pushl %ebx /* Multiboot header pointer */

    jmp jmp_to_long


.align 4

jmp_to_long:
.extern paging_pml4t
.extern paging_pdpt


.set PAGE_SIZE, 4096
.set PAGE_PRESENT, (1 << 0)
.set PAGE_WRITABLE,(1 << 1)
.set PAGE_PS      ,(1 << 7)         /* PAGE Size */
.set PAGE_FLAGS,    (PAGE_PRESENT | PAGE_WRITABLE)

    /* Set up initial page region, which was zero'd for us by the loader */
    /* PML4T[0] -> PDPT. */
    mov $paging_pdpt, %eax
    orl  $PAGE_FLAGS, %eax
    mov $paging_pml4t, %edi
    mov %eax, (%edi)

    mov $paging_pdpt, %edi
    mov $(PAGE_FLAGS | PAGE_PS), (%edi) /* map 1GB starting from address $0 merged with flags (0  | PAGE_FLAGS) */
   
    mov $paging_pml4t, %edi
    mov %edi, %cr3  /* set our pml4 table pointer in CR3 */

    /* Enable PAE */
.set CR4_PAE_BIT, (1 << 5)
    mov %cr4, %eax
    or $CR4_PAE_BIT, %eax
    mov %eax, %cr4

    /* EFER long mode enable */
.set EFER_ADDRESS, 0xC0000080
.set EFER_LME_BIT, (1 << 8)
    mov $EFER_ADDRESS, %ecx
    rdmsr
    or $EFER_LME_BIT, %eax
    wrmsr

    /* Check (PG) Paging Bit */
.set CR0_PG_BIT, (1 << 31)
    mov %cr0, %eax

    /* If paging was enabled, assume we were already in long mode (eg. booted by EFI) */
    test $CR0_PG_BIT, %eax
    jnz .continue

    /* Otherwise enable paging */
    or $CR0_PG_BIT, %eax
    mov %eax, %cr0

    lgdt gdtr
    ljmp $gdt_base.code,$realm64

.align 8
gdtr:
    .word gdt_end-gdt_base
    .quad gdt_base



/* GDT Access bits */
.set GDT_PRESENT  , (1 << 7)
.set GDT_NOT_SYS  , (1 << 4)
.set GDT_EXEC     , (1 << 3)
.set GDT_DC       , (1 << 2)
.set GDT_RW       , (1 << 1)
.set GDT_ACCESSED , (1 << 0)
 
/* GDT Flag bits */
.set GDT_GRAN_4K   , (1 << 7)
.set GDT_SZ_32     , (1 << 6)
.set GDT_LONG_MODE , (1 << 5)

gdt_base:
    /* Null */
.set gdt_base.null, (. - gdt_base)
    .quad 0
    /* Code */
.set gdt_base.code, (. - gdt_base)
    .word 0xFFFF                                            /* Limit (low, bits 0-15) */
    .word 0                                                 /* Base  */
    .byte 0                                                 /* Base (mid, bits 16-23) */ 
    .byte (GDT_PRESENT | GDT_NOT_SYS | GDT_EXEC | GDT_RW)   /* Access */
    .byte (GDT_GRAN_4K | GDT_LONG_MODE | 0xF)               /* Flags & Limit (high, bits 16-19) */
    .byte 0                                                 /* Base (high, bits 24-31) */
    /*  Data */
.set gdt_base.data, (. - gdt_base)
    .word 0xFFFF                                            /* Limit (low, bits 0-15) */
    .word 0                                                 /* Base  */
    .byte 0                                                 /* Base (mid, bits 16-23) */ 
    .byte (GDT_PRESENT | GDT_NOT_SYS | GDT_RW)              /* Access */
    .byte (GDT_GRAN_4K | GDT_SZ_32   | 0xF)                 /* Flags & Limit (high, bits 16-19) */
    .byte 0                                                 /* Base (high, bits 24-31) */
gdt_end:



.code64
.align 8
.section .bootstrap

realm64:
    cli
    mov $gdt_base.data, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

.continue:
    cli
    pop %rdi
    pop %rsi
    pop %rdx
    callq kmain

halt:
    cli
    hlt
    jmp halt
