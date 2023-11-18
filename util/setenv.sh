if test "$#" -ne 1; then
	echo "Usage: source setenv.sh >image root>"
else
export SYSROOT=$1
export PKG_CONFIG_DIR="$SYSROOT"
export PKG_CONFIG_LIBDIR="$SYSROOT/usr/lib/pkgconfig:$SYSROOT/usr/share/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="$SYSROOT"
export CFLAGS="-O3 -I$SYSROOT/usr/include --sysroot $SYSROOT"
export CPPFLAGS="$CFLAGS -D__STDC_ISO_10646__ -D_GLIBCXX_USE_C99_LONG_LONG_DYNAMIC=0 -D_GLIBCXX_USE_C99_STDLIB=0"
export LDFLAGS="-L$SYSROOT/usr/lib"

fix_config_sub() {
	echo "Replacing file $1"
	echo -e "#!/bin/sh\necho i786-pc-xelix"
}
fi
