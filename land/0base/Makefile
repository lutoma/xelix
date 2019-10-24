.PHONY: all
all:
	true

.PHONY: install
install:
	install -d ../../mnt/home
	install -d ../../mnt/home/root
	install -d ../../mnt/dev
	install -d ../../mnt/etc
	install -d ../../mnt/sys
	install -d ../../mnt/tmp
	install -d ../../mnt/usr
	install -d ../../mnt/usr/bin
	install -d ../../mnt/usr/share
	install -d ../../mnt/var
	install -d ../../mnt/var/run
	ln -s home/root ../../mnt/root
	ln -s usr/bin ../../mnt/bin

	echo 'root:x:0:0:root:/home/root:/usr/bin/dash' > ../../mnt/etc/passwd
	echo 'root:x:0:root' > ../../mnt/etc/group
	echo 'localhost.localdomain' > ../../mnt/etc/hostname
	echo -e '/usr/bin/bash\n/usr/bin/zsh' > ../../etc/shells
	cp motd ../../mnt/etc/
	ln -s /sys/mounted ../../mnt/etc/mtab
