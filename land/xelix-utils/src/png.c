#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <libgen.h>
#include <xelixgfx.h>
#include <png.h>
#include "util.h"
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <sys/param.h>


int width, height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;

struct gfx_window window;

#define abort_(args...) do { fprintf(stderr, args); abort(); } while(0)


void read_png_file(char* file_name)
{
		char header[8];    // 8 is the maximum size that can be checked

		/* open file and test for it being a png */
		FILE *fp = fopen(file_name, "rb");
		if (!fp)
				abort_("[read_png_file] File %s could not be opened for reading", file_name);
		fread(header, 1, 8, fp);
		if (png_sig_cmp(header, 0, 8))
				abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


		/* initialize stuff */
		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

		if (!png_ptr)
				abort_("[read_png_file] png_create_read_struct failed");

		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
				abort_("[read_png_file] png_create_info_struct failed");

		if (setjmp(png_jmpbuf(png_ptr)))
				abort_("[read_png_file] Error during init_io");

		png_init_io(png_ptr, fp);
		png_set_sig_bytes(png_ptr, 8);

		png_read_info(png_ptr, info_ptr);

		width = png_get_image_width(png_ptr, info_ptr);
		height = png_get_image_height(png_ptr, info_ptr);
		color_type = png_get_color_type(png_ptr, info_ptr);
		bit_depth = png_get_bit_depth(png_ptr, info_ptr);

		number_of_passes = png_set_interlace_handling(png_ptr);
		png_read_update_info(png_ptr, info_ptr);


		/* read file */
		if (setjmp(png_jmpbuf(png_ptr)))
				abort_("[read_png_file] Error during read_image");

		row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
		for (int y = 0; y<height; y++)
				row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

		png_read_image(png_ptr, row_pointers);

		fclose(fp);
}

int main(int argc, char* argv[]) {
	if(argc < 2) {
		fprintf(stderr, "Usage: %s <image>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	read_png_file(argv[1]);

	if(gfx_open() < 0) {
		perror("Could not initialize gfx handle");
		exit(EXIT_FAILURE);
	}

	char title[1024];
	snprintf(title, 1024, "%s - Image Viewer",  basename(argv[1]));

	if(gfx_window_new(&window, title, 800, 600) < 0) {
		perror("Could not initialize gfx handle");
		exit(EXIT_FAILURE);
	}

	if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB) {
		int i = 0;

		for(int y = 0; y < MIN(window.height, height); y++) {
				png_byte* row = row_pointers[y];

				for (int x = 0; x < MIN(window.width, width); x++) {
						png_byte* ptr = &(row[x*3]);
						window.addr[i] = 0xff << 24 | ptr[0] << 16 | ptr[1] << 8 | ptr[2];
						i++;

				}
		}
	}

	gfx_window_blit_full(&window);
}
