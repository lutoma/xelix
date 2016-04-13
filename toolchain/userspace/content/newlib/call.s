# call.s: Syscall
# Copyright © 2013-2015 Lukas Martini

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

.global call_open
call_open:
    mov eax, 13
    mov ebx, [esp + 4]
    int 0x80
    ret

.global call_read
call_read:
    mov eax, 2
    mov ebx, [esp + 4]
    mov ecx, [esp + 8]
    mov edx, [esp + 12]
    int 0x80
    ret

.global call_write
call_write:
    mov eax, 3
    mov ebx, [esp + 4]
    mov ecx, [esp + 8]
    mov edx, [esp + 12]
    int 0x80
    ret

.global call_exit
call_exit:
    mov eax, 1
    mov ebx, [esp + 4]
    int 0x80
    ret

.global call_getpid
call_getpid:
    mov eax, 4
    int 0x80
    ret

.global call_brk
call_brk:
    mov eax, 5
    mov ebx, [esp + 4]
    int 0x80
    ret

.global call_mmap
call_mmap:
    mov eax, 7
    mov ecx, [esp + 4]
    int 0x80
    ret

.global call_seek
call_seek:
    mov eax, 14
    mov ebx, [esp + 4]
    mov ecx, [esp + 8]
    mov edx, [esp + 12]
    int 0x80
    ret

.global call_kill
call_kill:
    mov eax, 18
    mov ebx, [esp + 4]
    mov ecx, [esp + 8]
    int 0x80
    ret

.global call_chdir
call_chdir:
    mov eax, 20
    mov ebx, [esp + 4]
    int 0x80
    ret

.global call_getcwd
call_getcwd:
    mov eax, 21
    mov ebx, [esp + 4]
    mov ecx, [esp + 8]
    int 0x80
    ret

.global call_fork
call_fork:
    mov eax, 22
    int 0x80
    ret

.global call_execve
call_execve:
    mov eax, 23
    mov ebx, [esp + 4]
    mov ecx, [esp + 8]
    mov edx, [esp + 12]
    int 0x80
    ret

.global call_socket
call_socket:
    mov eax, 24
    mov ebx, [esp + 4]
    mov ecx, [esp + 8]
    mov edx, [esp + 12]
    int 0x80
    ret

.global call_bind
call_bind:
    mov eax, 25
    mov ebx, [esp + 4]
    mov ecx, [esp + 8]
    mov edx, [esp + 12]
    int 0x80
    ret

.global call_socket_send
call_socket_send:
    mov eax, 26
    mov ebx, [esp + 4]
    mov ecx, [esp + 8]
    mov edx, [esp + 12]
    int 0x80
    ret

.global call_socket_recv
call_socket_recv:
    mov eax, 27
    mov ebx, [esp + 4]
    mov ecx, [esp + 8]
    mov edx, [esp + 12]
    int 0x80
    ret

.global call_wait
call_wait:
    mov eax, 29
    mov ebx, [esp + 4]
    int 0x80
    ret

.global call_execnew
call_execnew:
    mov eax, 28
    mov ebx, [esp + 4]
    mov ecx, [esp + 8]
    mov edx, [esp + 12]
    int 0x80
    ret
