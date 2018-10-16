PACKAGE_VERSION := 2.9
PACKAGE_NAME := freetype-$(PACKAGE_VERSION)
PACKAGE_URL := https://download.savannah.gnu.org/releases/freetype/$(PACKAGE_NAME).tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	patch -p0 < ${PACKAGE_NAME}.patch
	cd ./$(PACKAGE_NAME) && CFLAGS="-I$(abspath ../../mnt)/usr/include" LDFLAGS="-L$(abspath ../../mnt)/usr/lib" ./configure --host=i786-pc-xelix --prefix=/usr
	make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
