#pragma once

/* Copyright © 2011 Fritz Grimpen
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

#include <generic.h>

typedef struct list list_t;

struct list *list_new();
struct list *list_alloc(void * (*allocator)(int len));

void *list_append(struct list *l, void *data);
void *list_prepend(struct list *l, void *data);
void *list_insert(struct list *l, void *data, int offset);

void *list_get(struct list *l, int offset);

void **list_flush(struct list *l, int offset, void **data, int len);
void *list_delete(struct list *l, int offset);
void **list_del_range(struct list *l, int offset, int length, void **data, int len);

void list_destroy(struct list *l);
void list_clean(struct list *l);
