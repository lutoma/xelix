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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/stat.h>

#define CHUNK_SIZE (0x800 * 2)

int main(int argc, const char** argv) {
	if(argc < 2) {
		fprintf(stderr, "Usage: %s [file]\n");
		return -1;
	}

	int source_fd = open(argv[1], O_RDONLY);
	if(source_fd < 0) {
		perror("Could not open");
		return -1;
	}

	struct stat stat;
	if(fstat(source_fd, &stat) < 0) {
		perror("Could not stat");
		return -1;
	}
	int dest_fd = open("/dev/ac97", O_WRONLY);
	if(source_fd < 0) {
		perror("Could not open device");
		return -1;
	}

	struct winsize winsize;
	if(ioctl(0, TIOCGWINSZ, &winsize) < 0) {
		perror("Could not get terminal size");
		return -1;
	}

	uint8_t* buf = malloc(CHUNK_SIZE);
	size_t total_written;
	float lcperc = 0;

	while(1) {
		if(!read(source_fd, buf, CHUNK_SIZE)) {
			break;
		}

		total_written += write(dest_fd, buf, CHUNK_SIZE);
		float cperc = (float)total_written / (float)stat.st_size;

		// 44.1khz sample rate, s16le, 2 channels
		//int second = total_written / (44100 * 16 * 2);
		if(cperc != lcperc) {
			printf("\033[G %2.1f%% [\033[47m \033[%db\033[m\033[%dG]", cperc * 100, (int)(cperc * winsize.ws_col), winsize.ws_col);
			fflush(stdout);
			lcperc = cperc;
		}
	}

	free(buf);
	close(source_fd);
	close(dest_fd);
	printf("\n");
	return 0;
}
