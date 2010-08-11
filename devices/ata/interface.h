#ifndef DEVICES_ATA_INTERFACE_H
#define DEVICES_ATA_INTERFACE_H

#include <common/generic.h>
struct ataDrive;
typedef struct ataDrive
{
	char name[128];
} ataDrive_t;

void ata_init();

#endif
