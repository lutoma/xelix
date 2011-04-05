/* argparser.c: Parse kernel command-line arguments
 * Copyright © 2011 Lukas Martini
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include "argparser.h"

#include "log.h"
#include "string.h"
#include <memory/kmalloc.h>

char** arguments = NULL;
char** values = NULL;
uint32_t parts = 1;
bool initialized = false;

/* Retrieve a single argument's key. Returns:
 * Key: Argument exists and has key
 * NULL: Argument exists, but has no key
 * -1: Argument doesn't exist (And probably also doesn't have a key ;) )
 * -2: Not initialized yet or nor arguments at all
 */
char* argparser_get(char* key)
{
	if((int)arguments == NULL || (int)values == NULL || parts == 0 || !initialized)
		return (char*)-2;
	
	/* Fixme: Find a solution which doesn't require this code to iterate
	 * through the whole array.
	 */
	int i;
	for(i = 0; i < parts; i++)
		if(!strcmp(key, arguments[i]))
			return values[i];
		
	return (char*)-1;
}

void argparser_init(char* commandLine)
{
	if((int)commandLine == NULL || strlen(commandLine) < 1)
		return;

	log("argparser: Starting to parse\n");

	/* Strtok_r destroys the source string. As we don't want that, copy
	 * it twice (one for first run, two for second run).
	 */
	uint32_t size = strlen(commandLine) * sizeof(char);

	char* one = (char*)kmalloc(size);
	char* two = (char*)kmalloc(size);

	if((int)one == NULL || (int)two == NULL)
		panic("Unable to allocate!\n");

	strcpy(one, commandLine);
	strcpy(two, commandLine);
	
	// Firstly, find out how many parts we have
	char* pch;
	char* sp;
	
	pch = strtok_r(one, " ", (char**)&sp);
	while(strtok_r(NULL, " ", (char**)&sp) != NULL)
		parts++;

	kfree(one);
	
	log("argparser: Counted %d parts\n", parts);

	// Allocate arrays
	arguments = (char**)kmalloc(parts * sizeof(uint32_t));
	values = (char**)kmalloc(parts * sizeof(uint32_t));
	
	if((int)arguments == NULL || (int)values == NULL)
	{
		log("argparser: Warning: Could not allocate array, leaving NULL.\n");
		return;
	}	
	
	// Strtok again and now actually fill array.
	
	// We can reuse the string we used last time, but 'empty' it.
	sp[0] = 0;
	// Like sp, only for use in the loop.
	char* spt;
	
	pch = strtok_r(two, " ", (char**)&sp);
	
	int i;
	for(i = 0; pch != NULL; i++)
	{
		spt[0] = 0;
		
		arguments[i] = strtok_r(pch, "=", (char**)&spt);
		values[i] = strtok_r(NULL, "=", (char**)&spt);
		
		// Next
		pch = strtok_r(NULL, " ", (char**)&sp);
	}
	
	kfree(two);
	initialized = true;
}
