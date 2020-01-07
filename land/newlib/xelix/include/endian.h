#ifndef	_ENDIAN_H
#define	_ENDIAN_H	1

#include <machine/endian.h>

#define	bswap16(_x)	__bswap16(_x)
#define	bswap32(_x)	__bswap32(_x)
#define	bswap64(_x)	__bswap64(_x)

#define htonl(_x) __htonl(_x)
#define	htons(_x) __htons(_x)
#define	ntohl(_x) __ntohl(_x)
#define	ntohs(_x) __ntohs(_x)

#endif	/* endian.h */
