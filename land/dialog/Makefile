PACKAGE_VERSION := 1.3-20191210
PACKAGE_NAME := dialog-$(PACKAGE_VERSION)
PACKAGE_URL := https://invisible-island.net/datafiles/release/dialog.tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	patch -p0 < ${PACKAGE_NAME}.patch
	cd $(PACKAGE_NAME) && CFLAGS="-L$(abspath ../../mnt/usr/lib) -I$(abspath ../../mnt/usr/include)" ./configure --host=i786-pc-xelix --disable-shared --enable-static --prefix=/usr
	make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
