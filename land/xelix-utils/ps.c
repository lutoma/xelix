#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pwd.h>
#include "util.h"

int main(int argc, char* argv[]) {
	FILE* fp = fopen("/sys/tasks", "r");
	if(!fp) {
		perror("Could not read tasks");
		exit(EXIT_FAILURE);
	}

	char* data = malloc(1024);
	fgets(data, 1024, fp);


	printf("%-4s %-8s %-10s %-4s %-8s\n", "PID", "User", "State", "PPID", "Mem");

	while(true) {
		if(feof(fp)) {
			return EXIT_SUCCESS;
		}

		uint32_t pid;
		uint32_t uid;
		uint32_t gid;
		uint32_t ppid;
		char cstate;
		char name[300];
		uint32_t mem;
		uint32_t entry;
		uint32_t sbrk;
		uint32_t stack;

		if(fscanf(fp, "%d %d %d %d %c %s %d 0x%x 0x%x 0x%x\n", &pid, &uid, &gid, &ppid, &cstate, name, &mem, &entry, &sbrk, &stack) != 10) {
			fprintf(stderr, "Matching error.\n");
			exit(EXIT_FAILURE);
		}

		char* state = "Unknown";
		switch(cstate) {
			case 'K': state = "Killed"; break;
			case 'T': state = "Terminated"; break;
			case 'B': state = "Blocking"; break;
			case 'S': state = "Stopped"; break;
			case 'R': state = "Running"; break;
			case 'W': state = "Waiting"; break;
			case 'C': state = "Syscall"; break;
		}

		char* user;
		struct passwd* pwd = getpwuid(uid);
		if(pwd) {
			user = pwd->pw_name;
		} else {
			char _user[10];
			itoa(uid, _user, 10);
			user = _user;
		}

		char buf[100];
		printf("%-4d %-8s %-10s %-4d %-8s %-15s\n", pid, user, state, ppid, readable_fs(mem, buf), name);
	}

	exit(EXIT_SUCCESS);
}
