#ifndef FILESYSTEMS_INTERFACE_H
#define FILESYSTEMS_INTERFACE_H

#include <common/generic.h>

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05
#define FS_SYMLINK     0x06
#define FS_MOUNTPOINT  0x08 // Is the file an active mountpoint?

struct fsNode;

typedef uint32 (*readType_t)(struct fsNode*,uint32,uint32,uint8*);
typedef uint32 (*writeType_t)(struct fsNode*,uint32,uint32,uint8*);
typedef void (*openType_t)(struct fsNode*);
typedef void (*closeType_t)(struct fsNode*);
typedef struct dirent * (*readdirType_t)(struct fsNode*,uint32);
typedef struct fsNode * (*finddirType_t)(struct fsNode*,char *name);

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
   readType_t read;
   writeType_t write;
   openType_t open;
   closeType_t close;
   readdirType_t readdir;
   finddirType_t finddir;
   struct fs_node *ptr; // Used by mountpoints and symlinks.
} fsNode_t;

struct dirent // One of these is returned by the readdir call, according to POSIX.
{
  char name[128]; // Filename.
  uint32 ino;     // Inode number. Required by POSIX.
};

extern fsNode_t *fsRoot; // The root of the filesystem.

// Standard read/write/open/close functions. Note that these are all suffixed with
// Fs to distinguish them from the read/write/open/close which deal with file descriptors
// not file nodes.
uint32 readFs(fsNode_t *node, uint32 offset, uint32 size, uint8 *buffer);
uint32 writeFs(fsNode_t *node, uint32 offset, uint32 size, uint8 *buffer);
void openFs(fsNode_t *node, uint8 read, uint8 write);
void closeFs(fsNode_t *node);
struct dirent *readdirFs(fsNode_t *node, uint32 index);
fsNode_t *finddirFs(fsNode_t *node, char *name);

#endif
