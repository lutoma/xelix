#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

int main() {
	printf("\nxelix tty1\n\n");
	printf("localhost login: ");
	fflush(stdout);

	char* user = malloc(50);
	char* read = fgets(user, 50, stdin);
	if(!read) {
		return 1;
	}

	printf("Password: ");
	fflush(stdout);

	char* pass = malloc(500);
	read = fgets(pass, 500, stdin);
	if(!read) {
		return 1;
	}

	// Strip the \n
	user[strlen(user) - 1] = (char)0;
	pass[strlen(pass) - 1] = (char)0;

	printf("\n");

	char* cmd = "/xshell";
	char* __argv[] = { "xshell", "-l", NULL };
	char* __env[] = { "PS1=[$USER@$HOST $PWD]# ", "HOME=/root", "TERM=dash", "PWD=/", "USER=root", "HOST=default", NULL }; 

	// Make the syscall
	asm volatile (
		"movl $0x1c, %%eax;\n"
		"movl %0, %%ebx;\n"
		"movl %1, %%ecx;\n"
		"movl %2, %%edx;\n"
		"int $0x80;\n"
		: /* No outputs */
		: "r" (cmd), "r" (__argv), "r" (__env)
		: "memory", "eax", "ebx"
	);

	// PID 1 must not die
	while(true);
}
