PACKAGE_VERSION := 5.0
PACKAGE_NAME := bash-$(PACKAGE_VERSION)
PACKAGE_URL := https://ftp.gnu.org/gnu/bash/$(PACKAGE_NAME).tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	patch -p0 < ${PACKAGE_NAME}.patch
	cd ./$(PACKAGE_NAME) && ./configure --host=i786-pc-xelix --prefix=/usr
	make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
