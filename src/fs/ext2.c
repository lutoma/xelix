/* ext2.c: Implementation of the extended filesystem, version 2
 * Copyright © 2013 Lukas Martini
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
#include <memory/kmalloc.h>
#include <hw/ide.h>
#include <fs/vfs.h>
#include "ext2.h"

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
char* verbose_filetypes[] = {NULL, "IFIFO", "IFCHR", NULL, "IFDIR", NULL, \
	NULL, "IFBLK", NULL, "IFREG", NULL, "IFLNK", NULL, "IFSOCK"};

#define filetype_to_verbose(t) \
	(verbose_filetypes[((t) > 0xc000 || (t) % 0x1000) ? 0 : (t) / 0x1000])
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

ext2_superblock_t* superblock = NULL;


/* Reads an inode. Takes the number of the inode as argument and locates and
 * returns the corresponding ext2_inode_t*.
 */
static ext2_inode_t* read_inode(uint32_t inode_num)
{
	uint32_t blockgroup_num = inode_to_blockgroup(inode_num);

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

static uint8_t* read_inode_block(ext2_inode_t* inode, uint32_t block_num)
{
	// Todo double & triple linked block support for large files & dirs
	if(block_num > 15)
	{
		// Yay recursion
		//uint8_t* double_block = read_inode_block(inode, 13);

		log(LOG_INFO, "ext2: sorry, no support for doubly & triply linked "
			"blocks. Poke lutoma.\n");
		return NULL;
	}

	// Read block
	intptr_t block_location = inode->blocks[block_num] *
		superblock_to_blocksize(superblock);

	// Allocate correct size + one HDD sector (Since we can only read in 512
	// byte chunks from the disk.
	uint8_t* block = (uint8_t*)kmalloc(superblock_to_blocksize(superblock) + 512);
	for(int i = 0; i < superblock_to_blocksize(superblock); i += 512)
		read_sector_or_fail(0, 0x1F0, 0, block_location / 512 + i, block + i);

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
	uint8_t* block = read_inode_block(inode, 0);

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
	// The root directory always has inode 2
	if(unlikely(!strcmp("/", path)))
		return ROOT_INODE;

	// Split path and iterate trough the single parts, going from / upwards.
	static char* pch;
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

	return current_inode_num;
}

// Reads multiple inode data blocks at once and create a continuous data stream
uint8_t* read_inode_blocks(ext2_inode_t* inode, uint32_t num)
{
	// TODO Check for valid block numbers
	uint8_t* data = (uint8_t*)kmalloc(superblock_to_blocksize(superblock) * num);

	if(!data)
		return NULL;

	for(int i = 0; i <= num; i++)
	{
		// TODO Pass buffer to read_inode_block instead of copying the data
		uint8_t* current_block = read_inode_block(inode, i);

		if(!current_block)
			return NULL;

		memcpy(data + superblock_to_blocksize(superblock) * i, 
			current_block, superblock_to_blocksize(superblock));
		kfree(current_block);
	}

	return data;
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
void* ext2_read_file(char* path, uint32_t offset, uint32_t size)
{
	uint32_t inode_num = inode_from_path(path);
	if(inode_num < 1)
		return NULL;

	ext2_inode_t* inode = read_inode(inode_num);
	if(!inode)
		return NULL;

	// Check if this inode is a file
	if(mode_to_filetype(inode->mode) != FT_IFREG)
	{
		log(LOG_WARN, "ext2_read_file: Try to read something weird "
			"(0x%x: %s)\n", inode->mode,
			filetype_to_verbose(mode_to_filetype(inode->mode)));

		return NULL;
	}

	void* block = read_inode_blocks(inode,
		(size + offset) / superblock_to_blocksize(superblock));

	if(!block)
		return NULL;

	return block + offset;
}

void ext2_init()
{
	// The superblock always has an offset of 1024, so is in sector 2 & 3
	superblock = (ext2_superblock_t*)kmalloc(sizeof(ext2_superblock_t));
	read_sector_or_fail(false, 0x1F0, 0, 2, (uint8_t*)superblock);
	read_sector_or_fail(false, 0x1F0, 0, 3, (uint8_t*)((void*)superblock + 512));

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

	vfs_mount("/", ext2_read_file, ext2_read_directory);
}