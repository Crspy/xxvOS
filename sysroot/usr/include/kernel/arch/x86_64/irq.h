#pragma once

#include <kernel/arch/x86_64/regs.h>

/**
 * Interrupt descriptor table
 */
typedef struct {
	uint16_t base_low;
	uint16_t selector;

	uint8_t zero;
	uint8_t flags;

	uint16_t base_mid;
	uint32_t base_high;
	uint32_t pad;
} __attribute__((packed)) idt_entry_t;

struct idt_pointer {
	uint16_t  limit;
	uintptr_t base;
} __attribute__((packed));


