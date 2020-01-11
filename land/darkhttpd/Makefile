PACKAGE_VERSION := 1.12
PACKAGE_NAME := darkhttpd-$(PACKAGE_VERSION)
PACKAGE_URL := https://unix4lyfe.org/darkhttpd/$(PACKAGE_NAME).tar.bz2

.PHONY: all
all: $(PACKAGE_NAME).tar.bz2
	tar xf $(PACKAGE_NAME).tar.bz2
	patch -p0 < ${PACKAGE_NAME}.patch
	CC=i786-pc-xelix-gcc CFLAGS="-O3 -DNO_IPV6" make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.bz2:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.bz2

.PHONY: install
install:
	cp ./$(PACKAGE_NAME)/darkhttpd $(abspath ../../mnt/usr/bin)
