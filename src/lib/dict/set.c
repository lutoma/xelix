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

void dict_set(dict_t* dict, void* key, void* value)
{
	debug("Setting 0x%x->0x%x in dict at 0x%x\n", (uint32_t)key, (uint32_t)value, (uint32_t)dict);

	entry_t* entry = _findEntry(dict, key);
	if(entry == NULL)
	{
		debug("No such entry 0x%x, creating new one\n", (uint32_t)key);

		// Create a new entry
		entry = (entry_t*)malloc(sizeof(entry_t));
	}

	if(dict->firstEntry == NULL)
	{
		// This is the very first entry
		debug("Setting 0x%x as first entry for dict 0x%x\n", (uint32_t)key, (uint32_t)dict);
		dict->firstEntry = entry;
		dict->lastEntry = entry;
	} else
	{
		// Append the entry to the list
		dict->lastEntry->next = entry;
		dict->lastEntry = entry;
	}

	entry->key = key;
	entry->value = value;
	entry->next = NULL;
	entry->parent = dict;
}
