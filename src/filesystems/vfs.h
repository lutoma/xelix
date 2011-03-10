#pragma once

/* Copyright © 2010, 2011 Lukas Martini
 *
 * This file is part of Xelix.
 *
 * Xelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <lib/generic.h>

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05
#define FS_SYMLINK     0x06
#define FS_MOUNTPOINT  0x08 // Is the file an active mountpoint?

typedef struct fsNode;
typedef uint32 (*read_type_t)(struct fsNode*,uint32,uint32,uint8*);
typedef uint32 (*write_type_t)(struct fsNode*,uint32,uint32,uint8*);
typedef uint32 (*open_type_t)(struct fsNode*);
typedef void (*close_type_t)(struct fsNode*);
typedef struct dirent* (*readdir_type_t)(struct fsNode*,uint32);
typedef struct fsNode* (*finddir_type_t)(struct fsNode*,char *name);

// __fsNode is only there for use in the function declarations above.
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

fsNode_t* vfs_createNode(char name[128], uint32 mask, uint32 uid, uint32 gid, uint32 flags, uint32 inode, uint32 length, uint32 impl, read_type_t read, write_type_t write, open_type_t open, close_type_t close, readdir_type_t readdir, finddir_type_t finddir, fsNode_t *ptr, fsNode_t *parent);

fsNode_t* vfs_rootNode; // Our root directory node.
fsNode_t** vfs_rootNodes; // Our root directory array.
int vfs_rootNodeCount;

void vfs_init(char** modules);
