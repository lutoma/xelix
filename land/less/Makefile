PACKAGE_VERSION := 551
PACKAGE_NAME := less-$(PACKAGE_VERSION)
PACKAGE_URL := http://www.greenwoodsoftware.com/less/$(PACKAGE_NAME).tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	cd $(PACKAGE_NAME) && ./configure --host=i786-pc-xelix --prefix=/usr
	make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
