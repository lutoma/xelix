CC := ../../toolchain/local/bin/i786-pc-xelix-gcc
CFLAGS += -std=gnu18 -O2 -g

TARGETS=xinit xshell cat ls pwd cd play malloctest basictest checkregs

all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS)

%: %.asm
	nasm -g -f elf -o $@.o $<
	$(CC) -nostdlib -o $@ $@.o

.PHONY: install
install:
	install -d ../../mnt/bin/
	install -D $(TARGETS) ../../mnt/bin/
