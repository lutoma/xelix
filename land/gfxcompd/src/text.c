#include <stdlib.h>
#include <unistd.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>

#include "util.h"
#include "window.h"
#include "text.h"

FT_Library ft_library;

static void load_font(cairo_font_face_t** fdesc, const char* path) {
	static FT_Face ft_font;
	if(FT_New_Face(ft_library, path, 0, &ft_font)) {
		printf("Font %s: ", path);
		perror("Could not read font");
		exit(EXIT_FAILURE);
	}

	static const cairo_user_data_key_t key;
	*fdesc = cairo_ft_font_face_create_for_ft_face(ft_font, 0);
	if(cairo_font_face_set_user_data(*fdesc, &key,
		ft_font, (cairo_destroy_func_t)FT_Done_Face)) {

		cairo_font_face_destroy(*fdesc);
		FT_Done_Face(ft_font);
		exit(EXIT_FAILURE);
	}
}

void text_init() {
	if(FT_Init_FreeType(&ft_library)) {
		perror("Could not initialize freetype");
		exit(EXIT_FAILURE);
	}

	load_font(&font, "/usr/share/fonts/Roboto-Regular.ttf");
	load_font(&font_light, "/usr/share/fonts/Roboto-Light.ttf");
	load_font(&font_bold, "/usr/share/fonts/Roboto-Bold.ttf");
}
