#include <unistd.h>
#include "util.h"

char* shortname(char* in) {
	for(char* i = in; i; i++) {
		if(*i == '.') {
			*i = 0;
			break;
		}
	}
	return in;
}
