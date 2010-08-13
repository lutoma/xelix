#include <common/generic.h>

// If you're asking yourself why those aren't in interface.h: Because they're not part of the interface in fact. For DMA mode for example, you need other & less ports.
#define SELECT_PORT 0x1F6
#define COMMAND_PORT 0x1F7
#define STATUS_PORT 0x1F7
#define DATA_PORT 0x1F0
