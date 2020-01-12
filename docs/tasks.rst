Tasks
*****

A single running process of a program is referred to as `task` within the Xelix kernel. All runtime data concerning a task is stored in a `task_t` struct (:file:`src/tasks/task.h`). It contains things like the PID, task state, memory allocations, as well as an `isf_t` (Interrupt stack frame) with the processor register state.

Task states
===========

Tasks have a number of states they can be in over their lifetime:

+-------------------------+-------------------------------------------------------------------------+
| TASK_STATE_RUNNING      | Task is running                                                         |
+-------------------------+-------------------------------------------------------------------------+
| TASK_STATE_SYSCALL      | Task is currently in a syscall                                          |
+-------------------------+-------------------------------------------------------------------------+
| TASK_STATE_WAITING      | Task has called `wait(pid)` syscall                                     |
+-------------------------+-------------------------------------------------------------------------+
| TASK_STATE_STOPPED      | `SIGSTOP` has been received for the task, task is paused                |
+-------------------------+-------------------------------------------------------------------------+
| TASK_STATE_TERMINATED   | Killed/exited, used regardless of specific signal/exit reason.          |
|                         | Tasks will only be in this state briefly: After task termination,       |
|                         | but before the scheduler has called `task_userland_eol`. Once that has  |
|                         | happened, the task switches to `TASK_STATE_ZOMBIE`.                     |
+-------------------------+-------------------------------------------------------------------------+
| TASK_STATE_ZOMBIE       | Task has been killed and `task_userland_eol` has run, but the parent    |
|                         | process hasn't called waitpid yet. Once that happens, the task switches |
|                         | to `TASK_STATE_REAPED` and will be deallocated.                         |
+-------------------------+-------------------------------------------------------------------------+
| TASK_STATE_REAPED       | Terminated task, parent has run waitpid. Should be ignored when         |
|                         | iterating the task list and will be removed by the scheduler            |
|                         | at its next invocation.                                                 |
+-------------------------+-------------------------------------------------------------------------+
| TASK_STATE_REPLACED     | Task has been replaced by a different task with an identical PID.       |
|                         | This is an artifact of the way execve is implemented in Xelix right     |
|                         | now.                                                                    |
+-------------------------+-------------------------------------------------------------------------+


Task initialization
===================

A new `task_t` struct can be created using the `task_new` function from :file:`src/tasks/task.h`:

.. code-block:: c

   task_t* task_new(task_t* parent, uint32_t pid, char name[TASK_MAXNAME],
       char** environ, uint32_t envc, char** argv, uint32_t argc);

This will create a memory context and allocate resources for the task. When this is done, a binary can be loaded into task memory using

.. code-block:: c

   int elf_load_file(task_t* task, char* path)

This loads the program headers from the ELF file and maps them into memory as requested. It also sets the absolute path and task permissions (when setuid is set) in the task struct.

The task can then be added to the scheduler (:file:`src/tasks/scheduler.h`), at which point it starts running:

.. code-block:: c

   void scheduler_add(task_t* task)

This manual process of adding a task is only used once in the kernel in :file:`src/boot/init.c` to start PID 1. All other programs are usually started using the `execve` syscall (implemented by `task_execve` in :file:`src/tasks/task.c`), which handles all of the steps above.

Task exit
=========

An exiting task is deallocated in a three-step process. When the task (or crt0) calls the exit syscall, the `task_exit` handler in :file:`src/tasks/task.c` sets the task state to `TASK_STATE_TERMINATED`.

During one of the next scheduler cycles, the scheduler will call `task_userland_eol` for the task. This will terminate the task from the userland perspective: If a parent task exists, it receives a SIGCHLD signal, lingering children will be reassigned to init, etc. The task state changes to `TASK_STATE_ZOMBIE`.

At this point, the kernel is waiting for the parent task to retrieve the exit status by invoking waitpid or similar. Until then, all data structures of the task are kept in memory.

As soon as the exit status has been retrieved the task state changes to `TASK_STATE_REAPED`, and the scheduler removes the task from the linked list and invokes `task_cleanup`, which frees the task's memory allocations.

Task memory
===========

Task memory allocations are stored in a linked list of `struct task_mem` in :file:`src/tasks/mem.c`. Memory can be mapped into the task address space using

.. code-block:: c

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

+----------------------+---------------------------+------------+------------+
| Type                 | Description               | User read  | User write |
+======================+===========================+============+============+
| TMEM_SECTION_NONE    | Don't map memory          | No         | No         |
+----------------------+---------------------------+------------+------------+
| TMEM_SECTION_STACK   | Initial stack             | Yes        | Yes        |
+----------------------+---------------------------+------------+------------+
| TMEM_SECTION_CODE    | Program code              | Yes        | No         |
+----------------------+---------------------------+------------+------------+
| TMEM_SECTION_DATA    | Static data               | Yes        | No         |
+----------------------+---------------------------+------------+------------+
| TMEM_SECTION_HEAP    | sbrk allocations          | Yes        | Yes        |
+----------------------+---------------------------+------------+------------+
| TMEM_SECTION_KERNEL  | Kernel memory             | No         | No         |
+----------------------+---------------------------+------------+------------+

Stack
=====

The task stack is initialized in :file:`src/task/mem.c`. The default stack for Xelix tasks is one page long and located at `TASK_STACK_LOCATION` (currently 0xc0000000). The pages below the current stack are intentionally left unmapped.

Task stacks on Xelix dynamically grow: As soon as a task reaches the lower bound of the allocated area, a page fault is generated by the CPU and intercepted by the task memory management code. Additional pages are then mapped below the stack to increase its size, and control is returned to the program at the instruction before the page fault.

Execdata
========

During task creation, Xelix creates a single page that is always mapped to a hard-coded location of `0x5000` in userspace memory. This page contains runtime information for the task such as its PID, the parent PID, arguments, environment variables etc. This data is used by the crt0 to invoke the tasks's main() function, and to implement stdlib functions like getpid() or getppid() without the need for a syscall.

It might also be possible to just put this on the task stack, but so far this approach has worked well.

Signals
=======

Task

`task_sigjmp_crt0`

Scheduler
=========

Xelix uses a very simplistic round-robin scheduler.


SysFS integration
=================

Tasks and the scheduler are integrated into SysFS. :file:`/sys/tasks` returns a list of all tasks loaded by the scheduler and a bit of basic information on each. This is used by the xelix-utils ps command.

:file:`/sys/task<pid>` contains more detailed information on a task, including open files and memory mappings.
