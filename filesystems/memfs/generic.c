#include <filesystems/memfs/interface.h>
#include <memory/kmalloc.h>
#include <common/memcpy.h>

memfsHeader_t *memfsHeader;     // The header.
memfsFileHeader_t *memfsHeaders; // The list of file headers.
fsNode_t *rootNode;             // Our root directory node.
fsNode_t *devNode;              // We also add a directory node for /dev, so we can mount devfs later on.
fsNode_t *rootNodes;              // List of file nodes.
int rootNodeCount;                    // Number of file nodes.

struct dirent dirent;

// Read single file
static uint32 memfs_read(fsNode_t *node, uint32 offset, uint32 size, uint8 *buffer)
{
   memfsFileHeader_t header = memfsHeaders[node->inode];
   if (offset > header.length)
       return 0;
   if (offset+size > header.length)
       size = header.length-offset;
   memcpy(buffer, (uint8*) (header.offset+offset), size);
   return size;
}

// Read directory [aka get contents]
static struct dirent *memfs_readdir(fsNode_t *node, uint32 index)
{
   if (node == rootNode && index == 0)
   {
     strcpy(dirent.name, "dev");
     dirent.name[3] = 0; // Make sure the string is NULL-terminated.
     dirent.ino = 0;
     return &dirent;
   }

   if (index-1 >= rootNodeCount)
       return 0;
   strcpy(dirent.name, rootNodes[index-1].name);
   dirent.name[strlen(rootNodes[index-1].name)] = 0; // Make sure the string is NULL-terminated.
   dirent.ino = rootNodes[index-1].inode;
   return &dirent;
}

static fsNode_t *memfs_finddir(fsNode_t *node, char *name)
{
   if (node == rootNode &&
       !strcmp(name, "dev") )
       return devNode;

   int i;
   for (i = 0; i < rootNodeCount; i++)
       if (!strcmp(name, rootNodes[i].name))
           return &rootNodes[i];
   return 0;
}

fsNode_t *memfs_init(uint32 location)
{
   // Initialise the main and file header pointers and populate the root directory.
   memfsHeader = (memfsHeader_t *)location;
   memfsHeaders = (memfsFileHeader_t *) (location+sizeof(memfsHeader_t));

   // Initialise the root directory.
   rootNode = (fsNode_t*)kmalloc(sizeof(fsNode_t));
   strcpy(rootNode->name, "initrd");
   rootNode->mask = rootNode->uid = rootNode->gid = rootNode->inode = rootNode->length = 0;
   rootNode->flags = FS_DIRECTORY;
   rootNode->read = 0;
   rootNode->write = 0;
   rootNode->open = 0;
   rootNode->close = 0;
   rootNode->readdir = &memfs_readdir;
   rootNode->finddir = &memfs_finddir;
   rootNode->ptr = 0;
   rootNode->impl = 0;

   // Initialise the /dev directory [yes, we need one. required.]
   devNode = (fsNode_t*)kmalloc(sizeof(fsNode_t));
   strcpy(devNode->name, "dev");
   devNode->mask = devNode->uid = devNode->gid = devNode->inode = devNode->length = 0;
   devNode->flags = FS_DIRECTORY;
   devNode->read = 0;
   devNode->write = 0;
   devNode->open = 0;
   devNode->close = 0;
   devNode->readdir = &memfs_readdir;
   devNode->finddir = &memfs_finddir;
   devNode->ptr = 0;
   devNode->impl = 0;

   rootNodes = (fsNode_t*)kmalloc(sizeof(fsNode_t) * memfsHeader->fileCount); //Allocate space for files
   rootNodeCount = memfsHeader->fileCount;

   // Make all the shiny files
   int i;
   for (i = 0; i < memfsHeader->fileCount; i++)
   {
       // Edit the file's header - currently it holds the file offset
       // relative to the start of the ramdisk. We want it relative to the start
       // of memory.
       memfsHeaders[i].offset += location;
       // Create a new file node.
       strcpy(rootNodes[i].name, &memfsHeaders[i].name);
       rootNodes[i].mask = rootNodes[i].uid = rootNodes[i].gid = 0;
       rootNodes[i].length = memfsHeaders[i].length;
       rootNodes[i].inode = i;
       rootNodes[i].flags = FS_FILE;
       rootNodes[i].read = &memfs_read;
       rootNodes[i].write = 0;
       rootNodes[i].readdir = 0;
       rootNodes[i].finddir = 0;
       rootNodes[i].open = 0;
       rootNodes[i].close = 0;
       rootNodes[i].impl = 0;
   }
   return rootNode;
}
