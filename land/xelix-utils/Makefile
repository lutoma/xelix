PATH += :../../toolchain/local/bin/
CC := i786-pc-xelix-gcc
CFLAGS += -std=gnu18 -O3 -g -D_GNU_SOURCE

TARGETS=xinit xshell cat ls cd malloctest basictest checkregs ps uptime

all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS)

%: %.asm
	nasm -g -f elf -o $@.o $<
	$(CC) -nostdlib -o $@ $@.o

.PHONY: install
install:
	install -d ../../mnt/usr/bin/
	install -D $(TARGETS) ../../mnt/usr/bin/
