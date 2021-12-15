# GUI development

!!! note
	The graphical interface is still highly experimental, prone to crash and the API is incomplete and subject to (many) future changes.

The graphical interface of xelix is handled by `gfxcompd` (Graphics compositor daemon), which lives in `land/gfxcompd` in the source tree. It uses [FreeType](https://freetype.org/) for text rendering and [Cairo](https://www.cairographics.org/) for general drawing and compositing.

It currently exposes a very simple API though `libxelixgfx` / `xelixgfx.h` that can be used to create windows and draw arbitrary pixels inside them. There is currently no support for input events.

## libxelixgfx reference

### gfx_open

```c
int gfx_open();
```

Opens a communication channel to gfxcompd though `/dev/gfxbus` and creates a shared memory region for the framebuffer. Returns 0 on success, a negative integer on failure.

### gfx_close

```c
int gfx_close();
```

Closes the communication channel and framebuffer. Returns 0 on success, a negative integer on failure.

### struct gfx_window

```c
struct gfx_window {
	uint32_t* addr;
	uint32_t bpp;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t size;
	uint32_t wid;
};
```

The primary representation of a GUI window.

addr
:   A pointer to an [ARGB32](https://en.wikipedia.org/wiki/RGBA_color_model#ARGB32) framebuffer of `size` 32-bit words – one for each pixel. Note that changes to this framebuffer will not be displayed until `gfx_window_blit` or `gfx_window_blit_full` are called.

bpp
:   Bits per pixel - Currently always 32.

width, height
:   Width and height of the framebuffer/paintable window area in pixels.

pitch
:   Bytes required to advance the y position by one, i.e. the width of the framebuffer in memory. This may differ from `width` for better memory alignment.

size
:   Total framebuffer size in bytes. This is equal to `pitch * (height * bpp/4)`.

wid
:   Window ID

### gfx_window_new

```c
int gfx_window_new(struct gfx_window* win, const char* title, size_t width, size_t height);
```

Creates a new window and initializes the `struct gfx_window` in `win`. `title` will be used as the window title displayed to the user, `width` and `height` are a window size request. Returns 0 on success, a negative integer on failure.

### gfx_window_blit

```c
int gfx_window_blit(struct gfx_window* win, size_t x, size_t y, size_t width, size_t height);
```

Draws the area of the framebuffer starting at `x, y` and with size `width, height` to the screen.

### gfx_window_blit_full

```c
int gfx_window_blit_full(struct gfx_window* win);
```

Draws the entire framebuffer to the screen.


## Sample program


```c
#include <stdlib.h>
#include <xelixgfx.h>

struct gfx_window window;

int main() {
	if(gfx_open() < 0) {
		perror("Could not initialize gfx handle");
		exit(EXIT_FAILURE);
	}

	if(gfx_window_new(&window, "Example program", 800, 600) < 0) {
		perror("Could not initialize gfx window");
		exit(EXIT_FAILURE);
	}

	while(1) {
		do_stuff();

		// Put some pixels in the framebuffer
		memset32(window.addr, 0xffffffff, window.size / 4);

		// Blit framebuffer
		gfx_window_blit_full(&window);
	}

	gfx_close();
}
```

Compile with `i786-pc-xelix-gcc -o testgui testgui.c -lxelixgfx -lm`
