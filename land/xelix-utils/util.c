#include <unistd.h>
#include "util.h"

char* shortname(char* in) {
	for(char* i = in; *i; i++) {
		if(*i == '.') {
			*i = 0;
			break;
		}
	}
	return in;
}


char* readable_fs(uint32_t size, char* buf) {
    int i = 0;
    const char* units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    while (size > 1024) {
        size /= 1024;
        i++;
    }
    sprintf(buf, "%d %s", size, units[i]);
    return buf;
}
