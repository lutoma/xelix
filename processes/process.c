#include <processes/process.h>
#include <processes/scheduler.h>
#include <memory/kmalloc.h>

uint32 maxPid = 0;


void createProcess(char name[100], void function())
{
	process_t* p = kmalloc(sizeof(p));
	
	p->pid = ++maxPid;
	p->parent = 0;
	
	// allocate space for the stack
	p->esp = kmalloc(0x1000);
	p->esp += 0x1000; // in x86, the stack grows downwards
	
	
	uint32* stack = p->esp;
	// initialise stack so it looks as if it were stored in switchcontext
	// processor data
	*--stack = 0x202;       // EFLAGS
	*--stack = 0x08;	// CS
	*--stack = (uint32)function;       // EIP

	// pusha
	*--stack = 0;	   // EDI
	*--stack = 0;	   // ESI
	*--stack = 0;	   // EBP
	*--stack = 0;	   // NULL
	*--stack = 0;	   // EBX
	*--stack = 0;	   // EDX
	*--stack = 0;	   // ECX
	*--stack = 0;	   // EAX

	// data segments
	*--stack = 0x10;	// DS
	*--stack = 0x10;	// ES
	*--stack = 0x10;	// FS
	*--stack = 0x10;	// GS
	
	p->esp = stack;
	
	scheduler_addProcess(p);
}
