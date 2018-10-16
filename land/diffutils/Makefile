PACKAGE_VERSION := 3.6
PACKAGE_NAME := diffutils-$(PACKAGE_VERSION)
PACKAGE_URL := https://ftp.gnu.org/gnu/diffutils/$(PACKAGE_NAME).tar.xz

.PHONY: all
all: $(PACKAGE_NAME).tar.xz
	tar xf $(PACKAGE_NAME).tar.xz
	patch -p0 < ${PACKAGE_NAME}.patch
	cd ./$(PACKAGE_NAME) && ./configure --host=i786-pc-xelix --prefix=/usr
	make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.xz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.xz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
