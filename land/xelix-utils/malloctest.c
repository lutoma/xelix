#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main() {
	printf("malloc test!\n");

	for(int i = 0; i < 20; i++) {
		char* m1 = malloc(100 + i);
		strcpy(m1, "beep boop.");
		printf("0x%x: %s\n", m1, m1);
		free(m1);
	}
}
