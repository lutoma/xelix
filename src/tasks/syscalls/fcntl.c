/* fcntl.c: fcntl syscall
 * Copyright © 2019 Lukas Martini
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

#include <tasks/syscall.h>
#include <errno.h>
#include <print.h>

#define	F_DUPFD		0	/* Duplicate fildes */
#define	F_GETFD		1	/* Get fildes flags (close on exec) */
#define	F_SETFD		2	/* Set fildes flags (close on exec) */
#define	F_GETFL		3	/* Get file flags */
#define	F_SETFL		4	/* Set file flags */
#define	F_GETOWN 	5	/* Get owner - for ASYNC */
#define	F_SETOWN 	6	/* Set owner - for ASYNC */
#define	F_GETLK  	7	/* Get record-locking information */
#define	F_SETLK  	8	/* Set or Clear a record-lock (Non-Blocking) */
#define	F_SETLKW 	9	/* Set or Clear a record-lock (Blocking) */
#define	F_RGETLK 	10	/* Test a remote lock to see if it is blocked */
#define	F_RSETLK 	11	/* Set or unlock a remote lock */
#define	F_CNVT 		12	/* Convert a fhandle to an open fd */
#define	F_RSETLKW 	13	/* Set or Clear remote record-lock(Blocking) */
#define	F_DUPFD_CLOEXEC	14	/* As F_DUPFD, but set close-on-exec flag */


SYSCALL_HANDLER(fcntl) {
	if(syscall.params[1] != F_DUPFD) {
		sc_errno = ENOSYS;
		return -1;
	}

	vfs_file_t* fp = vfs_get_from_id(syscall.params[0], syscall.task);
	if(!fp) {
		sc_errno = EBADF;
		return -1;
	}

	uint32_t fileno2 = syscall.params[2];
	vfs_file_t* nfile;
	//if(fileno2) {
		nfile = &syscall.task->files[fileno2];
	//} else {
	//	nfile = vfs_alloc_fileno(syscall.task);
	//	fileno2 = nfile->num;
	//}

	memcpy(nfile, fp, sizeof(vfs_file_t));
	nfile->num = fileno2;
	return 0;
}
