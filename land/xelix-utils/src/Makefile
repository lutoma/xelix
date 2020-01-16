PATH += :../../toolchain/local/bin/
CC := i786-pc-xelix-gcc
LD := i786-pc-xelix-ld
CFLAGS += -std=gnu18 -O3 -g -D_GNU_SOURCE
DESTDIR ?= ../../mnt

TARGETS=basictest init ps uptime free login dmesg su play strace host telnetd mount umount

all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS) util.o libutil.a basictest.o

init: init.c ini.c libutil.a
	$(CC) $(CFLAGS) -o $@ init.c ini.c -L. -lutil

play: play.c
	$(CC) $(CFLAGS) -o $@ play.c -lFLAC -lm

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
	install -d $(DESTDIR)/etc/init.d/
	install -D $(TARGETS) $(DESTDIR)/usr/bin/
	install -D tty1-login $(DESTDIR)/etc/init.d
	chmod u+s $(DESTDIR)/usr/bin/su
