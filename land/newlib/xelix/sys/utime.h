#ifndef _SYS_UTIME_H
#define _SYS_UTIME_H

#include <sys/types.h>

_BEGIN_STD_C

struct utimbuf
{
  time_t actime;
  time_t modtime;
};


int utime(const char *path, const struct utimbuf *times);

_END_STD_C
#endif /* _SYS_UTIME_H */
