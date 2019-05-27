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

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define CHUNK_SIZE (0x800 * 2)

static inline char poll_input(int timeout) {
	struct pollfd pfd = {
		.fd = 0,
		.events = POLLIN,
	};

	if(poll(&pfd, 1, timeout) < 1) {
		return 0;
	}
	return getchar();
}

void playback_loop(size_t size, int source_fd, int dest_fd) {
	// 44.1khz sample rate, s16le, 2 channels
	int runtime = size / (44100 * 4);

	size_t total_written;
	float lcperc = 0;
	int lptime = 0;
	bool pause = false;

	uint8_t* buf = malloc(CHUNK_SIZE);

	while(1) {
		char input = poll_input(pause ? -1 : 0);
		if(input) {
			switch(input) {
				case ' ':
					pause = !pause;
					break;
			}
			fflush(stdin);
		}

		if(!pause) {
			if(!read(source_fd, buf, CHUNK_SIZE)) {
				break;
			}

			total_written += write(dest_fd, buf, CHUNK_SIZE);
		}

		float cperc = (float)total_written / (float)size;
		int ptime = total_written / (44100 * 4);

		if(cperc != lcperc || ptime != lptime) {
			printf("\033[2K\033[G %d:%02d / %d:%02d (%2.1f%%)",
				ptime / 60, ptime % 60,
				runtime / 60, runtime % 60,
				cperc * 100);

			if(pause) {
				printf(" (paused)");
			}

			fflush(stdout);
			lcperc = cperc;
			lptime = ptime;
		}
	}

	free(buf);
}

void play_file(const char* path, int dest_fd) {
	//printf("Playing: %s\n", basename(path));
	int source_fd = open(path, O_RDONLY);
	if(source_fd < 0) {
		perror("Could not open");
		return ;
	}

	struct stat stat;
	if(fstat(source_fd, &stat) < 0) {
		perror("Could not stat");
		return;
	}

	playback_loop(stat.st_size, source_fd, dest_fd);
	close(source_fd);
}

int main(int argc, const char** argv) {
	if(argc < 2) {
		fprintf(stderr, "Usage: %s [file]\n", argv[0]);
		return -1;
	}

	int dest_fd = open("/dev/dsp", O_WRONLY);
	if(dest_fd == -1) {
		perror("Could not open device");
		return -1;
	}

	// Raw stdin
	struct termios termios;
	ioctl(0, TCGETS, &termios);
	termios.c_lflag &= ~(ICANON | ECHO);
	ioctl(0, TCSETS, &termios);

	for(int i = 1; i < argc; i++) {
		play_file(argv[i], dest_fd);
	}

	close(dest_fd);
	printf("\n");
	return 0;
}
