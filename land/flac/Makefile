PACKAGE_VERSION := 1.3.3
PACKAGE_NAME := flac-$(PACKAGE_VERSION)
PACKAGE_URL := https://ftp.osuosl.org/pub/xiph/releases/flac/$(PACKAGE_NAME).tar.xz

.PHONY: all
all: $(PACKAGE_NAME).tar.xz
	tar xf $(PACKAGE_NAME).tar.xz
	patch -p0 < ${PACKAGE_NAME}.patch
	cd $(PACKAGE_NAME) && CPPFLAGS="-D_GLIBCXX_USE_C99_LONG_LONG_DYNAMIC=0 -D_GLIBCXX_USE_C99_STDLIB=0 -mno-sse" CFLAGS="-mno-sse" ./configure --host=i786-pc-xelix --prefix=/usr --disable-shared --disable-sse --disable-asm-optimizations --without-ogg
	make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.xz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.xz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install

