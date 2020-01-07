/* Copyright © 2015 Lukas Martini
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

#ifndef _BYTESWAP_H_
#define _BYTESWAP_H_

#include <stdint.h>

static inline uint16_t bswap_16(uint16_t x)
{
    return ((((x) & 0xff00) >> 8) | (((x) & 0xff) << 8));
}

static inline uint32_t bswap_32(uint32_t x)
{
    return
    ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |
     (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24));
}

static inline uint64_t bswap_64(uint64_t x)
{
    return
    ((((x) & 0xff00000000000000ULL) >> 56) |
     (((x) & 0x00ff000000000000ULL) >> 40) |
     (((x) & 0x0000ff0000000000ULL) >> 24) |
     (((x) & 0x000000ff00000000ULL) >>  8) |
     (((x) & 0x00000000ff000000ULL) <<  8) |
     (((x) & 0x0000000000ff0000ULL) << 24) |
     (((x) & 0x000000000000ff00ULL) << 40) |
     (((x) & 0x00000000000000ffULL) << 56));
}

#endif /* _BYTESWAP_H */