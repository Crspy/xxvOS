# xxvOS

My first hobby OS for x86_64 systems.

## Building & running

### Dependencies

Install the following packages:
+ `grub`
+ `xorriso` for Debian/Ubuntu; `libisoburn` on Archlinux
+ `mtools`
+ `qemu` (recommended)
+ `x86_64 cross compiler`

### Cross-compiler

#### Using a preinstalled cross-compiler

If your distro provides you with a cross compiler like x86_64-elf-gcc on Archlinux, you may want to save time and use it. To do so, you must edit the following variables in the main `Makefile` so that they match the executables of your cross compiler:

    override TARGET := x86_64-elf-

### Running xxvOS

Run

    make run