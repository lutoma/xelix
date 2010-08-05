#include <memory/gdt.h>

// Lets us access our ASM functions from our C code.
extern void gdt_flush(uint32);

// Internal function prototypes.34
static void gdt_set_gate(sint32,uint32,uint32,uint8,uint8);

gdt_entry_t gdt_entries[5];
gdt_ptr_t   gdt_ptr;
//memory/gdt.c:11:13: Fehler: expected »=«, »,«, »;«, »asm« or »__attribute__« before »idt_entries«
//memory/gdt.c:12:13: Fehler: expected »=«, »,«, »;«, »asm« or »__attribute__« before »idt_ptr«
//idt_entry_t idt_entries[256];
//idt_ptr_t   idt_ptr;

// Initialisation routine - zeroes all the interrupt service routines,
// initialises the GDT and IDT.
void gdt_init()
{
   gdt_ptr.limit = (sizeof(gdt_entry_t) * 5) - 1;
   gdt_ptr.base  = (uint32)&gdt_entries;

   gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
   gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
   gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
   gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
   gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment

   gdt_flush((uint32)&gdt_ptr);
}

// Set the value of one GDT entry.
static void gdt_set_gate(sint32 num, uint32 base, uint32 limit, uint8 access, uint8 gran)
{
   gdt_entries[num].base_low    = (base & 0xFFFF);
   gdt_entries[num].base_middle = (base >> 16) & 0xFF;
   gdt_entries[num].base_high   = (base >> 24) & 0xFF;

   gdt_entries[num].limit_low   = (limit & 0xFFFF);
   gdt_entries[num].granularity = (limit >> 16) & 0x0F;

   gdt_entries[num].granularity |= gran & 0xF0;
   gdt_entries[num].access      = access;
}