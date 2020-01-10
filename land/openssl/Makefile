PACKAGE_VERSION := 1.1.1d
PACKAGE_NAME := openssl-$(PACKAGE_VERSION)
PACKAGE_URL := https://www.openssl.org/source/$(PACKAGE_NAME).tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	patch -p0 < ${PACKAGE_NAME}.patch
	cd ./$(PACKAGE_NAME) && CROSS_COMPILE=i786-pc-xelix- ./Configure xelix-x86 no-shared no-threads no-hw no-dso no-asm 386 no-sse2 no-zlib no-tests --prefix=/usr
	make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
