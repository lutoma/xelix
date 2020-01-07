PACKAGE_VERSION := 4.0.2
PACKAGE_NAME := mpfr-$(PACKAGE_VERSION)
PACKAGE_URL := https://www.mpfr.org/mpfr-current/$(PACKAGE_NAME).tar.xz

.PHONY: all
all: $(PACKAGE_NAME).tar.xz
	tar xf $(PACKAGE_NAME).tar.xz
	patch -p0 < ${PACKAGE_NAME}.patch
	cd ./$(PACKAGE_NAME) && ./configure --host=i786-pc-xelix --target=i786-pc-xelix --prefix=/usr --disable-shared --with-gmp-build=$(abspath ../gmp/gmp-6.1.2)
	make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.xz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.xz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
