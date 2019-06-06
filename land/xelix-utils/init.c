/* Copyright © 2019 Lukas Martini
 *
 * This file is part of Xelix.
 *
 * Xelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <sys/wait.h>
#include "util.h"
#include "ini.h"

struct service {
	struct service* next;

	pid_t pid;
	char tty[PATH_MAX];
	char path[PATH_MAX];
};

struct service* services;

static void launch_service(struct service* service) {
	char* argv[40];
	char* path = strdup(service->path);

	argv[0] = strtok(path, " ");
	if(!argv[0]) {
		return;
	}

	size_t argc = 1;
	for(; argc < 40; argc++) {
		argv[argc] = strtok(NULL, " ");
		if(!argv[argc]) {
			break;
		}
	}

	pid_t pid = fork();
	if(pid == -1) {
		perror("Could not fork");
		return;
	}

	if(pid) {
		service->pid = pid;
		service->next = services;
		services = service;
	} else {
		if(*service->tty) {
			if(open(service->tty, O_RDONLY) < 0) {
				perror("Could not open tty");
				exit(-1);
			}
		}

		char* env[] = {"foo=bar", NULL};
		execve(argv[0], argv, env);
	}
}

#define MATCH(s, n) strcasecmp(section, s) == 0 && strcasecmp(name, n) == 0
static int handler(void* user, const char* section, const char* name,
                   const char* value) {

	struct service* service = (struct service*)user;
    if (MATCH("service", "execstart")) {
    	strlcpy(service->path, value, PATH_MAX);
	} else if (MATCH("service", "tty")) {
    	strlcpy(service->tty, value, PATH_MAX);
    } else {
        return 0;
    }
    return 1;
}


int main() {
	if(getpid() != 1) {
		fprintf(stderr, "This program needs to be run as PID 1.\n");
		return -1;
	}

	open("/dev/tty0", O_RDONLY);
	sigset_t set;
	sigfillset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	if(chdir("/etc/init.d") == -1) {
		perror("Could not switch to services directory");
		return -1;
	}

	DIR* svd_dir = opendir(".");
	if(!svd_dir) {
		perror("Could not open services directory");
		return -1;
	}

	struct dirent* ent = readdir(svd_dir);
	for(; ent; ent = readdir(svd_dir)) {
		if(ent->d_type != DT_REG && ent->d_type != DT_LNK) {
			continue;
		}

		struct service* service = calloc(1, sizeof(struct service));
		if(!service) {
			perror("Could not allocate service struct");
			continue;
		}

	    if(ini_parse(ent->d_name, handler, service) < 0) {
	        fprintf(stderr, "Can't load '%s'\n", ent->d_name);
	        continue;
	    }

	    launch_service(service);
	}

	while(1) {
		int wstat;
		pid_t pid = waitpid(-1, &wstat, 0);

		if(!pid || WIFSTOPPED(wstat) || (WIFSIGNALED(wstat) &&
			(WTERMSIG(wstat) == SIGKILL || WTERMSIG(wstat) == SIGTERM))) {
			continue;
		}

		struct service* service = services;
		for(; service; service = service->next) {
			if(service->pid == pid) {
				launch_service(service);
				break;
			}
		}
	}
}
