# Tasks

A single running process of a program is referred to as `task` within the Xelix kernel. All runtime data concerning a task is stored in a `task_t` struct (`src/tasks/task.h`). It contains things like the PID, task state, memory allocations, as well as an `isf_t` (Interrupt stack frame) with the processor register state.

## States

Tasks have a number of states they can be in over their lifetime:

 State name              | Description
-------------------------|-------------------------------------------------------------------------
 TASK_STATE_RUNNING      | Task is running
 TASK_STATE_SYSCALL      | Task is currently in a syscall
 TASK_STATE_WAITING      | Task has invoked [wait](https://pubs.opengroup.org/onlinepubs/9699919799/functions/wait.html) syscall
 TASK_STATE_STOPPED      | A [SIGSTOP](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/signal.h.html) signal has been received for the task
 TASK_STATE_TERMINATED   | Killed/exited, used regardless of specific signal/exit reason. Tasks will only be in this state briefly: After task termination, but before the scheduler has called `task_userland_eol`. Once that has happened, the task switches to `TASK_STATE_ZOMBIE`.
 TASK_STATE_ZOMBIE       | Task has been killed and `task_userland_eol` has run, but the parent process hasn't called waitpid yet. Once that happens, the task switches to `TASK_STATE_REAPED` and will be deallocated.
 TASK_STATE_REAPED       | Terminated task, parent has run waitpid. Should be ignored when iterating the task list and will be removed by the scheduler at its next invocation.
 TASK_STATE_REPLACED     | Task has been replaced by a different task with an identical PID. This is an artifact of the way execve is implemented in Xelix right now.


## Initialization

A new `task_t` struct can be created using the `task_new` function from `src/tasks/task.h`:

	#!c
	task_t* task_new(task_t* parent, uint32_t pid, char name[TASK_MAXNAME],
		char** environ, uint32_t envc, char** argv, uint32_t argc);

This will create a memory context and allocate resources for the task. When this is done, a binary can be loaded into task memory using

	#!c
	int elf_load_file(task_t* task, char* path)

This loads the program headers from the ELF file and maps them into memory as requested. It also sets the absolute path and task permissions (when setuid is set) in the task struct.

The task can then be added to the scheduler (`src/tasks/scheduler.h`), at which point it starts running:

	#!c
	void scheduler_add(task_t* task)

This manual process of adding a task is only used once in the kernel in `src/boot/init.c` to start PID 1. All other programs are usually started using the `execve` syscall (implemented by `task_execve` in `src/tasks/task.c`), which handles all of the steps above.

## Exit

An exiting task is deallocated in a three-step process. When the task (or crt0) calls the exit syscall, the `task_exit` handler in `src/tasks/task.c` sets the task state to `TASK_STATE_TERMINATED`.

During one of the next scheduler cycles, the scheduler will call `task_userland_eol` for the task. This will terminate the task from the userland perspective: If a parent task exists, it receives a SIGCHLD signal, lingering children will be reassigned to init, etc. The task state changes to `TASK_STATE_ZOMBIE`.

At this point, the kernel is waiting for the parent task to retrieve the exit status by invoking waitpid or similar. Until then, all data structures of the task are kept in memory.

As soon as the exit status has been retrieved the task state changes to `TASK_STATE_REAPED`, and the scheduler removes the task from the linked list and invokes `task_cleanup`, which frees the task's memory allocations.

## Memory management

Task memory allocations are stored in a linked list of `struct task_mem` in `src/tasks/mem.c`. Memory can be mapped into the task address space using

	#!c
	void task_add_mem(task_t* task, void* virt_start, void* phys_start,
		uint32_t size, enum task_mem_section section, int flags);

	// If virt_start and phys_start are equal, task_add_mem_flat can be used
	void task_add_mem_flat(task_t* task, void* start,
		uint32_t size, enum task_mem_section section, int flags);

The memory region that `phys_start` points to already needs to be allocated, for example using `kmalloc`.

Available flags:

* **TASK_MEM_FORK** Copy this memory region when the task is forked
* **TASK_MEM_FREE** Automatically deallocate this memory region when task is removed

The section type determines the access privileges the task will have on the memory and can be one of

 Type                 | Description               | User read  | User write
----------------------|---------------------------|------------|------------
 TMEM_SECTION_NONE    | Don't map memory          | No         | No
 TMEM_SECTION_STACK   | Initial stack             | Yes        | Yes
 TMEM_SECTION_CODE    | Program code              | Yes        | No
 TMEM_SECTION_DATA    | Static data               | Yes        | No
 TMEM_SECTION_HEAP    | sbrk allocations          | Yes        | Yes
 TMEM_SECTION_KERNEL  | Kernel memory             | No         | No

## Stack

The task stack is initialized in `src/task/mem.c`. The default stack for Xelix tasks is one page long and located at `TASK_STACK_LOCATION` (currently 0xc0000000). The pages below the current stack are intentionally left unmapped.

Task stacks on Xelix dynamically grow: As soon as a task reaches the lower bound of the allocated area, a page fault is generated by the CPU and intercepted by the task memory management code. Additional pages are then mapped below the stack to increase its size, and control is returned to the program at the instruction before the page fault.

## Execdata

During task creation, Xelix creates two pages that are always mapped to a hard-coded location of `0x5000` in userspace memory. These contain runtime information for the task such as its PID, the parent PID, arguments, environment variables etc. This data is used by the crt0 to invoke the tasks's main() function, and to implement stdlib functions like getpid() or getppid() without the need for a syscall.

It might also be possible to just put this on the task stack, but so far this approach has worked well.

## Syscalls

All syscalls in Xelix use interrupt `0x80`, which is registered during boot by `src/tasks/syscall.c`. All syscalls are dispatched by the `int_handler` function, which looks up the correct handler in the syscall table, copies userland buffers to kernel memory, and logs the call if strace is enabled.

The signature for syscall callbacks is

	#!c
	uint32_t (*syscall_cb)(task_t* task, [isf_t* state], [0 to 3 arguments])

state may or may not be passed depending on global flags, and the number and type of arguments depends on the argument flags (see below).

### Syscall table

Syscalls are defined in the syscall table in `src/tasks/syscalls.h` using entries of the format


	#!c
	{"name", callback, flags, arg0_flags, arg1_flags, arg2_flags, ptr_size}

name
:	A syscall name for debugging purposes

callback
:	Function to call when syscall is invoked

flags
:	Syscall flags

arg[0-2]\_flags
:	Argument type and flags. Set to 0 to mark argument as unused (will not be passed to callback then)

ptr_size
:	Default size for SCA_POINTER arguments

The only currently defined syscall flag is **SCF_STATE**, in which case the Interrupt Stack Frame `isf_t` is passed to the callback as second argument. By default, it is not passed.

In addition, each argument has an individual type/flags field. If this field is 0, the corresponding argument is ignored (and the callback will be called without it). Available argument types:

 Type         | Desc                            | Translate memory | Debug representation
--------------|---------------------------------|------------------|---------------------
 SCA_INT      | Integer                         | No               | %d
 SCA_POINTER  | Pointers / Buffers              | Yes              | %#x
 SCA_STRING   | String buffers                  | Yes              | %s

If translate is `Yes`, the syscall handler will map the argument pointer from user memory to kernel memory, or fail the syscall if not possible. The type may be OR'd with the following flags to control translation behaviour:

SCA_NULLOK
:	Normally when translating a pointer, a value of NULL is rejected. With this, NULL will be passed 1:1.

SCA_SIZE_IN_0
:	Size of the buffer is passed in argument 0

SCA_SIZE_IN_1
:	Size of the buffer is passed in argument 1

SCA_SIZE_IN_2
:	Size of the buffer is passed in argument 2

If none of SCA_SIZE_IN_x are passed, the default size from ptr_size is used.

### strace

xelix has basic strace facilities using the `strace` syscall. This syscall works like fork(), except it returns a file descriptor from which the syscalls invoked by the child process can be read in the following format:

	#!c
	struct strace {
		uint32_t call;
		uint32_t result;
		uint32_t errno;
		uintptr_t args[3];
		char ptrdata[3][0x50];
	};

If the argument is marked as a pointer in the syscall table (see above), ptrdata contains the first 0x50 bytes of the buffer.
Internally, the strace syscall opens a pipe, assigns the write end of it to `task->strace_fd` for the child task, and returns the read end to the parent task.

xelix-utils includes an `strace` binary that uses this API.

## SysFS integration

Tasks and the scheduler are integrated into SysFS. `/sys/tasks` returns a list of all tasks loaded by the scheduler and a bit of basic information on each. This is used by the xelix-utils ps command.

`/sys/task<pid>` contains more detailed information on a task, including open files and memory mappings.
