#pragma once
#include <common/generic.h>

void log(const char *fmt, ...);
void logDec(uint32 num);
void logHex(uint32 num);
void log_init();
void setLogLevel(int level);
