// Multiboot-related functions
#include <common/multiboot.h>

#include <common/log.h>

void multiboot_printInfo(multibootHeader_t *pointer)
{
	log("%%Multiboot information:%%\n", 0x0f);
	log("\tflags: %d\n", pointer->flags);
	log("\tmemLower: 0x%x\n", pointer->memLower);
	log("\tmemUpper: 0x%x\n", pointer->memUpper);
	//log("\tbootDevice: %s\n", pointer->bootDevice);
	//if(pointer->bootLoaderName != 0) //QEMU doesn't append a \0 to the cmdline, resulting in epic failure in printf().
		//log("\tcmdLine: %s\n", pointer->cmdLine);
	log("\tmodsCount: %d\n", pointer->modsCount);
	log("\tmodsAddr: 0x%x\n", pointer->modsAddr);
	log("\tnum: %d\n", pointer->num);
	log("\tsize: %d\n", pointer->size);
	log("\taddr: 0x%x\n", pointer->addr);
	log("\tshndx: %d\n", pointer->shndx);
	log("\tmmapLength: %d\n", pointer->mmapLength);
	log("\tmmapAddr: 0x%x\n", pointer->mmapAddr);
	log("\tdrivesLength: %d\n", pointer->drivesLength);
	log("\tdrivesAddr: 0x%x\n", pointer->drivesAddr);
	log("\tconfigTable: %d\n", pointer->configTable);
	if(pointer->bootLoaderName != 0)
		log("\tbootLoaderName: %s\n", pointer->bootLoaderName);
	else
		log("\tbootLoaderName: 0\n");
	log("\tapmTable: %d\n", pointer->apmTable);
	log("\tvbeControlInfo: %d\n", pointer->vbeControlInfo);
	log("\tvbeModeInfo: %d\n", pointer->vbeModeInfo);
	log("\tvbeMode: %d\n", pointer->vbeMode);
	log("\tvbeInterfaceSeg: %d\n", pointer->vbeInterfaceSeg);
	log("\tvbeInterfaceOff: %d\n", pointer->vbeInterfaceOff);
	log("\tvbeInterfaceLen: %d\n", pointer->vbeInterfaceLen);
}
