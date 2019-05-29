PACKAGE_VERSION := 6.1
PACKAGE_NAME := ncurses-$(PACKAGE_VERSION)
PACKAGE_URL := ftp://ftp.gnu.org/gnu/ncurses/$(PACKAGE_NAME).tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	patch -p0 < ${PACKAGE_NAME}.patch
	cd ./$(PACKAGE_NAME) && ./configure --host=i786-pc-xelix --prefix=/usr --without-cxx-binding
	make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
	tic -o $(abspath ../../mnt/usr/share/terminfo) xelix.terminfo
