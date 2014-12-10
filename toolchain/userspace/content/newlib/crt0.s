# crt0.s: Initial execution code for Xelix binaries
# Copyright © 2013 Lukas Martini

# This file is part of Xelix.
#
# Xelix is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Xelix is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Xelix. If not, see <http://www.gnu.org/licenses/>.

.intel_syntax noprefix

.extern main
.extern call_exit
.extern environ

.global _start
_start:
	# Get argv, argc, environ
	mov eax, 19
	mov ebx, 2
	int 0x80
	mov environ, eax

	mov eax, 19
	mov ebx, 1
	int 0x80
	push eax

	mov eax, 19
	mov ebx, 0
	int 0x80
	push eax

    call main

    add esp, 8
    call call_exit
