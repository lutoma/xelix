// Standard configuration - May be overwritten by user's local.h

#pragma once

#define PIT_RATE 500
#define DEBUGCONSOLE_PROMPT "\n> "
#define LOG_SAVE
#define LOG_MAXSIZE 10000
#define MEMORY_MAX_KMEM 0xBFFFFF

#define WITH_SERIAL
#define WITH_DEBUGCONSOLE
// Doesn't work as it should
#undef WITH_NEW_KMALLOC

