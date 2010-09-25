/** @file init/main.c
 * \brief Generic file handling functions
 * This file provices abstraction from the filesystem drivers for normal use.
 * @author Lukas Martini
 */

#include <filesystems/interface.h>

/** Read contents of a node
 * @param node Node to read
 * @return Data of the node
 */
char* fs_read(node_t* node)
{
	if(node->readFunction <= 0) return;
	(*node->readFunction) (node);
	return "foo";
}

/** Write to a specific node
 * @param node Node to write to
 * @param what Data to write to the node
 */
void fs_write(node_t* node, char* what)
{

}

/** Show contents of directory
 * @param node The directory-node to be read
 * @param buffer A buffer array to write to
 * @return Array with file-nodes
 * @bug Only supports 50 items [50]
 */
node_t* fs_list(node_t* node, node_t buffer[])
{

}

/** Search for a specific node by filename
 * @param start Start directory
 * @param name Name to search for
 * @return Node of the node found
 */
node_t* fs_search(node_t* start, char* name)
{

}


/** Initialize the filesystem abstraction system
 */
void fs_init()
{

	
}
