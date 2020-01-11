PACKAGE_VERSION := 8.30
PACKAGE_NAME := coreutils-$(PACKAGE_VERSION)
PACKAGE_URL := https://ftp.gnu.org/gnu/coreutils/$(PACKAGE_NAME).tar.xz


.PHONY: all
all: $(PACKAGE_NAME).tar.xz
	tar xf $(PACKAGE_NAME).tar.xz
	patch -p0 < ${PACKAGE_NAME}.patch
	cd ./$(PACKAGE_NAME) && autoreconf
	cd ./$(PACKAGE_NAME) && ./configure --host=i786-pc-xelix --prefix=/usr --disable-threads --disable-acl --disable-xattr --disable-largefile --enable-no-install-program=arch,coreutils,dmesg,mknod
	patch -p0 < ${PACKAGE_NAME}-patch-two.patch
	make -C $(PACKAGE_NAME)

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install

$(PACKAGE_NAME).tar.xz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.xz
