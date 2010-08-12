#ifndef DEVICES_FLOPPY_INTERFACE_H
#define DEVICES_FLOPPY_INTERFACE_H
#include <common/generic.h>

typedef struct floppyDrive
{
  int number;
  size_t size;
  size_t inch;
} floppyDrive_t;

void floppy_init();

#endif
