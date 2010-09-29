#pragma once

#include <common/generic.h>

void serial_init();
char serial_recv();
void serial_print(char* s);
