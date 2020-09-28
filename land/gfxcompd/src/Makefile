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
	$(CC) $(CFLAGS) -o $@ $^ -lcairo -lpixman-1 -lfreetype -lpng -lz -lm

libxelixgfx.a: libxelixgfx.o
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: install
install:
	install -d $(DESTDIR)/usr/bin/
	install -D gfxcompd $(DESTDIR)/usr/bin/
	install -d $(DESTDIR)/usr/lib/
	install -D libxelixgfx.a $(DESTDIR)/usr/lib/
	install -d $(DESTDIR)/usr/include/
	install -D libxelixgfx.h $(DESTDIR)/usr/include/xelixgfx.h
