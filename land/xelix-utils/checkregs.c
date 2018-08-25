#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

int eax asm("eax");
int ebx asm("ebx");
int ecx asm("ecx");
int edx asm("edx");

int main(int argc, char* argv[]) {
	/*asm(
		"mov $1, %%eax;\n"
		"mov $2, %%ebx;\n"
		"mov $3, %%ecx;\n"
		"mov $4, %%edx;\n"
		::: "eax", "ebx", "ecx", "edx"
	);*/
	eax = 1;
	ebx = 2;
	ecx = 3;
	edx = 4;


	while(true) {
		iprintf("eax: %d, ebx: %d, ecx: %d, edx: %d\n", eax, ebx, ecx, edx);
		fflush(stdout);
		//sleep(1);
	}

	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");
	printf("dummy\n");

	return 0;
}
