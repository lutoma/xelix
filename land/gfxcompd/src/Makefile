PATH += :../../toolchain/local/bin/
CC := i786-pc-xelix-gcc
LD := i786-pc-xelix-ld
CFLAGS += -std=gnu18 -O3 -g -D_GNU_SOURCE -I$(DESTDIR)/usr/include/freetype2
DESTDIR ?= ../../../mnt

.PHONY: all
all: gfxcompd

.PHONY: clean
clean:
	rm -f gfxcompd

gfxcompd: main.o mouse.o window.o bus.o text.o render.o
	$(CC) $(CFLAGS) -o $@ $^ -lcairo -lpixman-1 -lfreetype -lpng -lz -lm

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: install
install:
	install -d $(DESTDIR)/usr/bin/
	install -D gfxcompd $(DESTDIR)/usr/bin/
