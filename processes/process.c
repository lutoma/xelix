#include <processes/process.h>
#include <processes/scheduler.h>
#include <memory/kmalloc.h>

uint32 maxPid = 0;


void createProcess(char name[100], void function())
{
	print("create process ");
	print(name);
	print("\n");
	process_t* p = kmalloc(sizeof(p));
	
	p->pid = ++maxPid;
	p->parent = 0;
	
	

	// this process gets its own virtual memory space
	p->pageDirectory = paging_cloneCurrentDirectory();
	
	
	
	
	p->regs.ss = 0x10 ;// kernel data selector
	p->regs.useresp = 0; // this and the entry above only becomes relevant when switching to user mode
	
	// allocate space for the stack
	p->regs.useresp = (uint32)kmalloc(0x1000); // 0xff100000
	p->regs.useresp += 0x1000; // in x86, the stack grows downwards
	p->regs.eflags = 0x202;
	p->regs.cs = 0x08; // kernel code segment
	p->regs.eip = (uint32)function;
	p->regs.err_code = 0;
	p->regs.int_no = 0;
	p->regs.eax = 0;
	p->regs.ecx = 0;
	p->regs.edx = 0;
	p->regs.ebx = 0;
	p->regs.esp = p->regs.useresp;
	p->regs.ebp = 0;
	p->regs.esi = 0;
	p->regs.edi = 0;
	p->regs.ds = 0x10; // kernel data segment
	
	scheduler_addProcess(p);
}
