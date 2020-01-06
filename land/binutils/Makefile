PACKAGE_VERSION := 2.31.1
PACKAGE_NAME := binutils-$(PACKAGE_VERSION)
PACKAGE_URL := https://ftp.gnu.org/gnu/binutils/$(PACKAGE_NAME).tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	patch -p0 -d$(PACKAGE_NAME) < ${PACKAGE_NAME}.patch
	cp xelix_i386.sh $(PACKAGE_NAME)/ld/emulparams/
	cd ./$(PACKAGE_NAME) && ./configure --host=i786-pc-xelix --target=i786-pc-xelix --prefix=/usr --disable-werror --disable-plugins
	make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
