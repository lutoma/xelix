PATH += :../../toolchain/local/bin/
CC := i786-pc-xelix-gcc
LD := i786-pc-xelix-ld
CFLAGS += -std=gnu18 -O3 -g -D_GNU_SOURCE
DESTDIR ?= ../../../mnt

TARGETS=basictest init ps uptime free login dmesg su play strace host telnetd mount umount gfxterm png

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS) util.o libutil.a basictest.o

init: init.c ini.c libutil.a
	$(CC) $(CFLAGS) -o $@ init.c ini.c -L. -lutil

play: play.c
	$(CC) $(CFLAGS) -o $@ play.c -L. -lFLAC -lm -lutil

gfxterm: gfxterm.c tmt.c tmt.h
	$(CC) $(CFLAGS) -o $@ tmt.c gfxterm.c -I$(DESTDIR)/usr/include -I$(DESTDIR)/usr/include/freetype2 -lxelixgfx -lfreetype -lpng -lz -lm -lbz2

png: png.c
	$(CC) $(CFLAGS) -o $@ png.c -lxelixgfx -lpng -lz -lm

libutil.a: util.c argparse.c
	$(CC) $(CFLAGS) -c -o util.o util.c
	$(CC) $(CFLAGS) -c -o argparse.o argparse.c
	ar rcs $@ util.o argparse.o

%: %.asm
	nasm -g -f elf -o $@.o $<
	$(LD) -nostdlib -o $@ $@.o

%: %.c libutil.a
	$(CC) $(CFLAGS) -o $@ $@.c -L. -lutil

.PHONY: install
install:
	install -d $(DESTDIR)/usr/bin/
	install -D $(TARGETS) $(DESTDIR)/usr/bin/
	chmod u+s $(DESTDIR)/usr/bin/su
