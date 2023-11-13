FROM alpine:latest
LABEL maintainer="hello@lutoma.org"

ARG TARGET=i786-pc-xelix
ARG PREFIX=/toolchain

ARG BINUTILS_VERSION=2.40
ARG BINUTILS_URL=https://ftp.gnu.org/gnu/binutils
ARG BINUTILS_PACKAGE=binutils-${BINUTILS_VERSION}.tar.xz

ARG GCC_VERSION=13.1.0
ARG GCC_URL=https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}
ARG GCC_PACKAGE=gcc-${GCC_VERSION}.tar.xz

ARG NEWLIB_VERSION=3.2.0
ARG NEWLIB_URL=https://sourceware.org/pub/newlib
ARG NEWLIB_PACKAGE=newlib-${NEWLIB_VERSION}.tar.gz

ENV PATH="${PREFIX}/bin:${PATH}"

RUN apk --no-cache add wget musl-dev make gcc g++ m4 perl autoconf automake \
	patch libtool mpc1-dev gmp-dev mpfr-dev gawk texinfo file

# Build outdated autoconf and automake versions for newlib
WORKDIR /src
RUN wget https://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz
RUN wget https://ftp.gnu.org/gnu/automake/automake-1.11.6.tar.xz
RUN tar xf autoconf-2.69.tar.gz
RUN tar xf automake-1.11.6.tar.xz

WORKDIR /src/autoconf-2.69
RUN ./configure --program-suffix=-2.69
RUN make all install

WORKDIR /src/automake-1.11.6
RUN ./configure --program-suffix=-1.11
RUN make all install
RUN ln -s /usr/local/share/aclocal-1.11 /usr/local/share/aclocal

# Download sources and apply Xelix patches/support files
WORKDIR /src
RUN wget -c ${BINUTILS_URL}/${BINUTILS_PACKAGE}
RUN wget -c ${GCC_URL}/${GCC_PACKAGE}
RUN wget -c ${NEWLIB_URL}/${NEWLIB_PACKAGE}

RUN tar -xf ${BINUTILS_PACKAGE}
RUN tar -xf ${GCC_PACKAGE}
RUN tar -xf ${NEWLIB_PACKAGE}

COPY land/binutils/binutils-${BINUTILS_VERSION}.patch /src/
RUN patch -p0 -dbinutils-${BINUTILS_VERSION}/ < binutils-${BINUTILS_VERSION}.patch
COPY land/binutils/elf_i386_xelix.sh binutils-${BINUTILS_VERSION}/ld/emulparams/
COPY land/binutils/elf_x86_64_xelix.sh binutils-${BINUTILS_VERSION}/ld/emulparams/

RUN cd binutils-${BINUTILS_VERSION}/ld && aclocal
RUN cd binutils-${BINUTILS_VERSION}/ld && automake
RUN cd binutils-${BINUTILS_VERSION} && autoreconf-2.69

COPY land/gcc/gcc-${GCC_VERSION}.patch /src/
RUN patch -p0 < gcc-${GCC_VERSION}.patch
COPY land/gcc/xelix.h gcc-${GCC_VERSION}/gcc/config/

COPY land/newlib/newlib-${NEWLIB_VERSION}.patch /src/
RUN patch -p0 < newlib-${NEWLIB_VERSION}.patch

COPY land/newlib/xelix newlib-${NEWLIB_VERSION}/newlib/libc/sys/xelix

WORKDIR /src/newlib-${NEWLIB_VERSION}/newlib/libc/sys/xelix/
RUN aclocal-1.11 -I ../../../ -I ../../../../
RUN autoconf-2.69
RUN automake-1.11 --cygnus Makefile

WORKDIR /src/newlib-${NEWLIB_VERSION}/newlib/libc/sys/
RUN aclocal-1.11 -I ../.. -I ../../..
RUN autoconf-2.69
RUN automake-1.11 --cygnus Makefile

WORKDIR /build/binutils
RUN /src/binutils-${BINUTILS_VERSION}/configure --target=${TARGET} --prefix=${PREFIX} --disable-nls --disable-werror
RUN make all install

# GCC build uses newlib files, so configure that first
WORKDIR /build/newlib
RUN /src/newlib-${NEWLIB_VERSION}/configure --target=${TARGET} --prefix=${PREFIX}

WORKDIR /build/gcc
RUN /src/gcc-${GCC_VERSION}/configure \
	--target=${TARGET} \
	--prefix=${PREFIX} \
	--disable-nls \
	--enable-languages=c,c++ \
	--with-headers=/src/newlib-${NEWLIB_VERSION}/newlib/libc/include \
	--without-docdir

RUN make -j$(nproc) all-gcc
RUN make -j$(nproc) all-target-libgcc

# causes issues with newer GCC versions. Needs debugging
#RUN make -j$(nproc) all-target-libstdc++-v3

RUN make install-gcc install-target-libgcc
RUN ln -s i786-pc-xelix-gcc ${PREFIX}/bin/i786-pc-xelix-cc

WORKDIR /build/newlib
RUN make -j$(nproc) all
RUN make install
RUN cp i786-pc-xelix/newlib/libc/sys/xelix/crti.o i786-pc-xelix/newlib/libc/sys/xelix/crtn.o ${PREFIX}/i786-pc-xelix/lib/

# Drop sys-include directory to avoid conflicts with regular includes
run rm -r ${PREFIX}/i786-pc-xelix/sys-include

FROM alpine:latest
WORKDIR /src

RUN apk --no-cache add wget musl-dev gcc libarchive-dev gettext-dev gawk bash \
	meson ninja curl-dev

# Build a native/host copy of pacman so we can build and manage Xelix userland packages
RUN wget -c https://gitlab.archlinux.org/pacman/pacman/-/archive/v6.0.2/pacman-v6.0.2.tar.gz
RUN tar xf pacman-v6.0.2.tar.gz

WORKDIR /src/pacman-v6.0.2
RUN meson setup -Dc_link_args='-lintl' --prefix /usr build
RUN ninja -C build
RUN DESTDIR=/pacman ninja -C build install

# Now build the actual image
FROM alpine:latest
WORKDIR /src
COPY --from=0 /toolchain /usr
COPY --from=1 /pacman /

RUN apk --no-cache add wget git make gcc g++ nasm m4 perl autoconf automake \
	patch libtool mpc1 gmp mpfr libarchive gettext gawk bash coreutils \
	texinfo file python3 tar findutils gzip xz meson ninja sudo curl

# Add python stuff for (legacy) xpkg
ENV PYTHONUNBUFFERED=1
RUN python3 -m ensurepip
RUN pip3 install --no-cache --upgrade pip setuptools PyYAML toposort click

# Add non-root user for makepkg
RUN adduser --disabled-password --gecos '' dev
RUN echo 'dev ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

CMD ["/bin/bash"]
