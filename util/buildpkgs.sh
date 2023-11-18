#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -P "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source $SCRIPT_DIR/setenv.sh "/tmp/image"

mkdir /tmp/packages || true

for dir in "$@"; do
	makedepend=()
	depend=()
	source "$dir/PKGBUILD"

	echo -e "Building $pkgname-$pkgver: $pkgdesc\n"

	sudo rm -rf "$SYSROOT"
	mkdir -p "$SYSROOT/var/lib/pacman"

	need_packages=('base' ${depend[@]} ${makedepend[@]})
	sudo pacman --root "$SYSROOT" --noconfirm -Sy "${need_packages[@]}"
	sudo rm -vf "$SYSROOT"/usr/lib/*.la

	cd $dir
	makepkg -Ad --sign
	mv *.pkg.tar* /tmp/packages
	cd ..
done
