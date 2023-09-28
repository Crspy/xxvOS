/**
 * @brief Turns on the floating-point unit.
 *
 * Enables a few bits so we can get SSE.
 *
 * We don't do any fancy lazy FPU reload as x86-64 assumes a wide
 * variety of FPU-provided registers are available so most userspace
 * code will be messing with the FPU anyway and we'd probably just
 * waste time with all the interrupts turning it off and on...
 */
 .code64
.align 8
.global fpu_initialize
.type fpu_initialize, @function

fpu_initialize:
.set CR0_MP, (1 << 1)
.set CR0_EM, (1 << 2)
.set CR0_ET,  (1 << 4)
.set CR4_OSFXSR, (1 << 9)
.set CR4_OSXMMEXCPT, (1 << 10)

    clts /*clear Task switched flag in CR0 (CR0.TS) */
	mov %cr0, %rax
	and $(~CR0_MP), %ax 
	or $CR0_ET, %ax
	mov %rax, %cr0  
	fninit         /* Initialize FPU without checking for pending unmasked floating-point exceptions*/
	mov %cr0, %rax 
	and $(~CR0_EM), %ax 
	or  $CR0_MP, %ax
	mov %rax, %cr0
	mov %cr4, %rax
	or $(CR4_OSFXSR | CR4_OSXMMEXCPT), %rax 
	mov %rax, %cr4 
	push $0x1F80	/* mxcsr bits from 7-12 are enabled (IM | DM | ZM | OM | UM | PM) */
	ldmxcsr (%rsp)
	addq $8, %rsp
    retq

