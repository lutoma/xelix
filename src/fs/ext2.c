/* ext2.c: Implementation of the extended filesystem, version 2
 * Copyright © 2013-2015 Lukas Martini
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
#include <lib/log.h>
#include <lib/string.h>
#include <lib/md5.h>
#include <memory/kmalloc.h>
#include <hw/ide.h>
#include <fs/vfs.h>
#include "ext2.h"

#ifdef EXT2_DEBUG
  #define debug(args...) log(LOG_DEBUG, "ext2: " args)
#else
  #define debug(...)
#endif

typedef struct ext2_superblock {
	uint32_t inode_count;
	uint32_t block_count;
	uint32_t reserved_blocks;
	uint32_t free_blocks;
	uint32_t free_inodes;
	uint32_t first_data_block;
	uint32_t block_size;
	int32_t fragment_size;
	uint32_t blocks_per_group;
	uint32_t fragments_per_group;
	uint32_t inodes_per_group;
	uint32_t mount_time;
	uint32_t write_time;
	uint16_t mount_count;
	int16_t max_mount_count;
	uint16_t magic;
	uint16_t state;
	uint16_t errors;
	uint16_t minor_revision;
	uint32_t last_check_time;
	uint32_t check_interval;
	uint32_t creator_os;
	uint32_t revision;
	uint16_t default_res_uid;
	uint16_t default_res_gid;
	uint32_t first_inode;
	uint16_t inode_size;
	uint16_t blockgroup_num;
	uint32_t features_compat;
	uint32_t features_incompat;
	uint32_t features_ro;
	uint32_t volume_id[4];
	char volume_name[16];
	char last_mounted[64];
	uint32_t algo_bitmap;
	uint32_t reserved[205];
} __attribute__((packed)) ext2_superblock_t;

typedef struct ext2_blockgroup {
	uint32_t block_bitmap;
	uint32_t inode_bitmap;
	uint32_t inode_table;
	uint16_t free_blocks;
	uint16_t free_inodes;
	uint16_t used_directories;
	uint16_t padding;
	uint32_t reserved[3];
} __attribute__((packed)) ext2_blockgroup_t;

typedef struct ext2_inode {
	uint16_t mode;
	uint16_t uid;
	uint32_t size;
	uint32_t access_time;
	uint32_t creation_time;
	uint32_t modification_time;
	uint32_t deletion_time;
	uint16_t gid;
	uint16_t link_count;
	uint32_t block_count;
	uint32_t flags;
	uint32_t reserved1;
	uint32_t blocks[15];
	uint32_t version;
	uint32_t file_acl;
	uint32_t dir_acl;
	uint32_t fragment_address;
	uint8_t fragment_number;
	uint8_t fragment_size;
	uint16_t reserved2[5];
} __attribute__((packed)) ext2_inode_t;

typedef struct ext2_dirent {
	uint32_t inode;
	uint16_t record_len;
	uint8_t name_len;
	uint8_t type;
	char name[];
} __attribute__((packed)) ext2_dirent_t;

#define SUPERBLOCK_MAGIC 0xEF53
#define SUPERBLOCK_STATE_CLEAN 1
#define SUPERBLOCK_STATE_DIRTY 2
#define ROOT_INODE 2

// File Types
#define FT_IFSOCK	0xC000
#define FT_IFLNK	0xA000
#define FT_IFREG	0x8000
#define FT_IFBLK	0x6000
#define FT_IFDIR	0x4000
#define FT_IFCHR	0x2000
#define FT_IFIFO	0x1000

// Some helper macros for commonly needed conversions
#define inode_to_blockgroup(inode) ((inode - 1) / superblock->inodes_per_group)
#define mode_to_filetype(mode) (mode & 0xf000)
// TODO Should use right shift for negative values
#define superblock_to_blocksize(superblock) (1024 << superblock->block_size)

#define read_sector_or_fail(rc, args...) do {													\
		if(ide_read_sector(args) != true) {														\
			log(LOG_ERR, "ext2: IDE read failed in %s line %d, bailing.", __func__, __LINE__);	\
			return rc;																			\
		}																						\
	} while(0)

static ext2_superblock_t* superblock = NULL;


static char* filetype_to_verbose(int filetype) {
	switch(filetype) {
		case FT_IFSOCK: return "FT_IFSOCK";
		case FT_IFLNK: return "FT_IFLNK";
		case FT_IFREG: return "FT_IFREG";
		case FT_IFBLK: return "FT_IFBLK";
		case FT_IFDIR: return "FT_IFDIR";
		case FT_IFCHR: return "FT_IFCHR";
		case FT_IFIFO: return "FT_IFIFO";
		default: return NULL;
	}
}

/* Reads an inode. Takes the number of the inode as argument and locates and
 * returns the corresponding ext2_inode_t*.
 */
