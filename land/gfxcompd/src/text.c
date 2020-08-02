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

void text_init() {
	if(FT_Init_FreeType(&ft_library)) {
		perror("Could not initialize freetype");
		exit(EXIT_FAILURE);
	}

	static FT_Face ft_font;
	if(FT_New_Face(ft_library, "/usr/share/fonts/Roboto-Light.ttf", 0, &ft_font)) {
		perror("Could not read font");
		exit(EXIT_FAILURE);
	}
/*
	if(FT_Set_Char_Size(font_regular, 600, 0, 100, 0 )) {
		perror("Could not set char size");
		exit(EXIT_FAILURE);
	}
*/

	static const cairo_user_data_key_t key;

	font = cairo_ft_font_face_create_for_ft_face(ft_font, 0);
	if(cairo_font_face_set_user_data(font, &key,
		ft_font, (cairo_destroy_func_t)FT_Done_Face)) {

		cairo_font_face_destroy(font);
		FT_Done_Face(ft_font);
		exit(EXIT_FAILURE);
	}
}
