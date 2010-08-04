

kernel: init/main.o init/loader.o devices/display/generic.o common.o
	ld -T linker.ld -o kernel.bin $^

# how to compile .c to .o
.c.o:
	gcc -Wall -I . -nostartfiles -nodefaultlibs -nostdlib -o $@ -c $<


# dependencies
init/main.c: common.h devices/display/interface.h

common.c: common.h

devices/display/interface.h: common.h
devices/display/generic.c: devices/display/interface.h




clean:
	rm -rf kernel.bin init/main.o init/loader.o devices/display/generic.o common.o


init/loader.o: init/loader.s
	nasm -f elf -o init/loader.o init/loader.s


run: kernel
	qemu -kernel kernel.bin

