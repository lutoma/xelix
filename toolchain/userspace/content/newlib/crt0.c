#include "call.h"
extern int main(int argc, char* argv);

void _start() {
	call_exit(main(0, NULL));
}
