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

#ifndef _BITS_WORDSIZE_H_
#define _BITS_WORDSIZE_H_

// This is silly as we only support 32 bits anyways. Future compatibility I guess
#if defined __x86_64__
# define __WORDSIZE     64
# define __WORDSIZE_COMPAT32    1
#else
# define __WORDSIZE     32
#endif

#endif /* _BITS_WORDSIZE_H */
