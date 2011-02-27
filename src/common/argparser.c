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

#include <common/log.h>
#include <common/string.h>
#include <memory/kmalloc.h>

/* FIXME: Unfinished due to the lack of a strtok_r. Going to implement
 * it. --Lukas
 */

char** arguments = NULL;
char** values = NULL;

char* argparser_get(char* key)
{
	
}

void argparser_init(char* commandLine)
{
	if(commandLine == NULL || *commandLine == "")
		return;

	log("argparser: Starting to parse\n");

	// Strtok destroys the source string. As we don't want that, copy it
	uint32 size = strlen(commandLine) * sizeof(char);

	//char* one = kmalloc(size);
	char* one = strcpy((char*) kmalloc(size), commandLine);
	char* two = strcpy((char*) kmalloc(size), commandLine);
	
	// Firstly, find out how many parts we have
	static char* pch;
	static int parts = 1;
	pch = strtok(one, " ");
	
	while(strtok(NULL, " ") != NULL)
		parts++;
	
	kfree(one);
	
	log("argparser: Counted %d parts\n", parts);
	if(parts < 1)
		return;

	// Allocate arrays
	arguments = kmalloc(parts * sizeof(uint32));
	values = kmalloc(parts * sizeof(uint32));
	
	if((int)arguments == NULL || (int)values == NULL)
	{
		log("argparser: Warning: Could not allocate array, leaving NULL.\n");
		return;
	}	
	
	// Strtok again and now actually fill array.
	pch = strtok(two, " ");

	for(i = 0; pch != NULL; i++)
	{
		pch[strlen(pch) -1] = 0; // No clue why, but works.
		arguments[i] = pch;
		values[i] = pch;
		
		// Next
		pch = strtok(NULL, " ");
	}
	
	kfree(two);
		
	for(i=0; i < parts; i++)
		log("argparser: plbk: %s = %s\n", arguments[i], values[i]);
}