static ext2_inode_t* read_inode(uint32_t inode_num)
{
	uint32_t blockgroup_num = inode_to_blockgroup(inode_num);

	debug("Reading inode struct %d in blockgroup %d\n", inode_num, blockgroup_num);

	// Sanity check the blockgroup num
	if(blockgroup_num > (superblock->block_count / superblock->blocks_per_group))
		return NULL;

	uint32_t blockgroup_table_disk_sector =
		superblock_to_blocksize(superblock) / 512;

	// FIXME This should rather calculate the absolute disk offset
	if(superblock_to_blocksize(superblock) == 1024)
		blockgroup_table_disk_sector *= 2;

	uint8_t* blockgroup_table = (uint8_t*)kmalloc(512);

	read_sector_or_fail(0, 0x1F0, 0, blockgroup_table_disk_sector, blockgroup_table);

	// Locate the entry for the relevant blockgroup
	ext2_blockgroup_t* blockgroup = 
		(ext2_blockgroup_t*)((intptr_t)blockgroup_table + 
		(sizeof(ext2_blockgroup_t) * blockgroup_num));

	// Now that we have the blockgroup data, look for the inode
	uint32_t inode_table_location = blockgroup->inode_table *
		superblock_to_blocksize(superblock);

	kfree(blockgroup_table);

	// Read inode table for this block group
	// TODO Only read the relevant parts (Or just cache the darn thing)
	uint8_t* inode_table_sector = (uint8_t*)kmalloc(
		superblock->inodes_per_group * superblock->inode_size);

	for(int i = 0; i < (superblock->inodes_per_group * superblock->inode_size) / 512; i++)
		read_sector_or_fail(0, 0x1F0, 0, (inode_table_location / 512) + i, inode_table_sector + i * 512);


	ext2_inode_t* inode = (ext2_inode_t*)((intptr_t)inode_table_sector +
		(((inode_num - 1) % superblock->inodes_per_group) *
		superblock->inode_size));

	return inode;
}

static uint8_t* read_inode_block(ext2_inode_t* inode, uint32_t block_num, bool direct_indirect)
{
	// FIXME This value actually depends on block size. This expects a 1024 block size
	if(block_num > 268)
	{
		log(LOG_INFO, "ext2: sorry, no support for doubly & triply indirect "
			"blocks. Poke lutoma.\n");
		return NULL;
	}

	uint32_t real_block_num = 0;
	if(block_num >= 12 && !direct_indirect) {
		// Indirect block
		uint32_t* blocks_table = (uint32_t*)read_inode_block(inode, 12, true);
		real_block_num = blocks_table[block_num - 12];
		kfree(blocks_table);
	} else {
		real_block_num = inode->blocks[block_num];
	}

	if(!real_block_num) {
		return NULL;
	}

	debug("read_inode_block: Actual block for inode block %d is %d\n", block_num, real_block_num);

	intptr_t block_location = real_block_num * superblock_to_blocksize(superblock);

	// Allocate correct size + one HDD sector (Since we can only read in 512
	// byte chunks from the disk.
	uint8_t* block = (uint8_t*)kmalloc(superblock_to_blocksize(superblock) + 512);
	for(int i = 0; i < superblock_to_blocksize(superblock); i += 512) {
		read_sector_or_fail(0, 0x1F0, 0, block_location / 512 + i, block + i);
		debug("READING offset %d AT %d, write to %d\n", i, block_location / 512 + i, block + i);
	}

	return block;
}

