#ifndef _SYS_RESOURCE_H_
#define _SYS_RESOURCE_H_

#include <sys/time.h>

#define	RUSAGE_SELF	0		/* calling process */
#define	RUSAGE_CHILDREN	-1		/* terminated child processes */

#define RLIM_INFINITY 0
#define RLIM_NLIMITS 0

#define RLIMIT_CORE 1
#define RLIMIT_CPU 2
#define RLIMIT_DATA 3
#define RLIMIT_FSIZE 4
#define RLIMIT_NOFILE 5
#define RLIMIT_STACK 6
#define RLIMIT_AS 7

_BEGIN_STD_C

struct rusage {
  	struct timeval ru_utime;	/* user time used */
	struct timeval ru_stime;	/* system time used */
	long ru_maxrss;
};

typedef uint32_t rlim_t;

struct rlimit {
	rlim_t rlim_cur;  /* Soft limit */
	rlim_t rlim_max;  /* Hard limit (ceiling for rlim_cur) */
};

int  getpriority(int, id_t);
int  getrlimit(int, struct rlimit *);
int  getrusage(int, struct rusage *);
int  setpriority(int, id_t, int);
int  setrlimit(int, const struct rlimit *);

_END_STD_C
#endif

