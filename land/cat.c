#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	printf("hello cat world.\n");

	char* file = "hello.txt";
	FILE* fp = fopen(file, "r");

	char* data = malloc(1024);
	size_t read = fread(data, 1024, 1, fp);
	puts(data);
	exit(EXIT_SUCCESS);
}
