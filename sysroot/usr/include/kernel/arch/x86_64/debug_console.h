#pragma once

/*
    debug console for early logging which uses port 0xE9 to output logging messages
    during the start of the kernel initialization
*/
void debugcon_init();
void debugcon_putchar(char ch);

