#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include "internal.h"

// This is all very hacky and not properly integrated.

static void xelix_scan(struct pci_access *a) {
  u8 busmap[256];
  int bus;

  FILE* fp = fopen("/sys/pci", "r");
  if(!fp) {
    perror("Could not read pci stats");
    exit(EXIT_FAILURE);
  }

  while(true) {
    if(feof(fp)) {
      return EXIT_SUCCESS;
    }

    uint32_t bus;
    uint32_t dev;
    uint32_t func;
    uint32_t vendorID;
    uint32_t deviceID;
    uint32_t class;
    uint32_t revision;
    uint32_t iobase;
    uint32_t headerType;
    uint32_t interruptLine;
    uint32_t interruptPin;

    if(fscanf(fp, "%d:%d.%d %x:%x %x %x %x %x %d %d\n",
      &bus, &dev, &func, &vendorID, &deviceID, &class, &revision, &iobase, &headerType, &interruptLine, &interruptPin) != 11) {
      fprintf(stderr, "Matching error.\n");
      exit(EXIT_FAILURE);
    }

    struct pci_dev* d = pci_alloc_dev(a);
    d->bus = bus;
    d->dev = dev;
    d->func = func;
    d->vendor_id = vendorID;
    d->device_id = deviceID;
    d->device_class = class;
    d->hdrtype = headerType;
    pci_link_dev(a, d);
  }

  exit(EXIT_SUCCESS);
}

static void xelix_config(struct pci_access *a) {
  pci_define_param(a, "xelix.path", PCI_PATH_XELIX_DEVICE, "Path to the xelix PCI device");
}

static int xelix_detect(struct pci_access *a UNUSED) {
  int fno = open(PCI_PATH_XELIX_DEVICE, O_RDONLY);
  if(fno == -1) {
    return 0;
  } else {
    close(fno);
    return 1;
  }
}

static void xelix_init(struct pci_access *a UNUSED) {}
static void xelix_cleanup(struct pci_access *a UNUSED) {}

static int xelix_read(struct pci_dev *d, int pos UNUSED, byte *buf UNUSED, int len UNUSED) {
  char* dname;
  asprintf(&dname, "/dev/pci%dd%df%d", d->bus, d->dev, d->func);
  FILE* fp = fopen(dname, "r");
  if(!fp) {
    return 0;
  }

  free(dname);
  fclose(fp);
  return 1;
}

static int xelix_write(struct pci_dev *d, int pos, byte *buf, int len) {
  return 0;
}

int xelix_fill_info(struct pci_dev *d, int flags)
{
}

struct pci_methods pm_xelix_device = {
  "xelix-device",
  "xelix /dev/pci devices",
  xelix_config,
  xelix_detect,
  xelix_init,
  xelix_cleanup,
  xelix_scan,
  xelix_fill_info,
  xelix_read,
  xelix_write,
  NULL,			// no read_vpd
  NULL,			// no init_dev
  NULL,			// no cleanup_dev
};
