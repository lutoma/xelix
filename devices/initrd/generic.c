// 'driver' for initrd access as a device
#include <devices/initrd/interface.h>

#include <common/log.h>
#include <memory/kmalloc.h>
#include <common/fio.h>

static uint32 position;

uint32 __readHandler(fsNode_t *node, uint32 offset, uint32 size, uint8 *buffer)
{
	memcpy(buffer, position, size);
	return size;
}

//uint32 vfs_writeNode(fsNode_t *node, uint32 offset, uint32 size, uint8 *buffer);


void initrd_init(char** ramPosition)
{
	if(ramPosition[0] != NULL)
	{
		position = ramPosition[0];
		log("initrd: Creating /dev/initrd\n");
		vfs_createNode("initrd", 0, 0, 0, FS_FILE, 1, 0, 0, &__readHandler, NULL, NULL, NULL, NULL, NULL, NULL, vfs_devNode);
	}

	FILE* initrd = fopen("/dev/initrd", "r");
	if(initrd != NULL)
		fclose(initrd);

}
