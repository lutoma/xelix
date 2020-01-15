#ifndef	_ERR_H
#define	_ERR_H	1

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

void err(int eval, const char *fmt, ...);
void verr(int eval, const char *fmt, va_list args);
void errc(int eval, int code, const char *fmt, ...);
void verrc(int eval, int code, const char *fmt, va_list args);
void errx(int eval, const char *fmt, ...);
void verrx(int eval, const char *fmt, va_list args);
void warn(const char *fmt, ...);
void vwarn(const char *fmt, va_list args);
void warnc(int code, const char *fmt, ...);
void vwarnc(int code, const char *fmt, va_list args);
void warnx(const char *fmt, ...);
void vwarnx(const char *fmt, va_list args);

#ifdef __cplusplus
}       /* C++ */
#endif
#endif /* err.h */
