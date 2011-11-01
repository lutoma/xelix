Xelix libc
==========

This is the libc of the Xelix kernel.

Usage
=====

Firstly, compile using ./configure and make. Then, link your program
with the following parameters:

ld -nostdlib -nostdinc -Iinclude -L. -melf_i386 -o <your_program> crt0.o <your_objectfiles> -lstdc
