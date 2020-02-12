/* Copyright © 2019-2020 Lukas Martini
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
#include <errno.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <FLAC/stream_decoder.h>
#include "argparse.h"

uint16_t* interleaved_buf = NULL;
size_t interleaved_buf_size = 0;
int dest_fd = -1;
int quiet = 0;
uint64_t cur_sample = 0;
FLAC__StreamDecoder* decoder = NULL;
const FLAC__StreamMetadata* metadata = NULL;
struct winsize winsize;

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

void metadata_callback(const FLAC__StreamDecoder *decoder,
	const FLAC__StreamMetadata *_metadata, void *client_data) {

	if(_metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		metadata = _metadata;

		if(!quiet) {
			printf("%u channels, %2.1f kHz, %u bps, %llu samples\n",
				metadata->data.stream_info.channels,
				(float)metadata->data.stream_info.sample_rate / 1000,
				metadata->data.stream_info.bits_per_sample,
				metadata->data.stream_info.total_samples);
			fflush(stdout);
		}
	}
}

void draw_osd(int cur_sample) {
	uint32_t elapsed = (uint32_t)(cur_sample / metadata->data.stream_info.sample_rate);
	uint32_t length = (uint32_t)(metadata->data.stream_info.total_samples /
		metadata->data.stream_info.sample_rate);

	float perc = (float)cur_sample / (float)metadata->data.stream_info.total_samples;

	printf("\033[2K\033[G");
	int status_len = printf(" %llu:%02u / %u:%02u (%2.1f%%) ",
		elapsed / 60, elapsed % 60, length / 60, length % 60, perc * 100);

	int bar_size = winsize.ws_col - status_len - 6;
	int bar_prog = bar_size * perc;

	putchar('[');
	for(int i = 0; i < bar_size; i++) {
		if(i == bar_prog) {
			putchar('>');
		} else if(i <= bar_prog) {
			putchar('=');
		} else {
			putchar(' ');
		}
	}
	putchar(']');
	fflush(stdout);
}

FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder,
	const FLAC__Frame *frame, const FLAC__int32* const buffer[], void *client_data) {

	// Handle input
	char input = poll_input(0);
	if(input) {
		fflush(stdout);
		switch(input) {
			case 'q':
				exit(0);
			case ' ':
				while(poll_input(-1) != ' ');
				break;
		}
		fflush(stdin);
	}

	// Simple OSD
	if(!quiet && metadata) {
		cur_sample += frame->header.blocksize;
		draw_osd(cur_sample);
	}

	size_t bsize = sizeof(uint16_t) * frame->header.blocksize * 2;
	if(bsize > interleaved_buf_size) {
		if(!interleaved_buf) {
			interleaved_buf = malloc(bsize);
		} else {
			interleaved_buf = realloc(interleaved_buf, bsize);
		}

		interleaved_buf_size = bsize;
	}

	int j = 0;
	for(int i = 0; i < frame->header.blocksize; i++) {
		interleaved_buf[i*2] = buffer[0][i];
		interleaved_buf[i*2 + 1] = buffer[1][i];
	}

	size_t sz = frame->header.blocksize * 4;
	size_t nw = 0;
	while(nw < sz) {
		nw += write((int)client_data, (void*)interleaved_buf + nw, sz - nw);
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void error_callback(const FLAC__StreamDecoder *decoder,
	FLAC__StreamDecoderErrorStatus status, void *client_data) {

	fprintf(stderr, "Decode error: %s\n",
		FLAC__StreamDecoderErrorStatusString[status]);
	exit(-1);
}

void play_file(FLAC__StreamDecoder* decoder, const char* path, int dest_fd) {
	if(!quiet) {
		printf("Playing: %s\n", basename(path));
		fflush(stdout);
	}

	cur_sample = 0;

	FLAC__StreamDecoderInitStatus dc_status = FLAC__stream_decoder_init_file(
		decoder, path, write_callback, metadata_callback,
		error_callback, (void*)dest_fd);

	if(dc_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		fprintf(stderr, "Could not initialize decoder: %s\n",
			FLAC__StreamDecoderInitStatusString[dc_status]);
		return;
	}

	if(!FLAC__stream_decoder_process_until_end_of_stream(decoder)) {
		fprintf(stderr, "Decoding failed: %s\n",
			FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(decoder)]);
		return;
	}

	FLAC__stream_decoder_finish(decoder);
}

void cleanup() {
	if(decoder) {
		FLAC__stream_decoder_finish(decoder);
		FLAC__stream_decoder_delete(decoder);
	}

	if(dest_fd != -1) {
		close(dest_fd);
		printf("\n");
	}

	// Reset stdin
	struct termios termios;
	ioctl(0, TCGETS, &termios);
	termios.c_lflag |= ICANON | ECHO;
	ioctl(0, TCSETS, &termios);
}

static const char *const usage[] = {
    "play [options] <file1> [<file2>, ...]",
    NULL,
};

int main(int argc, const char** argv) {
	const char* path = "/dev/dsp1";
	struct argparse_option options[] = {
		OPT_BOOLEAN('h', "help", NULL, "show this help message and exit", argparse_help_cb, 0, OPT_NONEG),
		OPT_STRING('d', "device", &path, "audio device to use"),
		OPT_BOOLEAN('q', "quiet", &quiet, "don't write status to stdout while running"),
        OPT_END(),
	};

    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse, "\nPlay flac audio files.",
    	"\nplay is a simple player for FLAC files using libFLAC.\n"
    	"By default, it uses /dev/dsp1 as output device.\n"
    	"play is part of xelix-utils. Please report bugs to <hello@lutoma.org>.");
    argc = argparse_parse(&argparse, argc, argv);

    if(argc < 1) {
    	argparse_usage(&argparse);
    	exit(1);
    }

	atexit(cleanup);
	dest_fd = open(path, O_WRONLY);
	if(dest_fd < 0) {
		fprintf(stderr, "Could not open %s: %s\n", path, strerror(errno));
		return 1;
	}

	decoder = FLAC__stream_decoder_new();
	if(!decoder) {
		perror("Could not allocate decoder\n");
		return 1;
	}

	if(!quiet) {
		// Raw stdin
		struct termios termios;
		ioctl(0, TCGETS, &termios);
		termios.c_lflag &= ~(ICANON | ECHO);
		termios.c_lflag |= ISIG;
		ioctl(0, TCSETS, &termios);

		// Get terminal size
		if(ioctl(0, TIOCGWINSZ, &winsize) < 0) {
			perror("Could not get terminal dimensions");
			return 1;
		}
	}

	for(int i = 0; i < argc; i++) {
		if(i) {
			printf("\n\n");
		}

		play_file(decoder, argv[i], dest_fd);
	}

	return 0;
}
