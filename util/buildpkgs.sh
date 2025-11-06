#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -P "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LAND_DIR="$SCRIPT_DIR/../land"
source $SCRIPT_DIR/setenv.sh "/tmp/image"

mkdir /tmp/packages || true

for dir in "$@"; do
	# Create a new empty sysroot
	sudo rm -rf "$SYSROOT"
	mkdir -p "$SYSROOT/var/lib/pacman"

	makedepend=()
	depend=()
	source "$LAND_DIR/$dir/PKGBUILD"
	echo -e "Building $pkgname-$pkgver: $pkgdesc\n"

	# Install package build dependencies to the sysroot
	need_packages=('base' ${depend[@]} ${makedepend[@]})
	sudo pacman --config "$LAND_DIR/pacman/pacman.linux.conf" --root "$SYSROOT" --noconfirm -Sy "${need_packages[@]}"
	sudo rm -vf "$SYSROOT"/usr/lib/*.la

	# Build package
	cd "$LAND_DIR/$dir"
	makepkg -fAd --sign
	mv *.pkg.* /tmp/packages
	cd ..
done
