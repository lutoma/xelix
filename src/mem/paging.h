#pragma once

/* Copyright © 2011-2020 Lukas Martini
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

#include <stdbool.h>

#define PAGE_SIZE 0x1000

struct page {
	bool present:1;
	bool rw:1;
	bool user:1;
	bool write_through:1;
	bool cache_disabled:1;
	bool accessed:1;
	// Or page size for dir entries
	bool dirty:1;
	bool global:1;

	uint8_t _unused:4;

	uint32_t frame:20;
};

struct paging_context {
	struct page dir_entries[1024];
};

struct vmem_range;
void paging_set_range(struct paging_context* ctx, struct vmem_range* range);
void paging_rm_context(struct paging_context* ctx);
void paging_init(void* hwdata);
