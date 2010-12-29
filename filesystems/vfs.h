#pragma once

#include <common/generic.h>

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05
#define FS_SYMLINK     0x06
#define FS_MOUNTPOINT  0x08 // Is the file an active mountpoint?

typedef uint32 (*read_type_t)(struct fsNode*,uint32,uint32,uint8*);
typedef uint32 (*write_type_t)(struct fsNode*,uint32,uint32,uint8*);
typedef uint32 (*open_type_t)(struct fsNode*);
typedef void (*close_type_t)(struct fsNode*);
typedef struct dirent * (*readdir_type_t)(struct fsNode*,uint32);
typedef struct fsNode * (*finddir_type_t)(struct fsNode*,char *name);


typedef struct fsNode
{
   char name[128];     // The filename.
   uint32 mask;        // The permissions mask.
   uint32 uid;         // The owning user.
   uint32 gid;         // The owning group.
   uint32 flags;       // Includes the node type. See #defines above.
   uint32 inode;       // This is device-specific - provides a way for a filesystem to identify files.
   uint32 length;      // Size of the file, in bytes.
   uint32 impl;        // An implementation-defined number.
   read_type_t read;
   write_type_t write;
   open_type_t open;
   close_type_t close;
   readdir_type_t readdir;
   finddir_type_t finddir;
   struct fsNode *ptr; // Used by mountpoints and symlinks.
   struct fsNode *parent;
} fsNode_t;

struct dirent // One of these is returned by the readdir call, according to POSIX.
{
  char name[128]; // Filename.
  uint32 ino;     // Inode number. Required by POSIX.
};
/*
// Standard read/write/open/close functions.
uint32 vfs_readNode(fsNode_t *node, uint32 offset, uint32 size, uint8 *buffer);
uint32 vfs_writeNode(fsNode_t *node, uint32 offset, uint32 size, uint8 *buffer);
int vfs_openNode(fsNode_t *node, uint8 read, uint8 write);
void vfs_closeNode(fsNode_t *node);
struct dirent *vfs_readdirNode(fsNode_t *node, uint32 index);
fsNode_t *vfs_finddirNode(fsNode_t *node, char *name);
*/

fsNode_t* vfs_createNode(char name[128], uint32 mask, uint32 uid, uint32 gid, uint32 flags, uint32 inode, uint32 length, uint32 impl, read_type_t read, write_type_t write, open_type_t open, close_type_t close, readdir_type_t readdir, finddir_type_t finddir, fsNode_t *ptr, fsNode_t *parent);

fsNode_t* vfs_rootNode; // Our root directory node.
fsNode_t* vfs_tmpNode;
fsNode_t* vfs_devNode;

void vfs_init();

