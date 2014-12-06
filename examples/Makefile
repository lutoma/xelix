all: fork-test

fork-test: fork-test.asm
	nasm -f elf32 -o fork-test.o fork-test.asm
	ld -melf_i386 -o fork-test fork-test.o
	rm fork-test.o

clean:
	rm -f fork-test
