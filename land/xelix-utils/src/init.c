/* Copyright © 2019-2023 Lukas Martini
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
#include <sys/mount.h>
#include "util.h"
#include "ini.h"

struct service {
	struct service* next;
	char name[NAME_MAX];
	char target[NAME_MAX];
	char tty[PATH_MAX];
	char path[PATH_MAX];
	pid_t pid;
	enum {
		RESTART_NEVER = 0,
		RESTART_ALWAYS = 1,
		RESTART_ON_FAILURE = 2
	} restart_mode;
};

struct service* services;
DIR* svd_dir;

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
		closedir(svd_dir);

		if(*service->tty) {
			if(open(service->tty, O_RDONLY) < 0) {
				perror("Could not open tty");
				exit(-1);
			}
		}

		chdir("/home/root");
		if(execv(argv[0], argv) < 0) {
			perror("execv failed");
			exit(-1);
		}
	}
}

static int handler(void* user, const char* section, const char* name,
	const char* value) {

	struct service* service = (struct service*)user;
	if(strcasecmp(section, "Service") != 0) {
		return 0;
	}

	if(strcasecmp(name, "ExecStart") == 0) {
		strlcpy(service->path, value, PATH_MAX);
	} else if(strcasecmp(name, "TTY") == 0) {
		strlcpy(service->tty, value, PATH_MAX);
	} else if(strcasecmp(name, "Target") == 0) {
		strlcpy(service->target, value, NAME_MAX);
	} else if(strcasecmp(name, "Restart") == 0) {
		if(strcasecmp(value, "Always") == 0) {
			service->restart_mode = RESTART_ALWAYS;
		} else if(strcasecmp(value, "OnFailure") == 0) {
			service->restart_mode = RESTART_ON_FAILURE;
		}
	} else {
		return 0;
	}
	return 1;
}


static char* get_target() {
	int fd = open("/sys/cmdline", O_RDONLY);
	if(fd < 1) {
		return NULL;
	}

	char buf[500];
	int nread = read(fd, buf, 500);
	close(fd);
	if(nread < 1) {
		return NULL;
	}

	char* pch;
	char* strtok_state;
	int state = 0;
	pch = strtok_r(buf, " =\n", &strtok_state);
	while(pch != NULL) {
		if(state == 0) {
			if(strcmp(pch, "init_target") == 0) {
				state = 2;
			} else {
				state = 1;
			}
		} else if(state == 1) {
			state = 0;
		} else if(state == 2) {
			char* result = malloc(500);
			strlcpy(result, pch, 500);
			return result;
		}
		pch = strtok_r(NULL, " =\n", &strtok_state);
	}

	return NULL;
}



int main() {
	if(getpid() != 1) {
		fprintf(stderr, "This program needs to be run as PID 1.\n");
		return -1;
	}

	open("/dev/console", O_RDONLY);
	sigset_t set;
	sigfillset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	if(mount("/dev/ide1p1", "/boot", "ext2", 0, NULL) < 0) {
		perror("Could not mount /boot");
	}

	char* target = get_target();
	if(!target) {
		target = "default";
	}
	printf("init: Booting target %s\n", target);

	if(chdir("/etc/init.d") == -1) {
		perror("Could not switch to services directory");
		return -1;
	}

	svd_dir = opendir(".");
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

		strlcpy(service->name, ent->d_name, NAME_MAX);
		if(ini_parse(ent->d_name, handler, service) < 0) {
			fprintf(stderr, "Can't load '%s'\n", ent->d_name);
			continue;
		}

		if(!strcmp(service->target, target)) {
			printf("init: Launching service %s, command %s\n", service->name, service->path);
			launch_service(service);
		}
	}

	while(1) {
		int wstat;
		pid_t pid = waitpid(-1, &wstat, 0);
		if(pid < 1) {
			perror("init: waitpid failed");
			exit(EXIT_FAILURE);
		}

		if(!pid) {
			continue;
		}

		struct service* service = services;
		for(; service; service = service->next) {
			if(service->pid == pid) {
				int failed = 0;
				if(WIFEXITED(wstat)) {
					if(WEXITSTATUS(wstat) != 0) {
						failed = 1;
					}
				} else if(WIFSIGNALED(wstat)) {
					if(WTERMSIG(wstat) != SIGKILL && WTERMSIG(wstat) != SIGTERM) {
						failed = 1;
					}
				}
				int do_restart = service->restart_mode == RESTART_ALWAYS ||
					(service->restart_mode == RESTART_ON_FAILURE && failed);

				printf("init: Service %s has %s%s\n",
					service->name, failed ? "failed" : "stopped",
					do_restart ? ", restarting" : "");

				if(do_restart) {
					launch_service(service);
				}
				break;
			}
		}
	}
}
