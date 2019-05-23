PACKAGE_VERSION := 4.2
PACKAGE_NAME := nano-$(PACKAGE_VERSION)
PACKAGE_URL := https://www.nano-editor.org/dist/v4/$(PACKAGE_NAME).tar.xz

.PHONY: all
all: $(PACKAGE_NAME).tar.xz
	tar xf $(PACKAGE_NAME).tar.xz
	patch -p0 < ${PACKAGE_NAME}.patch
	cd ./$(PACKAGE_NAME) && CFLAGS="-I/home/lutoma/code/xelix/mnt/usr/include -L/home/lutoma/code/xelix/mnt/usr/lib" NCURSESW_LIBS="-lncurses" ./configure --host=i786-pc-xelix --prefix=/usr --disable-largefile --disable-threads --disable-speller --disable-mouse --disable-utf8
	make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.xz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.xz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
