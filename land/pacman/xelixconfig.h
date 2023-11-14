#pragma once

#define CACHEDIR "/var/cache/pacman/pkg/"

#define CONFFILE "/etc/pacman.conf"

#define DBPATH "/var/lib/pacman/"

#undef ENABLE_NLS

#define FSSTATSTYPE struct statvfs

#define GPGDIR "/etc/pacman.d/gnupg/"

#undef HAVE_GETMNTENT

#define HAVE_LIBCURL 1

#undef HAVE_LIBGPGME

#define HAVE_LIBSSL 1

#define HAVE_MNTENT_H

#define HAVE_STRNDUP 1

#define HAVE_STRNLEN 1

#define HAVE_STRSEP 1

#undef HAVE_STRUCT_STATFS_F_FLAGS

#define HAVE_STRUCT_STATVFS_F_FLAG

#define HAVE_STRUCT_STAT_ST_BLKSIZE

#define HAVE_SWPRINTF 1

#define HAVE_SYS_MOUNT_H

#define HAVE_SYS_PARAM_H

#undef HAVE_SYS_STATVFS_H

#define HAVE_SYS_TYPES_H

#define HAVE_TCFLUSH 1

#define HAVE_TERMIOS_H

#define HOOKDIR "/etc/pacman.d/hooks/"

#define LDCONFIG "/sbin/ldconfig"

#define LIB_VERSION "13.0.1"

#define LOCALEDIR "/usr/share/locale"

#define LOGFILE "/var/log/pacman.log"

#define PACKAGE "pacman"

#define PACKAGE_VERSION "6.0.1"

#define PACMAN_DEBUG 1

#define ROOTDIR "/"

#define SCRIPTLET_SHELL "/bin/sh"

#define SYSHOOKDIR "/usr/share/libalpm/hooks/"

#define _GNU_SOURCE
