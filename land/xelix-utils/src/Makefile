CC := i786-pc-xelix-gcc
LD := i786-pc-xelix-ld
CFLAGS += -std=gnu18 -O3 -ggdb -D_GNU_SOURCE
DESTDIR ?= ../../../mnt

TARGETS=basictest init ps uptime free login dmesg su play strace host telnetd mount umount gfxterm png

.PHONY: all
all: $(TARGETS) ld-xelix.so

.PHONY: clean
clean:
	rm -f $(TARGETS) util.o libutil.a basictest.o ld-xelix.so ld-xelix.o ld-xelix-i386.o

init: init.c ini.c libutil.a
	$(CC) $(CFLAGS) -o $@ init.c ini.c -L. -lutil

play: play.c
	$(CC) $(CFLAGS) -o $@ play.c -L. -lFLAC -lm -lutil

gfxterm: gfxterm.c tmt.c tmt.h
	$(CC) $(CFLAGS) -o $@ tmt.c gfxterm.c -lxelixgfx -lfreetype -lpng -lz -lm -lbz2

png: png.c
	$(CC) $(CFLAGS) -o $@ png.c -lxelixgfx -lpng -lz -lm

ld-xelix.so: ld-xelix.c ld-xelix-i386.asm ld-xelix.ld
	$(CC) $(CFLAGS) -o ld-xelix.o -c ld-xelix.c
	nasm -g -f elf ld-xelix-i386.asm -o ld-xelix-i386.o
	$(CC) $(CFLAGS) -o $@ ld-xelix.o ld-xelix-i386.o -static -T ld-xelix.ld

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
	install -d $(DESTDIR)/usr/lib/
	install -D $(TARGETS) $(DESTDIR)/usr/bin/
	install -D ld-xelix.so $(DESTDIR)/usr/lib/
	chmod u+s $(DESTDIR)/usr/bin/su
