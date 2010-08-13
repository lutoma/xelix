#include <common/generic.h>

// If you're asking yourself why those aren't in interface.h: Because they're not part of the interface in fact. For DMA mode for example, you need other & less ports.
#define PRIMARY_DRIVE_SELECT 0x1F6
