PACKAGE_VERSION := 8.1
PACKAGE_NAME := vim-$(PACKAGE_VERSION)
PACKAGE_URL := ftp://ftp.vim.org/pub/vim/unix/$(PACKAGE_NAME).tar.bz2

.PHONY: all
all: $(PACKAGE_NAME).tar.bz2
	tar xf $(PACKAGE_NAME).tar.bz2

	cd ./vim81 && ac_cv_sizeof_int=4 \
	vim_cv_getcwd_broken=no \
	vim_cv_memmove_handles_overlap=yes \
	vim_cv_stat_ignores_slash=yes \
	vim_cv_tgetent=zero \
	vim_cv_terminfo=yes \
	vim_cv_toupper_broken=no \
	vim_cv_tty_group=world \
	LDFLAGS="-L$(abspath ../../mnt/usr/lib)" \
	CFLAGS="-I$(abspath ../../mnt/usr/include) -L$(abspath ../../mnt/usr/lib)" \
	./configure \
		--build=x86_64-linux \
		--host=i786-pc-xelix \
		--target=i786-pc-xelix \
		--with-tlib=ncurses \
		--prefix=/usr

	cd ./vim81/src && sh osdef.sh
	make -C vim81

$(PACKAGE_NAME).tar.bz2:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.bz2

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C vim81 install
	sudo ln -s vim $(abspath ../../mnt/usr/bin/vi)
