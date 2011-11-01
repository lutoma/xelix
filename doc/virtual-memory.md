Virtual Memory in Xelix
=======================

Xelix provides a simple Virtual Memory abstraction layer, which will abstract any platform-specific Virtual Memory Code (e.g. x86 Paging). Include the ``memory/vm.h`` to use it:

::

    #include <memory/vm.h>

Please consider that this API is a little bit object-oriented.

struct vm_context
-----------------

A ``struct vm_context`` provides a memory context. It will be provided as an incomplete struct, so you cannot access to struct members and you are encouraged to use the ``vm_*`` functions provided by ``memory/vm.h``.

vm_new()
--------

You must generate a new ``struct vm_context`` object via the ``vm_new()`` funtion. It will return a pointer to a new ``struct vm_context`` in the current memory context or will return a ``NULL`` if there is no memory left in the current mapping.

::

    #include <memory/vm.h>
    
    struct vm_context *vm_new();

struct vm_page
--------------

::

    struct vm_page
    {
        enum
        {
            VM_SECTION_STACK,   /* Initial stack */
            VM_SECTION_CODE,    /* Contains program code and is read-only */
            VM_SECTION_DATA,    /* Contains static data */
            VM_SECTION_HEAP,    /* Allocated by brk(2) at runtime */
            VM_SECTION_MMAP,    /* Allocated by mmap(2) at runtime */
            VM_SECTION_KERNEL,  /* Contains kernel-internal data */
            VM_SECTION_UNMAPPED /* Unmapped */
        } section;

        bool readonly:1;
        bool cow:1; /* Copy-on-Write mechanism */
        bool allocated:1;

        void *cow_src_addr;
        void *virt_addr;
        void *phys_addr;
    };

The ``struct vm_page`` represents a memory area with a size of ``PAGE_SIZE * sizeof(char)``. The ``struct vm_page`` contains a virtual address and a physical address, which must be page aligned, which means, that ``addr % (PAGE_SIZE * sizeof(char)) == 0`` must be 1 (or true). Additionally the struct contains the ``section`` field, which specifies the real  purpose of the memory area, but this is not conclusively regulated and the meaning of the sections should be obvious.

If the ``readonly`` flag is specified, then the pages are only readable. The ``allocated`` flag is set, if the physical address was allocated by a kernel allocator (e.g. ``kmalloc()``). This is useful, if you want to allocate a stack if it is necessary.

``struct vm_page`` supports a Copy-On-Write-mechanism through the ``cow`` flag. If the ``cow`` flag is true then the memory will mapped readonly to ``cow_src_addr`` until an instruction requests write access to the mapped memory area. Then the Page Fault Handler will allocate memory with ``kmalloc()``, copy the data from ``cow_src_addr`` to ``phys_addr``, set the ``allocated`` flag and unset the ``cow`` flag.

.. vim: sw=4 ts=4 et ai tw=72
