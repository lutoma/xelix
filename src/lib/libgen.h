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
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

static inline char* basename(char* path) {
	if(!path || !*path) {
		return ".";
	}

	if(!strcmp(path, "/")) {
		return path;
	}

	char* name = strrchr(path, '/');
	return name ? name + 1 : path;
}


static inline char* dirname(char* path) {
	if(!path) {
		return ".";
	}

	if(!strcmp(path, "/")) {
		return path;
	}

	char* end = strrchr(path, '/');
	if(!end) {
		return ".";
	}

	*end = '\0';
	return path;
}
