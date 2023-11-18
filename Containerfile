FROM alpine:latest
LABEL maintainer="hello@lutoma.org"

ARG TARGET=i786-pc-xelix

ARG BINUTILS_VERSION=2.40
ARG BINUTILS_URL=https://ftp.gnu.org/gnu/binutils
ARG BINUTILS_PACKAGE=binutils-${BINUTILS_VERSION}.tar.xz

ARG GCC_VERSION=13.1.0
ARG GCC_URL=https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}
ARG GCC_PACKAGE=gcc-${GCC_VERSION}.tar.xz

ARG NEWLIB_VERSION=3.2.0
ARG NEWLIB_URL=https://sourceware.org/pub/newlib
ARG NEWLIB_PACKAGE=newlib-${NEWLIB_VERSION}.tar.gz

ENV PATH="/toolchain/usr/bin:${PATH}"

RUN apk --no-cache add wget musl-dev make gcc g++ m4 perl autoconf automake \
	patch libtool mpc1-dev gmp-dev mpfr-dev gawk texinfo file

# Build outdated autoconf and automake versions for newlib
WORKDIR /usr/src
RUN wget https://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz
RUN wget https://ftp.gnu.org/gnu/automake/automake-1.11.6.tar.xz
RUN tar xf autoconf-2.69.tar.gz
RUN tar xf automake-1.11.6.tar.xz

WORKDIR /usr/src/autoconf-2.69
RUN ./configure --program-suffix=-2.69
RUN make all install

WORKDIR /usr/src/automake-1.11.6
RUN ./configure --program-suffix=-1.11
RUN make all install
RUN ln -s /usr/local/share/aclocal-1.11 /usr/local/share/aclocal

# Download sources and apply Xelix patches/support files
WORKDIR /usr/src
RUN wget -c ${BINUTILS_URL}/${BINUTILS_PACKAGE}
RUN wget -c ${GCC_URL}/${GCC_PACKAGE}
RUN wget -c ${NEWLIB_URL}/${NEWLIB_PACKAGE}

RUN tar -xf ${BINUTILS_PACKAGE}
RUN tar -xf ${GCC_PACKAGE}
RUN tar -xf ${NEWLIB_PACKAGE}

COPY land/binutils/binutils-${BINUTILS_VERSION}.patch /usr/src/
RUN patch -p0 -dbinutils-${BINUTILS_VERSION}/ < binutils-${BINUTILS_VERSION}.patch
COPY land/binutils/elf_i386_xelix.sh binutils-${BINUTILS_VERSION}/ld/emulparams/
COPY land/binutils/elf_x86_64_xelix.sh binutils-${BINUTILS_VERSION}/ld/emulparams/

RUN cd binutils-${BINUTILS_VERSION}/ld && aclocal
RUN cd binutils-${BINUTILS_VERSION}/ld && automake
RUN cd binutils-${BINUTILS_VERSION} && autoreconf-2.69

COPY land/gcc/gcc-${GCC_VERSION}.patch /usr/src/
RUN patch -p0 < gcc-${GCC_VERSION}.patch
COPY land/gcc/xelix.h gcc-${GCC_VERSION}/gcc/config/

COPY land/newlib/newlib-${NEWLIB_VERSION}.patch /usr/src/
RUN patch -p0 < newlib-${NEWLIB_VERSION}.patch

COPY land/newlib/xelix newlib-${NEWLIB_VERSION}/newlib/libc/sys/xelix

WORKDIR /usr/src/newlib-${NEWLIB_VERSION}/newlib/libc/sys/xelix/
RUN aclocal-1.11 -I ../../../ -I ../../../../
RUN autoconf-2.69
RUN automake-1.11 --cygnus Makefile

WORKDIR /usr/src/newlib-${NEWLIB_VERSION}/newlib/libc/sys/
RUN aclocal-1.11 -I ../.. -I ../../..
RUN autoconf-2.69
RUN automake-1.11 --cygnus Makefile

WORKDIR /build/binutils
RUN /usr/src/binutils-${BINUTILS_VERSION}/configure \
	--target=${TARGET} \
	--prefix=/usr \
	--sysconfdir=/etc \
	--disable-nls \
	--disable-werror

RUN make DESTDIR=/toolchain all install

# GCC build uses newlib files, so configure that first
WORKDIR /build/newlib
RUN /usr/src/newlib-${NEWLIB_VERSION}/configure \
	--target=${TARGET} \
	--prefix=/usr \
	--sysconfdir=/etc \
	--enable-newlib-mb \
	--enable-newlib-iconv \
	--enable-newlib-io-c99-formats \
	--enable-newlib-io-long-long \
	--enable-newlib-io-long-double

WORKDIR /build/gcc
RUN /usr/src/gcc-${GCC_VERSION}/configure \
	--target=${TARGET} \
	--prefix=/usr \
	--sysconfdir /etc \
	--disable-nls \
	--enable-languages=c,c++ \
	--with-headers=/usr/src/newlib-${NEWLIB_VERSION}/newlib/libc/include \
	--without-docdir \
	--with-newlib

RUN make -j$(nproc) all-gcc
RUN make -j$(nproc) all-target-libgcc
RUN make -j$(nproc) all-target-libstdc++-v3

RUN make DESTDIR=/toolchain install-gcc install-target-libgcc install-target-libstdc++-v3
RUN ln -s i786-pc-xelix-gcc /toolchain/usr/bin/i786-pc-xelix-cc

WORKDIR /build/newlib
RUN make -j$(nproc) all
RUN make DESTDIR=/toolchain install
RUN cp i786-pc-xelix/newlib/libc/sys/xelix/crti.o i786-pc-xelix/newlib/libc/sys/xelix/crtn.o /toolchain/usr/i786-pc-xelix/lib/

# Strip debug info from binaries (Reduces image size substantially)
RUN strip --strip-unneeded /toolchain/usr/bin/i786-pc-xelix-* /toolchain/usr/i786-pc-xelix/bin/*
RUN find /toolchain/usr/libexec/gcc/i786-pc-xelix/13.1.0 -type f -exec strip --strip-unneeded {} \;

# Now build the actual image
FROM alpine:latest
WORKDIR /src
COPY --from=0 /toolchain /
COPY --from=0 /usr/local /usr/local

RUN apk --no-cache add wget git make gcc g++ nasm m4 perl autoconf automake \
	patch libtool mpc1 gmp mpfr libarchive gettext gawk bash coreutils \
	texinfo file python3 tar findutils gzip xz meson ninja sudo curl pacman \
	fakeroot util-linux-misc openssh-client-default rsync nano bison flex \
	qemu-img grub-bios sfdisk e2fsprogs moreutils bison flex pkgconfig gperf \
	ripgrep tzdata-utils

# Add non-root user for makepkg
RUN adduser --disabled-password --gecos '' dev
RUN echo 'dev ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# For uploads in automated runs
RUN mkdir /root/.ssh
RUN ssh-keyscan pkgs.xelix.org >> /root/.ssh/known_hosts

COPY land/pacman/pacman.linux.conf /etc/pacman.conf
CMD ["/bin/bash"]
