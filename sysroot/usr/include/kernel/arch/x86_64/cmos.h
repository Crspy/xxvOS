#pragma once

#include <kernel/arch/x86_64/ports.h>



typedef struct 
{
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned int year;
} date_t;

void arch_clock_initialize(void);
date_t read_rtc();
uint64_t read_epoch_time();