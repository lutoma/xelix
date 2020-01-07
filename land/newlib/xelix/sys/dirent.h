#ifndef _SYS_DIRENT_H
#define _SYS_DIRENT_H

#include <sys/types.h>
#include <bits/dirent.h>
#define _LIBC 1
#define  NOT_IN_libc 1
#include <sys/lock.h>
#undef _LIBC

#define HAVE_NO_D_NAMLEN    /* no struct dirent->d_namlen */
#define HAVE_DD_LOCK        /* have locking mechanism */

#define MAXNAMLEN 255       /* sizeof(struct dirent.d_name)-1 */

#define DT_UNKNOWN 0
#define DT_REG 1
#define DT_DIR 2
#define DT_CHR 3
#define DT_BLK 4
#define DT_FIFO 5
#define DT_SOCK 6
#define DT_LNK 7

_BEGIN_STD_C

typedef struct {
    int dd_fd;      /* directory file */
    int dd_loc;     /* position in buffer */
    int dd_seek;
    char *dd_buf;   /* buffer */
    int dd_len;     /* buffer length */
    int dd_size;    /* amount of data in buffer */
    _LOCK_RECURSIVE_T dd_lock;
} DIR;

_END_STD_C

#define __dirfd(dir) (dir)->dd_fd

#include <dirent.h>
#endif