// Returns the dirent for the n-th file in a directory inode
static ext2_dirent_t* read_dirent(uint32_t inode_num, uint32_t offset)
{
	ext2_inode_t* inode = read_inode(inode_num);
	if(!inode)
		return NULL;

	// Check if this inode is a directory
	if(mode_to_filetype(inode->mode) != FT_IFDIR)
	{
		log(LOG_WARN, "ext2_read_directory: This inode isn't a directory "
			"(Is %s [%d])\n", filetype_to_verbose(mode_to_filetype(inode->mode)),
				inode->mode);
		return NULL;
	}

	// TODO
	uint8_t* block = read_inode_block(inode, 0, false);
	if(!block) {
		return NULL;
	}

	block += 0x18; // WHY?

	// TODO Figure out how to find out the num of dirents in a block.
	// separate struct, NULL pointer?

	ext2_dirent_t* dirent = (ext2_dirent_t*)((intptr_t)block);

	// Get offset
	for(int i = 0; i < offset; i++)
		dirent = ((ext2_dirent_t*)((intptr_t)dirent + dirent->record_len));

	// This surely can't be right
	if(*((int*)dirent) == 0 || dirent->name == NULL || dirent->name_len == 0)
		return NULL;

	return dirent;
}

// Traverses directory tree to get inode from path
uint32_t inode_from_path(char* path)
{
	debug("Resolving inode for path %s\n", path);

	// The root directory always has inode 2
	if(unlikely(!strcmp("/", path)))
		return ROOT_INODE;

	// Split path and iterate trough the single parts, going from / upwards.
	char* pch;
	char* sp;

	// Throwaway pointer for strtok_r
	char* path_tmp = (char*)kmalloc((strlen(path) + 1) * sizeof(char));
	strcpy(path_tmp, path);

	pch = strtok_r(path_tmp, "/", &sp);	
	uint32_t current_inode_num = ROOT_INODE;

	while(pch != NULL)
	{
		// Now search the current inode for the searched directory part
		// TODO Maybe use a binary search or something similar here
		for(int i = 0;; i++)
		{
			ext2_dirent_t* dirent = read_dirent(current_inode_num, i);

			// If this dirent is NULL, this means there're no more files
			if(!dirent)
				return 0;

			char* dirent_name = strndup(dirent->name, dirent->name_len);

			// Check if this is what we're searching for
			if(!strcmp(pch, dirent_name))
			{
				current_inode_num = dirent->inode;
				kfree(dirent_name);
				break;
			}

			kfree(dirent_name);
		}

		pch = strtok_r(NULL, "/", &sp);
	}

	kfree(path_tmp);

	debug("Inode for path %s is %d\n", path, current_inode_num);
	return current_inode_num;
}

// Reads multiple inode data blocks at once and create a continuous data stream
uint8_t* read_inode_blocks(ext2_inode_t* inode, uint32_t num)
{
	debug("Reading %d inode blocks for ext2_inode_t* 0x%x\n", num, inode);
	debug("kmalloc = %d\n", superblock_to_blocksize(superblock) * num);

	// TODO Check for valid block numbers
	uint8_t* data = (uint8_t*)kmalloc_a(superblock_to_blocksize(superblock) * num);

	if(!data)
		return NULL;

	for(int i = 0; i < num; i++)
	{
		// TODO Pass buffer to read_inode_block instead of copying the data
		uint8_t* current_block = read_inode_block(inode, i, false);

		if(!current_block)
		{
			debug("read_inode_blocks: read_inode_block for block %d failed\n", i);
			return NULL;
		}

		memcpy(data + superblock_to_blocksize(superblock) * i, 
			current_block, superblock_to_blocksize(superblock));
		kfree(current_block);
	}

	return data;
}

char* ext2_get_verbose_permissions(ext2_inode_t* inode) {
	char* permstring = kmalloc(sizeof(char) * 10);

	permstring[0] = (inode->mode & 0x0100) ? 'r' : '-';
	permstring[1] = (inode->mode & 0x0080) ? 'w' : '-';
	permstring[2] = (inode->mode & 0x0040) ? 'x' : '-';
	permstring[3] = (inode->mode & 0x0020) ? 'r' : '-';
	permstring[4] = (inode->mode & 0x0010) ? 'w' : '-';
	permstring[5] = (inode->mode & 0x0008) ? 'x' : '-';
	permstring[6] = (inode->mode & 0x0004) ? 'r' : '-';
	permstring[7] = (inode->mode & 0x0002) ? 'w' : '-';
	permstring[8] = (inode->mode & 0x0001) ? 'x' : '-';
	permstring[9] = 0;
	return permstring;
}

