#pragma once

/* Copyright © 2023 Lukas Martini
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

#include <stdint.h>
#include <fs/vfs.h>
#include <int/int.h>

typedef struct worker {
	char name[VFS_NAME_MAX];
	bool stopped;
	isf_t* state;
	void* entry;
	void* stack;
} worker_t;

worker_t* worker_new(char* name, void* entry);
int worker_stop(worker_t* worker);
int worker_exit(worker_t* worker);
