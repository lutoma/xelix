#ifndef	_DLFCN_H
#define	_DLFCN_H	1

#define RTLD_LAZY 1
#define RTLD_NOW 2
#define RTLD_GLOBAL 3
#define RTLD_LOCAL 4

#ifdef __cplusplus
extern "C" {
#endif

int dlclose(void *);
char *dlerror(void);
void *dlopen(const char *, int);
void *dlsym(void *restrict, const char *restrict);

#ifdef __cplusplus
}       /* C++ */
#endif
#endif	/* dlfcn.h */
