#!/usr/bin/env bash
set -euo pipefail

source setenv.sh "/tmp/image"

mkdir /tmp/packages || true

for dir in "$@"; do
	source "$dir/PKGBUILD"
	echo -e "Building $pkgname-$pkgver: $pkgdesc\n"

	sudo rm -rf "$SYSROOT"
	mkdir -p "$SYSROOT/var/lib/pacman"

	need_packages=('base')

	if ! [ -z ${depend+x} ]; then
		need_packages+=(${depend[@]})
	fi

	if ! [ -z ${makedepend+x} ]; then
		need_packages+=(${makedepend[@]})
	fi


	echo "makedepend: ${makedepend[@]}"
	sudo pacman --root "$SYSROOT" --noconfirm -Sy "${need_packages[@]}"
	cd $dir
	makepkg -Ad
	mv *.pkg.tar* /tmp/packages
	cd ..
	# --sign
done
