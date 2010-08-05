# kernel binary
kernel: init/loader.o memory/gdta.o interrupts/idta.o  common/generic.o devices/cpu/generic.o devices/display/generic.o init/main.o interrupts/idt.o memory/gdt.o
	ld -T linker.ld -o kernel.bin $^

# dependencies
common/generic.h:
devices/cpu/interface.h: common/generic.h
devices/display/interface.h: common/generic.h
interrupts/idt.h: common/generic.h
memory/gdt.h: common/generic.h
common/generic.c: common/generic.h devices/display/interface.h
devices/cpu/generic.c: devices/cpu/interface.h
devices/display/generic.c: devices/display/interface.h
init/main.c: common/generic.h devices/display/interface.h devices/cpu/interface.h memory/gdt.h interrupts/idt.h
interrupts/idt.c: interrupts/idt.h devices/display/interface.h
memory/gdt.c: memory/gdt.h

# clean
clean:
	rm -rf kernel.bin init/loader.o common/generic.o devices/cpu/generic.o devices/display/generic.o init/main.o interrupts/idt.o memory/gdt.o


# how to compile .c to .o
.c.o:
	gcc -Wall -I . -nostartfiles -nodefaultlibs -nostdlib -o $@ -c $<


init/loader.o: init/loader.asm
	nasm -f elf -o init/loader.o init/loader.asm
memory/gdta.o: memory/gdt.asm
	nasm -f elf -o memory/gdta.o memory/gdt.asm
interrupts/idta.o: interrupts/idt.asm
	nasm -f elf -o interrupts/idta.o interrupts/idt.asm

run: kernel
	qemu -kernel kernel.bin

makefile:
	tools/makefile.py