// The public readdir interface to the virtual file system
char* ext2_read_directory(char* path, uint32_t offset)
{
	uint32_t inode_num = inode_from_path(path);
	if(inode_num < 1)
		return NULL;

	ext2_dirent_t* dirent = read_dirent(inode_num, offset);
	if(!dirent)
		return NULL;

	return strndup(dirent->name, dirent->name_len);
}

// The public read interface to the virtual file system
size_t ext2_read_file(void* dest, uint32_t size, char* path, uint32_t offset)
{
	debug("ext2_read_file for %s, off %d, size %d\n", path, offset, size);

	uint32_t inode_num = inode_from_path(path);
	if(inode_num < 1)
		return NULL;

	ext2_inode_t* inode = read_inode(inode_num);
	if(!inode)
		return NULL;

	debug("%s found at ext2_inode_t* 0x%x\n", path, inode);
	debug("%s uid=%d, gid=%d, size=%d, ft=%s mode=%s\n", path, inode->uid,
		inode->gid, inode->size, filetype_to_verbose(mode_to_filetype(inode->mode)),
		ext2_get_verbose_permissions(inode));

	// Check if this inode is a file
	if(mode_to_filetype(inode->mode) != FT_IFREG)
	{
		log(LOG_WARN, "ext2_read_file: Try to read something weird "
			"(0x%x: %s)\n", inode->mode,
			filetype_to_verbose(mode_to_filetype(inode->mode)));

		return NULL;
	}

	if(inode->size < 1) {
		return NULL;
	}

	if(size > inode->size) {
		debug("ext2_read_file: Attempt to read %d bytes, but file is only %d bytes. Capping.\n", size, inode->size);
		size = inode->size;
	}

	uint32_t num_blocks = (size + offset) / superblock_to_blocksize(superblock);
	if((size + offset) % superblock_to_blocksize(superblock) != 0) {
		num_blocks++;
	}

	void* block = read_inode_blocks(inode, num_blocks);

	if(!block)
		return NULL;

	#ifdef EXT2_DEBUG
		debug("Read file %s offset %d size %d with resulting md5sum of:\n\t", path, offset, size);
		MD5_dump(block + offset, size);
	#endif

	// FIXME This is just a quick hack to make this compatible with the new VFS
	// calls, should be properly rewritten.

	memcpy(dest, block+offset, size);
	return size;
}

void ext2_init()
{
	// The superblock always has an offset of 1024, so is in sector 2 & 3
	superblock = (ext2_superblock_t*)kmalloc(sizeof(ext2_superblock_t));
	read_sector_or_fail(, 0x1F0, 0, 2, (uint8_t*)superblock);
	read_sector_or_fail(, 0x1F0, 0, 3, (uint8_t*)((void*)superblock + 512));

	if(superblock->magic != SUPERBLOCK_MAGIC)
	{
		log(LOG_ERR, "ext2: Invalid magic\n");
		return;
	}

	// Check if the file system is marked as clean
	if(superblock->state != SUPERBLOCK_STATE_CLEAN)
	{
		log(LOG_ERR, "ext2: File system is not marked as clean.\n"
			"Please run fsck.ext2 on it.\n");
		return;
	}

	// TODO Compare superblocks to each other

	log(LOG_INFO, "ext2: Have ext2 revision %d. %d free / %d blocks.\n",
			superblock->revision, superblock->free_blocks,
			superblock->block_count);

	// RO is irrelevant for now since we're read-only anyways.
	//if(superblock->features_incompat || superblock->features_ro)
	if(superblock->features_incompat)
	{
		log(LOG_ERR, "ext2: Sorry, but this filesystem uses some extensions "
			"which I don't support (incompat: 0x%x, ro: 0x%x)\n",
			superblock->features_incompat, superblock->features_ro);
		//return;
	}

	if(superblock->features_compat)
	{
		log(LOG_INFO, "ext2: This file system supports additional special "
			"features. I'll ignore them (0x%x).\n", superblock->features_compat);
	}

	debug("Loaded ext2 file superblock. inode_count=%d, block_count=%d, block_size=%d\n",
		superblock->inode_count, superblock->block_count, superblock_to_blocksize(superblock));

	vfs_mount("/", ext2_read_file, ext2_read_directory);
}