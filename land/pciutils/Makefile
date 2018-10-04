PACKAGE_VERSION := 3.6.2
PACKAGE_NAME := pciutils-$(PACKAGE_VERSION)
PACKAGE_URL := https://github.com/pciutils/pciutils/archive/v$(PACKAGE_VERSION).tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	cp xelix-device.c $(PACKAGE_NAME)/lib
	patch -p0 < ${PACKAGE_NAME}.patch
	make CROSS_COMPILE=i786-pc-xelix- HOST=pc-xelix DNS=no ZLIB=no PREFIX=/usr SBINDIR=/usr/bin DESTDIR=../../../mnt -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: install
install:
	make CROSS_COMPILE=i786-pc-xelix- HOST=pc-xelix DNS=no ZLIB=no PREFIX=/usr SBINDIR=/usr/bin DESTDIR=../../../mnt -C $(PACKAGE_NAME) install
