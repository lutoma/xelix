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

extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

extern void idt_flush(uint32);
static void setGate(uint8,uint32,uint16,uint8);

isr_t interrupt_handlers[256];

void idt_init()
{
   asm("sti"); // Enable interrupts. Usally a good idea if you want to use them...
   idt_ptr.limit = sizeof(idt_entry_t) * 256 -1;
   idt_ptr.base  = (uint32)&idt_entries;

   memset(&idt_entries, 0, sizeof(idt_entry_t)*256);

    // Remap the irq table.
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);
    print("Remapped IRQ table to ISRs 32-47.\n");

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

  //IRQs
   setGate(32, (uint32)irq0  , 0x08, 0x8E);
   setGate(33, (uint32)irq1  , 0x08, 0x8E);
   setGate(34, (uint32)irq2  , 0x08, 0x8E);
   setGate(35, (uint32)irq3  , 0x08, 0x8E);
   setGate(36, (uint32)irq4  , 0x08, 0x8E);
   setGate(37, (uint32)irq5  , 0x08, 0x8E);
   setGate(38, (uint32)irq6  , 0x08, 0x8E);
   setGate(39, (uint32)irq7  , 0x08, 0x8E);
   setGate(40, (uint32)irq8  , 0x08, 0x8E);
   setGate(41, (uint32)irq9  , 0x08, 0x8E);
   setGate(42, (uint32)irq10 , 0x08, 0x8E);
   setGate(43, (uint32)irq11 , 0x08, 0x8E);
   setGate(44, (uint32)irq12 , 0x08, 0x8E);
   setGate(45, (uint32)irq13 , 0x08, 0x8E);
   setGate(46, (uint32)irq14 , 0x08, 0x8E);
   setGate(47, (uint32)irq15 , 0x08, 0x8E);

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

// This gets called from our ASM interrupt handler stub.
void irq_handler(registers_t regs)
{
   //print("Received IRQ.\n");
   // Send an EOI (end of interrupt) signal to the PICs.
   // If this interrupt involved the slave.
   if (regs.int_no >= 40)
   {
       // Send reset signal to slave.
       outb(0xA0, 0x20);
   }

   //print("Sent EOI\n");
   // Send reset signal to master. (As well as slave, if necessary).
   outb(0x20, 0x20);
  
   if (interrupt_handlers[regs.int_no] != 0)
   {
       isr_t handler = interrupt_handlers[regs.int_no];
       handler(regs);
   }
   
}

void idt_registerHandler(uint8 n, isr_t handler)
{
  print("Registered IRQ handler for ");
  display_printDec(n);
  print("[");
  //display_printDec(handler.ds);
  print("].\n");
  interrupt_handlers[n] = handler;
}
