#ifndef _SYS_UTIME_H
#define _SYS_UTIME_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct utimbuf
{
  time_t actime;
  time_t modtime;
};

#ifdef __cplusplus
};
#endif

#endif /* _SYS_UTIME_H */
