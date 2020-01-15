/* libc/sys/linux/sys/utmp.h - utmp structure */
/* Written 2000 by Werner Almesberger */
/* Some things copied from glibc's /usr/include/bits/utmp.h */
#ifndef _SYS_UTMP_H
#define _SYS_UTMP_H

#include <sys/types.h>
#define UTMP_FILE "/var/run/utmp"
#define UT_LINESIZE 32
#define UT_NAMESIZE 32
#define UT_HOSTSIZE 256

#ifdef __cplusplus
extern "C" {
#endif

struct exit_status {              /* Type for ut_exit, below */
    short int e_termination;      /* Process termination status */
    short int e_exit;             /* Process exit status */
};

struct utmp {
	short   ut_type;              /* Type of record */
	pid_t   ut_pid;               /* PID of login process */
	char    ut_line[UT_LINESIZE]; /* Device name of tty - "/dev/" */
	char    ut_id[4];             /* Terminal name suffix,
	                                or inittab(5) ID */
	char    ut_user[UT_NAMESIZE]; /* Username */
	char    ut_host[UT_HOSTSIZE]; /* Hostname for remote login, or
	                                kernel version for run-level
	                                messages */
	struct  exit_status ut_exit;  /* Exit status of a process
	                                marked as DEAD_PROCESS */
	int32_t   ut_session;           /* Session ID */
	struct timeval ut_tv;        /* Time entry was made */

	int32_t ut_addr_v6[4];        /* Internet address of remote
	                                host; IPv4 address uses
	                                just ut_addr_v6[0] */
	char __filler[20];            /* Reserved for future use */
};

/* Backward compatibility hacks */
#define ut_name ut_user
#ifndef _NO_UT_TIME
#define ut_time ut_tv.tv_sec
#endif
#define ut_xtime ut_tv.tv_sec
#define ut_addr ut_addr_v6[0]

#define RUN_LVL   1
#define BOOT_TIME 2
#define NEW_TIME  3
#define OLD_TIME  4
#define INIT_PROCESS  5
#define LOGIN_PROCESS 6
#define USER_PROCESS  7
#define DEAD_PROCESS  8
/* --- redundant, from sys/cygwin/sys/utmp.h --- */
struct utmp *_getutline (struct utmp *);
struct utmp *getutent (void);
struct utmp *getutid (struct utmp *);
struct utmp *getutline (struct utmp *);
void endutent (void);
void pututline (struct utmp *);
void setutent (void);
void utmpname (const char *);

#ifdef __cplusplus
}       /* C++ */
#endif
#endif
