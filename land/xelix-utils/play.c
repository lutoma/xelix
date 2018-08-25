#include <stdio.h>

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: %s <sound file>\n", argv[0]);
	}

	char* filep = argv[1];

	asm(
		"mov $30, %%eax;\n"
		"mov %0, %%ebx;\n"
		"int $0x80;\n"
		:: "r" (filep) : "eax"
	);


	return 0;
}
