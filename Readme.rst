Xelix libc
==========

This is the libc of the Xelix kernel.

Usage
=====

Firstly, compile using ./configure and make. Then, link your program
with the following parameters:

ld -Tlinker.ld -nostdlib -nostdinc -I include -lstdc
