#pragma once

/* Copyright © 2018 Lukas Martini
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
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <print.h>

#define sysfs_printf(args...) rsize += snprintf(dest + rsize, size - rsize, args);

typedef size_t (*sysfs_read_callback_t)(void* dest, size_t size, size_t offset, void* meta);
typedef size_t (*sysfs_write_callback_t)(void* data, size_t size, size_t offset, void* meta);

struct sysfs_file {
	char name[40];
	sysfs_read_callback_t read_cb;
	sysfs_write_callback_t write_cb;
	void* meta;
	struct sysfs_file* next;
	struct sysfs_file* prev;
};

struct sysfs_file* sysfs_add_file(char* name, sysfs_read_callback_t read_cb, sysfs_write_callback_t write_cb);
struct sysfs_file* sysfs_add_dev(char* name, sysfs_read_callback_t read_cb, sysfs_write_callback_t write_cb);
void sysfs_rm_file(char* name);
void sysfs_rm_dev(char* name);
void sysfs_init();
