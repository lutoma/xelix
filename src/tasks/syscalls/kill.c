/* open.c: Kill Syscall
 * Copyright © 2013-2015 Lukas Martini
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
#include <fs/vfs.h>
#include <log.h>
#include <tasks/scheduler.h>

// Only supports killing the current task so far

char* signal_names[] = {
	"SIGHUP",	/* hangup */
	"SIGINT",	/* interrupt */
	"SIGQUIT",	/* quit */
	"SIGKILL",	/* illegal instruction (not reset when caught) */
	"SIGTRAP",	/* trace trap (not reset when caught) */
	"SIGIOT",	/* IOT instruction */
	"SIGABRT",	/* used by abort, replace SIGIOT in the future */
	"SIGEMT",	/* EMT instruction */
	"SIGFPE",	/* floating point exception */
	"SIGKILL",	/* kill (cannot be caught or ignored) */
	"SIGBUS",	/* bus error */
	"SIGSEGV",	/* segmentation violation */
	"SIGSYS",	/* bad argument to system call */
	"SIGPIPE",	/* write on a pipe with no one to read it */
	"SIGALRM",	/* alarm clock */
	"SIGTERM",	/* software termination signal from kill */
};

/* Return codes:
 * -1 == ENOSYS
 * -2 == EINVAL (Invalid sig)
 * -3 == EPERM (Permission denied)
 * -4 == ESRCH (No such proc)
 */
SYSCALL_HANDLER(kill)
{
	int pid = syscall.params[0];
	int sig = syscall.params[1];

	if(pid != syscall.task->pid)
		return -1;

	char* sig_name = signal_names[sig];
	if(!sig_name)
		sig_name = "Unknown";

	log(LOG_INFO, "tasks: PID %d was killed by %s from PID %d\n", pid, sig_name, syscall.task->pid);

	scheduler_terminate_current();
	return 0;
}
