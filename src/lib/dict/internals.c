/* Copyright © 2011 Lukas Martini
 *
 * This file is part of Libkdict.
 *
 * Libkdict is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Libkdict is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Libkdict. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "dict.h"
#include "internals.h"

// Find an entry. Returns NULL if there's no such entry.
entry_t* _findEntry(dict_t* dict, char* key)
{
	entry_t* entry = dict->firstEntry;
	while(entry != NULL)
	{
		if(entry->key == key)
			break;
	
		entry = entry->next;
	}
	
	return entry;
}

// Delete an entry. Stub so far.
void _deleteEntry(entry_t* entry)
{
	if(entry == NULL)
		return;
}
