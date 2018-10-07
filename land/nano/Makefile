PACKAGE_VERSION := 3.1
PACKAGE_NAME := nano-$(PACKAGE_VERSION)
PACKAGE_URL := https://www.nano-editor.org/dist/v3/$(PACKAGE_NAME).tar.xz

.PHONY: all
all: $(PACKAGE_NAME).tar.xz
	tar xf $(PACKAGE_NAME).tar.xz
	patch -p0 < ${PACKAGE_NAME}.patch
	cd ./$(PACKAGE_NAME) && NCURSESW_LIBS="-lncurses" ./configure --host=i786-pc-xelix --prefix=/usr --disable-largefile --disable-threads --disable-speller --disable-mouse
	make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.xz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.xz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
