#ifndef FILESYSTEMS_INTERFACE_H
#define FILESYSTEMS_INTERFACE_H

#include <common/generic.h>

typedef struct node {
	uint32 inode;
	char* name;
	struct node* parent;
	char* (*readFunction)(struct node*);
	void (*writeFunction)(struct node*, char*);
	struct node* (*listFunction)(struct node*, struct node*);
	struct node* (*searchFunction)(struct node*, char*);
	int mountPoint;
	int isDir;
	char* fsType;
} node_t;

/// The root directory /.
node_t* fs_root;

char* fs_read(node_t* node);
void fs_write(node_t* node, char* what);
node_t* fs_list(node_t* node, node_t buffer[]);
node_t* fs_search(node_t* start, char* name);
void fs_init();

#endif
