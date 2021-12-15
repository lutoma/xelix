# xelix-utils

xelix-utils is a small collection of tools that supplant the functionality provided by GNU packages like coreutils. It's essentially the Xelix equivalent of [util-linux](https://en.wikipedia.org/wiki/Util-linux).

## init

PID 1. Mounts `/dev/ide1p1` to `/boot` and launches services. Service definitions are located in `/etc/init.d` and take the format of

```
[Service]
ExecStart=/usr/bin/gfxcompd
```

Services are automatically restarted if they exit. There is currently no way to manually start/stop services.

## mount / umount

View currently mounted file systems and mount/unmount new ones. Equivalent to the BSD/Linux utilities of the same name.

## dmesg

Reads and formats the kernel log from `/sys/log`, then prints it. Equivalent to the BSD/Linux utility of the same name.

## strace

Trace syscalls invoked by a program. Use by prefixing it before the program to be run, like `strace ls /usr/bin`.

## uptime

Prints system uptime. Does not include system load as on BSD/Linux, as there is currently no tracking of system load.

## ps

Prints currently running tasks and their program state.

## telnetd

A very barebones telnet server. Does not implement the telnet protocol properly, so it's best to just connect using using netcat: `nc <ip> 23`.

It mostly serves as a demonstration of Xelix's networking capabilties and will be removed once the ported SSH daemon from dropbear has become more stable.

## play

A simple CLI player for FLAC audio files using libFLAC. This mostly serves as a demonstration for the AC97 sound chip driver and will be replaced by a more generic solution using the existing ffmpeg port at some later point.
