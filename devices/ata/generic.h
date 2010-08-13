#include <common/generic.h>

// If you're asking yourself why those aren't in interface.h: Because they're not part of the interface in fact. For DMA mode for example, you need other & less ports.
#define ATA_SELECT_PORT 0x1F6
#define ATA_COMMAND_PORT 0x1F7
#define ATA_STATUS_PORT 0x1F7
#define ATA_DATA_PORT 0x1F0
#define ATA_MASTER_DRIVE 0xA0
#define ATA_SLAVE_DRIVE 0xB0
