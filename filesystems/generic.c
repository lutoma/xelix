// This file provices abstraction from the filesystem drivers for normal use.

#include <filesystems/interface.h>
#include <memory/kmalloc.h>

// Read contents of a node
char* fs_read(node_t* node)
{
	if(node->readFunction <= 0) return;
	(*node->readFunction) (node);
	return "foo";
}

// Write to a specific node
void fs_write(node_t* node, char* what)
{

}

// Show contents of directory
// buffer A buffer array to write to
// returns array with file-nodes
// Only supports 50 items [50]
//
node_t* fs_list(node_t* node, node_t buffer[])
{

}

// Search for a specific node by filename
// returns found node
//
node_t* fs_search(node_t* start, char* name)
{

}


// Initialize the filesystem abstraction system
void fs_init()
{
	fs_root = kmalloc(sizeof(node_t));
	log("Position of filesystem root node in memory: ");
	logHex(fs_root);
	log("\n");

	ASSERT(fs_root > 0); // Now check for fs_root

	fs_root->isDir = 1;

	log("Initialized filesystem abstraction\n");
}
