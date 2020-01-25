# File system

!!! note
	The virtual file system is currently under more or less active refactoring. Some of the information here may be outdated by the time you read this.

File manipulation functions are defined in `src/fs/vfs.c`. These are prefixed with `vfs_` and include all the common POSIX operations such as vfs_open, vfs_read, vfs_write, vfs_poll, etc.

The first argument is always a `task_t*` to pass the calling task for permission checks and such. The remaining arguments are usually the same as the corresponding C standard library function.

These functions are used as syscall handlers, but can also be called directly from kernel space. In this case, the `task` argument can be set to NULL (This will also bypass permissions checks, so sometimes specifying a task is still required).

A list of open files for each task is stored in the respective task_t structs. A new open file number can be allocated using `vfs_alloc_fileno(task_t* task, int min)`.

## Driver callbacks

In the Xelix virtual file system, every mount point and every open file have a `struct vfs_callbacks` from `src/fs/vfs.h` associated with them. This struct contains the driver callbacks.

	#!c
	struct vfs_callbacks {
		struct vfs_file* (*open)(struct vfs_callback_ctx* ctx, uint32_t flags);
		int (*access)(struct vfs_callback_ctx* ctx, uint32_t amode);
		size_t (*read)(struct vfs_callback_ctx* ctx, void* dest, size_t size);
		size_t (*write)(struct vfs_callback_ctx* ctx, void* source, size_t size);
		size_t (*getdents)(struct vfs_callback_ctx* ctx, void* dest, size_t size);
		int (*stat)(struct vfs_callback_ctx* ctx, vfs_stat_t* dest);
		int (*mkdir)(struct vfs_callback_ctx* ctx, uint32_t mode);
		int (*symlink)(struct vfs_callback_ctx* ctx, const char* target);
		int (*unlink)(struct vfs_callback_ctx* ctx);
		int (*chmod)(struct vfs_callback_ctx* ctx, uint32_t mode);
		int (*chown)(struct vfs_callback_ctx* ctx, uint16_t owner, uint16_t group);
		int (*utimes)(struct vfs_callback_ctx* ctx, struct timeval times[2]);
		int (*link)(struct vfs_callback_ctx* ctx, const char* new_path);
		int (*readlink)(struct vfs_callback_ctx* ctx, char* buf, size_t size);
		int (*rmdir)(struct vfs_callback_ctx* ctx);
		int (*ioctl)(struct vfs_callback_ctx* ctx, int request, void* arg);
		int (*poll)(struct vfs_callback_ctx* ctx, int events);
		int (*build_path_tree)(struct vfs_callback_ctx* ctx);
	};

Most of the driver callbacks map cleanly to public syscalls, but some syscalls are implemented in terms of different driver callbacks.

Not all of the callbacks in the struct need to be set on a mount point or file. If a syscall is used on a file that does not support the necessary callback, the VFS returns a fault with an ENOSYS errno.

Every callback invocation is accompanied by a `struct vfs_callback`. This struct is created in `src/fs/vfs.c` and used to pass request metadata forward.

	#!c
	struct vfs_callback_ctx {
		// File descriptor - Only set on vfs calls that take open files
		struct vfs_file* fp;

		// Path as seen by the vfs (but after vfs_normalize_path has been run)
		char* orig_path;

		// Path starting from the mountpoint
		char* path;

		struct vfs_mountpoint* mp;
		struct task* task;
		bool free_paths;
	};


## Block devices

Block device drivers currently have a simple API using two callbacks:


	#!c
	int (*vfs_block_read_cb)(struct vfs_block_dev* dev, uint64_t lba,
		uint64_t num_blocks, void* buf);

	int (*vfs_block_write_cb)(struct vfs_block_dev* dev, uint64_t lba,
		uint64_t num_blocks, void* buf);

New devices can be registered using `vfs_block_register_dev` from `src/fs/block.c`. Unless the added device is a partition itself (non-zero start offset), it will automatically be probed for partitions. It will also get added to /dev as device file.

The subsystem provides three functions to access block devices:

	#!c
	// Block-based access
	int vfs_block_read(struct vfs_block_dev* dev, int start_block,
	   int num_blocks, uint8_t* buf);

	// Streaming / random access
	uint8_t* vfs_block_sread(struct vfs_block_dev* dev, uint64_t offset,
	   uint64_t size, uint8_t* buf);
	bool vfs_block_swrite(struct vfs_block_dev* dev, uint64_t offset,
	   uint64_t size, uint8_t* buf);


## Mount points

The root file system is specified using the `root=` :ref:`kernel-command-line` parameter. This file system will automatically be mounted to / during VFS initialization. Mount points are kept in a simple linked list of `struct vfs_mountpoint`, since there are rarely more than just a few.

When dealing with absolute paths, the kernel will use `get_mountpoint` from `src/fs/vfs.c` to figure out the correct mount point by looking for the longest path match.

## SysFS

SysFS is a virtual file system used for `/dev` and `/sys`. It is a simple in-memory list of `struct vfs_callback` objects.

Files can be added using

	#!c
	// /sys
	struct sysfs_file* sysfs_add_file(char* name, struct vfs_callbacks* cb);

	// /dev
	struct sysfs_file* sysfs_add_dev(char* name, struct vfs_callbacks* cb);
