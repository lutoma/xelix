/* list.c: Generic list structure
 * Copyright © 2011 Fritz Grimpen
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

#include "list.h"
#include <memory/kmalloc.h>

struct list
{
	int length;
	struct list_node *first_node;
	struct list_node *last_node;
};

struct list_node
{
	struct list *list;
	struct list_node *next;
	struct list_node *prev;
	void *data;
};

struct list *list_new()
{
	struct list *list = kmalloc(sizeof(struct list));
	list->length = 0;
	list->first_node = NULL;
	list->last_node = NULL;

	return list;
}

struct list *list_alloc(void * (*allocator)(int len))
{
	return allocator(sizeof(struct list));
}

void *list_append(struct list *l, void *data)
{
	l->length++;

	struct list_node *node = kmalloc(sizeof(struct list));
	node->list = l;
	node->data = data;
	node->next = NULL;
	node->prev = l->last_node;

	if (l->last_node != NULL)
		l->last_node->next = node;

	l->last_node = node;
	if (l->first_node == NULL)
		l->first_node = node;

	return data;
}

void *list_prepend(struct list *l, void *data)
{
	l->length++;

	struct list_node *node= kmalloc(sizeof(struct list));
	node->list = l;
	node->data = data;
	node->next = l->first_node;
	node->prev = NULL;

	if (l->first_node != NULL)
		l->first_node->next = node;

	l->first_node = node;
	if (l->last_node == NULL)
		l->last_node = node;

	return data;
}

void *list_get(struct list *l, int offset)
{
	if (offset >= l->length)
		return NULL;

	struct list_node *current_node = l->first_node;
	int count = 0;
	while (current_node != NULL && count != offset)
	{
		current_node = current_node->next;
		count++;
	}

	return current_node->data;
}

