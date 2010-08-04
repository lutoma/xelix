

kernel: init/main.o init/loader.o
	ld -T linker.ld -o kernel.bin $^

# how to compile .c to .o
.c.o:
	gcc -Wall -nostartfiles -nodefaultlibs -nostdlib -o $@ -c $<


init/loader.o: init/loader.s
	nasm -f elf -o init/loader.o init/loader.s


run: kernel
	qemu -kernel kernel.bin

