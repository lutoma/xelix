PACKAGE_VERSION := 1.2.11
PACKAGE_NAME := zlib-$(PACKAGE_VERSION)
PACKAGE_URL := https://zlib.net/$(PACKAGE_NAME).tar.xz

.PHONY: all
all: $(PACKAGE_NAME).tar.xz
	tar xf $(PACKAGE_NAME).tar.xz
	cd ./$(PACKAGE_NAME) && CC=i786-pc-xelix-gcc ./configure --static --prefix=/usr
	make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.xz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.xz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
