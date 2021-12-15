# Compiling

!!! note
	Xelix can currently only be compiled on Linux. In theory it should also be possible to build on BSDs, MacOS or on Windows with WSL, but the Makefiles and configure script do not have support for these.

## Toolchain

To compile Xelix, you first need the Xelix toolchain. It consists of binutils, a GCC cross-compiler and the newlib C standard library, all with configuration and patches to support the i786-pc-xelix target.

### Dependencies

Please check [Prerequisites for GCC](https://gcc.gnu.org/install/prerequisites.html) for GCC build dependencies. On Debian-based systems, you can install them using `apt install build-essential autoconf automake`.

Unfortunately, the binutils and newlib build processes rely on deprecated autoconf and automake functionality that has been removed in current releases. In order to patch and compile those you will also need two specific versions:

  * autoconf 2.69
  * automake 1.11

The toolchain Makefile expects the binaries to be available in your `PATH` as `autoconf-2.69` and `automake-1.11`. Some distributions have these available through package management, otherwise you can build them with

```bash
wget https://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz
tar xf autoconf-2.69.tar.gz
cd autoconf-2.69
./configure --program-suffix=-2.69
make
sudo make install

wget https://ftp.gnu.org/gnu/automake/automake-1.11.6.tar.xz
tar xf automake-1.11.6.tar.xz
cd automake-1.11.6/
./configure --program-suffix=-1.11
make
sudo make install
```


### Building the toolchain

Once you have the dependencies, you can build the toolchain using

```bash
make -C toolchain
```

Depending on your hardware, this will take a significant amount of time.

Once it is done, you should find Xelix-specific binaries of GCC, g++, ld, etc. in `toolchain/local/bin`. I find it convenient to add that directory to `PATH`, but the Xelix Makefile will also work without it.

`i786-pc-xelix-gcc` and `i786-pc-xelix-g++` from that directory can also be used to compile userspace binaries and automatically include the correct standard library, linker script and crt0:

```bash
# Can be used like regular GCC
i786-pc-xelix-gcc test.c -o test
```

## Compiling the kernel

In addition to the toolchain, the [NASM](https://www.nasm.us/) assembler is also required. Since no Xelix-specific patches are needed, you can just use the one from your distribution's package sources (Arch Linux: `pacman -S nasm`, Debian/Ubuntun: `apt install nasm`).

Once you have that in place, you can compile Xelix using:

```bash
./configure

# Optional, if you want to customize settings:
make menuconfig

make
```

You should now see a binary called `xelix.bin` in your directory. 🎉

### Using the compiled kernel

By itself, the kernel is not very useful as it can only function within a Xelix disk image. You will have to download the disk image of a Xelix release, then replace the kernel in the `/boot` directory.

Since the disk images are in the QEMU qcow2 format, the easiest way to accomplish this is using `qemu-nbd`, which exposes the image as a network block device. This requires the Linux `nbd` kernel module, which is usually available by default.

```bash
sudo modprobe nbd
sudo qemu-nbd --connect=/dev/nbd0 xelix.qcow2
mkdir mnt
# First partition of the disk image is /boot, second partition is /
sudo mount /dev/nbd0p1 mnt
sudo cp xelix.bin mnt
sudo umount mnt
sudo qemu-nbd --disconnect /dev/nbd0
```

You can now run the disk image using qemu like usual.

Since this has to be done very frequently during development, there are a number of helpers in the Xelix makefile that automate these steps and run the resulting image. For this to work, you need to place the disk image in the Xelix git directory and name it `xelix.qcow2`. Then, invoking

```bash
make run
```

will automatically compile the kernel, place the resulting kernel in the disk image and start QEMU with it.


## Building a disk image

```
util/xpkg/xpkg -a
```

```
util/build-image.sh xpkg-build/image xelix.qcow2
```
