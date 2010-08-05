#include <interrupts/idt.h>
#include <devices/display/interface.h>

// A struct describing an interrupt gate.
struct idt_entry_struct
{
   uint16 base_lo;             // The lower 16 bits of the address to jump to when this interrupt fires.
   uint16 sel;                 // Kernel segment selector.
   uint8  always0;             // This must always be zero.
   uint8  flags;               // More flags. See documentation.
   uint16 base_hi;             // The upper 16 bits of the address to jump to.
} __attribute__((packed));
typedef struct idt_entry_struct idt_entry_t;

// A struct describing a pointer to an array of interrupt handlers.
// This is in a format suitable for giving to 'lidt'.
struct idt_ptr_struct
{
   uint16 limit;
   uint32 base;                // The address of the first element in our idt_entry_t array.
} __attribute__((packed));
typedef struct idt_ptr_struct idt_ptr_t;

typedef struct registers
{
   uint32 ds;                  // Data segment selector
   uint32 edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
   uint32 int_no, err_code;    // Interrupt number and error code (if applicable)
   uint32 eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
} registers_t;

// These extern directives let us access the addresses of our ASM ISR handlers.
extern void isr0 ();
extern void isr1 ();
extern void isr2 ();
extern void isr3 ();
extern void isr4 ();
extern void isr5 ();
extern void isr6 ();
extern void isr7 ();
extern void isr8 ();
extern void isr9 ();
extern void isr10 ();
extern void isr11 ();
extern void isr12 ();
extern void isr13 ();
extern void isr14 ();
extern void isr15 ();
extern void isr16 ();
extern void isr17 ();
extern void isr18 ();
extern void isr19 ();
extern void isr20 ();
extern void isr21 ();
extern void isr22 ();
extern void isr23 ();
extern void isr24 ();
extern void isr25 ();
extern void isr26 ();
extern void isr27 ();
extern void isr28 ();
extern void isr29 ();
extern void isr30 ();
extern void isr31();
extern void isr32();

idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

extern void idt_flush(uint32);
static void setGate(uint8,uint32,uint16,uint8);

void idt_init()
{
   idt_ptr.limit = sizeof(idt_entry_t) * 256 -1;
   idt_ptr.base  = (uint32)&idt_entries;

   memset(&idt_entries, 0, sizeof(idt_entry_t)*256);

   setGate( 0, (uint32)isr0  , 0x08, 0x8E);
   setGate( 1, (uint32)isr1  , 0x08, 0x8E);
   setGate( 2, (uint32)isr2  , 0x08, 0x8E);
   setGate( 3, (uint32)isr3  , 0x08, 0x8E);
   setGate( 4, (uint32)isr4  , 0x08, 0x8E);
   setGate( 5, (uint32)isr5  , 0x08, 0x8E);
   setGate( 6, (uint32)isr6  , 0x08, 0x8E);
   setGate( 7, (uint32)isr7  , 0x08, 0x8E);
   setGate( 8, (uint32)isr8  , 0x08, 0x8E);
   setGate( 9, (uint32)isr9  , 0x08, 0x8E);
   setGate(10, (uint32)isr10 , 0x08, 0x8E);
   setGate(11, (uint32)isr11 , 0x08, 0x8E);
   setGate(12, (uint32)isr12 , 0x08, 0x8E);
   setGate(13, (uint32)isr13 , 0x08, 0x8E);
   setGate(14, (uint32)isr14 , 0x08, 0x8E);
   setGate(15, (uint32)isr15 , 0x08, 0x8E);
   setGate(16, (uint32)isr16 , 0x08, 0x8E);
   setGate(17, (uint32)isr17 , 0x08, 0x8E);
   setGate(18, (uint32)isr18 , 0x08, 0x8E);
   setGate(19, (uint32)isr19 , 0x08, 0x8E);
   setGate(20, (uint32)isr20 , 0x08, 0x8E);
   setGate(21, (uint32)isr21 , 0x08, 0x8E);
   setGate(22, (uint32)isr22 , 0x08, 0x8E);
   setGate(23, (uint32)isr23 , 0x08, 0x8E);
   setGate(24, (uint32)isr24 , 0x08, 0x8E);
   setGate(25, (uint32)isr25 , 0x08, 0x8E);
   setGate(26, (uint32)isr26 , 0x08, 0x8E);
   setGate(27, (uint32)isr27 , 0x08, 0x8E);
   setGate(28, (uint32)isr28 , 0x08, 0x8E);
   setGate(29, (uint32)isr29 , 0x08, 0x8E);
   setGate(30, (uint32)isr30 , 0x08, 0x8E);
   setGate(31, (uint32)isr31 , 0x08, 0x8E);
   setGate(32, (uint32)isr32 , 0x08, 0x8E);

   idt_flush((uint32)&idt_ptr);
}

static void setGate(uint8 num, uint32 base, uint16 sel, uint8 flags)
{
   idt_entries[num].base_lo = base & 0xFFFF;
   idt_entries[num].base_hi = (base >> 16) & 0xFFFF;

   idt_entries[num].sel     = sel;
   idt_entries[num].always0 = 0;
   // We must uncomment the OR below when we get to using user-mode.
   // It sets the interrupt gate's privilege level to 3.
   idt_entries[num].flags   = flags /* | 0x60 */;
}

// This gets called from our ASM interrupt handler stub.
void isr_handler(registers_t regs)
{
   print("received interrupt: ");
   display_printHex(regs.int_no);
   print("\n");
}
