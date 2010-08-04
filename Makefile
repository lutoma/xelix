

kernel: init/main.o init/loader.o devices/display/generic.o devices/cpu/generic.o common/generic.o
	ld -T linker.ld -o kernel.bin $^

# how to compile .c to .o
.c.o:
	gcc -Wall -I . -nostartfiles -nodefaultlibs -nostdlib -o $@ -c $<


# dependencies
init/main.c: common/generic.h devices/display/interface.h devices/cpu/interface.h

common/generic.c: common/generic.h devices/display/interface.h

devices/display/interface.h: common/generic.h
devices/display/generic.c: devices/display/interface.h
devices/cpu/interface.h: common/generic.h
devices/cpu/generic.c: devices/cpu/interface.h



clean:
	rm -rf kernel.bin init/main.o init/loader.o devices/display/generic.o common/generic.o


init/loader.o: init/loader.s
	nasm -f elf -o init/loader.o init/loader.s


run: kernel
	qemu -kernel kernel.bin

