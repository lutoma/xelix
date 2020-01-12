Syscalls
********

All syscalls in Xelix use interrupt 0x80, which is registered during boot by :file:`src/tasks/syscall.c`. All syscalls are dispatched by the `int_handler` function, which looks up the correct handler in the syscall table, copies userland buffers to kernel memory, and logs the call if strace is enabled.

The signature for syscall callbacks is

.. code-block:: c

   uint32_t (*syscall_cb)(task_t* task, [isf_t* state], [0 to 3 arguments])

state may or may not be passed depending on global flags, and the number and type of arguments depends on the argument flags (see below).

Syscall table
=============

Syscalls are defined in the syscall table in :file:`src/tasks/syscalls.h` using entries of the format

.. code-block:: c

   {"name", callback, flags,
       arg0_flags, arg1_flags, arg2_flags, ptr_size},

* **name** A syscall name for debugging purposes
* **callback** Function to call when syscall is invoked
* **flags** Syscall flags
* **arg[0-2]_flags** Argument type and flags. Set to 0 to mark argument as unused (will not be passed to callback then)
* **ptr_size** Default size for SCA_POINTER arguments

The only currently defined syscall flag is **SCF_STATE**, in which case the Interrupt Stack Frame `isf_t` is passed to the callback as second argument. By default, it is not passed.

In addition, each argument has an individual type/flags field. If this field is 0, the corresponding argument is ignored (and the callback will be called without it). Available argument types:

+--------------+---------------------------------+-----------+------------+
| Type         + Desc                            | Translate | Debug repr |
+==============+=================================+===========+============+
| SCA_INT      | Integer                         | No        | %d         |
+--------------+---------------------------------+-----------+------------+
| SCA_POINTER  | Pointers / Buffers              | Yes       | %#x        |
+--------------+---------------------------------+-----------+------------+
| SCA_STRING   | String buffers                  | Yes       | %s         |
+--------------+---------------------------------+-----------+------------+

If translate is `Yes`, the syscall handler will map the argument pointer from user memory to kernel memory, or fail the syscall if not possible. The type may be OR'd with the following flags to control translation behaviour:

* **SCA_NULLOK** Normally when translating a pointer, a value of NULL is rejected. With this, NULL will be passed 1:1.
* **SCA_SIZE_IN_0** Size of the buffer is passed in argument 0
* **SCA_SIZE_IN_1** Size of the buffer is passed in argument 1
* **SCA_SIZE_IN_2** Size of the buffer is passed in argument 2

If none of SCA_SIZE_IN_x are passed, the default size from ptr_size is used.

strace
=======

xelix has basic strace facilities using the `strace` syscall. This syscall works like fork(), except it returns a file descriptor from which the syscalls invoked by the child process can be read in the following format:

.. code-block:: c

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
