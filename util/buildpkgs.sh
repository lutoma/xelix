#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -P "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPT_DIR/setenv.sh "/tmp/image"

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

	sudo pacman --root "$SYSROOT" --noconfirm -Sy "${need_packages[@]}"
	cd $dir
	makepkg -Ad --sign
	mv *.pkg.tar* /tmp/packages
	cd ..
done
