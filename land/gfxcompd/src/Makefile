PATH += :../../toolchain/local/bin/
CC := i786-pc-xelix-gcc
LD := i786-pc-xelix-ld
CFLAGS += -std=gnu18 -fcommon -O3 -g -D_GNU_SOURCE -I$(DESTDIR)/usr/include/freetype2
DESTDIR ?= ../../../mnt

.PHONY: all
all: gfxcompd libxelixgfx.a

.PHONY: clean
clean:
	rm -f gfxcompd

gfxcompd: main.o mouse.o window.o bus.o text.o render.o bar.o
	$(CC) $(CFLAGS) -o $@ $^ -lcairo -lpixman-1 -lfreetype -lpng -lz -lm -lbz2

libxelixgfx.a: libxelixgfx.o
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: install
install:
	install -d $(DESTDIR)/usr/bin/
	install -D gfxcompd $(DESTDIR)/usr/bin/
	install -d $(DESTDIR)/usr/lib/
	install -m 644 -D libxelixgfx.a $(DESTDIR)/usr/lib/
	install -d $(DESTDIR)/usr/include/
	install -m 644 -D libxelixgfx.h $(DESTDIR)/usr/include/xelixgfx.h
	install -d $(DESTDIR)/usr/share/icons
	install -m 644 -D view-app-grid-symbolic.png $(DESTDIR)/usr/share/icons/view-app-grid-symbolic.png
	install -d $(DESTDIR)/usr/share/gfxcompd
	install -m 644 -D cursor.png $(DESTDIR)/usr/share/gfxcompd
	install -m 644 -D bg.png $(DESTDIR)/usr/share/gfxcompd
