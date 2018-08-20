/* idt.c: Initialization of the IDT
 * Copyright © 2010 Christoph Sünderhauf
 * Copyright © 2011 Lukas Martini
 *
 * This file is part of Xelix.
 *
 * Xelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "idt.h"

#include <hw/interrupts.h>
#include <lib/log.h>
#include <lib/string.h>

// A struct describing an interrupt gate.
struct idt_entry {
	uint16_t base_lo;	// The lower 16 bits of the address to jump to when this interrupt fires.
	uint16_t sel;		// Kernel segment selector.
	uint8_t  always0;	// This must always be zero.
	uint8_t  flags;	// More flags. See documentation.
	uint16_t base_hi;	// The upper 16 bits of the address to jump to.
} __attribute__((packed));

// A struct describing a pointer to an array of interrupt handlers.
// This is in a format suitable for lidt.
struct {
	uint16_t limit;
	uint32_t base; // The address of the first element in our idt_entry_t array.
} __attribute__((packed)) idt_array;

extern void interrupts_handler0(void);
extern void interrupts_handler1(void);
extern void interrupts_handler2(void);
extern void interrupts_handler3(void);
extern void interrupts_handler4(void);
extern void interrupts_handler5(void);
extern void interrupts_handler6(void);
extern void interrupts_handler7(void);
extern void interrupts_handler8(void);
extern void interrupts_handler9(void);
extern void interrupts_handler10(void);
extern void interrupts_handler11(void);
extern void interrupts_handler12(void);
extern void interrupts_handler13(void);
extern void interrupts_handler14(void);
extern void interrupts_handler15(void);
extern void interrupts_handler16(void);
extern void interrupts_handler17(void);
extern void interrupts_handler18(void);
extern void interrupts_handler19(void);
extern void interrupts_handler20(void);
extern void interrupts_handler21(void);
extern void interrupts_handler22(void);
extern void interrupts_handler23(void);
extern void interrupts_handler24(void);
extern void interrupts_handler25(void);
extern void interrupts_handler26(void);
extern void interrupts_handler27(void);
extern void interrupts_handler28(void);
extern void interrupts_handler29(void);
extern void interrupts_handler30(void);
extern void interrupts_handler31(void);
extern void interrupts_handler32(void);
extern void interrupts_handler33(void);
extern void interrupts_handler34(void);
extern void interrupts_handler35(void);
extern void interrupts_handler36(void);
extern void interrupts_handler37(void);
extern void interrupts_handler38(void);
extern void interrupts_handler39(void);
extern void interrupts_handler40(void);
extern void interrupts_handler41(void);
extern void interrupts_handler42(void);
extern void interrupts_handler43(void);
extern void interrupts_handler44(void);
extern void interrupts_handler45(void);
extern void interrupts_handler46(void);
extern void interrupts_handler47(void);
extern void interrupts_handler48(void);
extern void interrupts_handler49(void);
extern void interrupts_handler50(void);
extern void interrupts_handler51(void);
extern void interrupts_handler52(void);
extern void interrupts_handler53(void);
extern void interrupts_handler54(void);
extern void interrupts_handler55(void);
extern void interrupts_handler56(void);
extern void interrupts_handler57(void);
extern void interrupts_handler58(void);
extern void interrupts_handler59(void);
extern void interrupts_handler60(void);
extern void interrupts_handler61(void);
extern void interrupts_handler62(void);
extern void interrupts_handler63(void);
extern void interrupts_handler64(void);
extern void interrupts_handler65(void);
extern void interrupts_handler66(void);
extern void interrupts_handler67(void);
extern void interrupts_handler68(void);
extern void interrupts_handler69(void);
extern void interrupts_handler70(void);
extern void interrupts_handler71(void);
extern void interrupts_handler72(void);
extern void interrupts_handler73(void);
extern void interrupts_handler74(void);
extern void interrupts_handler75(void);
extern void interrupts_handler76(void);
extern void interrupts_handler77(void);
extern void interrupts_handler78(void);
extern void interrupts_handler79(void);
extern void interrupts_handler80(void);
extern void interrupts_handler81(void);
extern void interrupts_handler82(void);
extern void interrupts_handler83(void);
extern void interrupts_handler84(void);
extern void interrupts_handler85(void);
extern void interrupts_handler86(void);
extern void interrupts_handler87(void);
extern void interrupts_handler88(void);
extern void interrupts_handler89(void);
extern void interrupts_handler90(void);
extern void interrupts_handler91(void);
extern void interrupts_handler92(void);
extern void interrupts_handler93(void);
extern void interrupts_handler94(void);
extern void interrupts_handler95(void);
extern void interrupts_handler96(void);
extern void interrupts_handler97(void);
extern void interrupts_handler98(void);
extern void interrupts_handler99(void);
extern void interrupts_handler100(void);
extern void interrupts_handler101(void);
extern void interrupts_handler102(void);
extern void interrupts_handler103(void);
extern void interrupts_handler104(void);
extern void interrupts_handler105(void);
extern void interrupts_handler106(void);
extern void interrupts_handler107(void);
extern void interrupts_handler108(void);
extern void interrupts_handler109(void);
extern void interrupts_handler110(void);
extern void interrupts_handler111(void);
extern void interrupts_handler112(void);
extern void interrupts_handler113(void);
extern void interrupts_handler114(void);
extern void interrupts_handler115(void);
extern void interrupts_handler116(void);
extern void interrupts_handler117(void);
extern void interrupts_handler118(void);
extern void interrupts_handler119(void);
extern void interrupts_handler120(void);
extern void interrupts_handler121(void);
extern void interrupts_handler122(void);
extern void interrupts_handler123(void);
extern void interrupts_handler124(void);
extern void interrupts_handler125(void);
extern void interrupts_handler126(void);
extern void interrupts_handler127(void);
extern void interrupts_handler128(void);
extern void interrupts_handler129(void);
extern void interrupts_handler130(void);
extern void interrupts_handler131(void);
extern void interrupts_handler132(void);
extern void interrupts_handler133(void);
extern void interrupts_handler134(void);
extern void interrupts_handler135(void);
extern void interrupts_handler136(void);
extern void interrupts_handler137(void);
extern void interrupts_handler138(void);
extern void interrupts_handler139(void);
extern void interrupts_handler140(void);
extern void interrupts_handler141(void);
extern void interrupts_handler142(void);
extern void interrupts_handler143(void);
extern void interrupts_handler144(void);
extern void interrupts_handler145(void);
extern void interrupts_handler146(void);
extern void interrupts_handler147(void);
extern void interrupts_handler148(void);
extern void interrupts_handler149(void);
extern void interrupts_handler150(void);
extern void interrupts_handler151(void);
extern void interrupts_handler152(void);
extern void interrupts_handler153(void);
extern void interrupts_handler154(void);
extern void interrupts_handler155(void);
extern void interrupts_handler156(void);
extern void interrupts_handler157(void);
extern void interrupts_handler158(void);
extern void interrupts_handler159(void);
extern void interrupts_handler160(void);
extern void interrupts_handler161(void);
extern void interrupts_handler162(void);
extern void interrupts_handler163(void);
extern void interrupts_handler164(void);
extern void interrupts_handler165(void);
extern void interrupts_handler166(void);
extern void interrupts_handler167(void);
extern void interrupts_handler168(void);
extern void interrupts_handler169(void);
extern void interrupts_handler170(void);
extern void interrupts_handler171(void);
extern void interrupts_handler172(void);
extern void interrupts_handler173(void);
extern void interrupts_handler174(void);
extern void interrupts_handler175(void);
extern void interrupts_handler176(void);
extern void interrupts_handler177(void);
extern void interrupts_handler178(void);
extern void interrupts_handler179(void);
extern void interrupts_handler180(void);
extern void interrupts_handler181(void);
extern void interrupts_handler182(void);
extern void interrupts_handler183(void);
extern void interrupts_handler184(void);
extern void interrupts_handler185(void);
extern void interrupts_handler186(void);
extern void interrupts_handler187(void);
extern void interrupts_handler188(void);
extern void interrupts_handler189(void);
extern void interrupts_handler190(void);
extern void interrupts_handler191(void);
extern void interrupts_handler192(void);
extern void interrupts_handler193(void);
extern void interrupts_handler194(void);
extern void interrupts_handler195(void);
extern void interrupts_handler196(void);
extern void interrupts_handler197(void);
extern void interrupts_handler198(void);
extern void interrupts_handler199(void);
extern void interrupts_handler200(void);
extern void interrupts_handler201(void);
extern void interrupts_handler202(void);
extern void interrupts_handler203(void);
extern void interrupts_handler204(void);
extern void interrupts_handler205(void);
extern void interrupts_handler206(void);
extern void interrupts_handler207(void);
extern void interrupts_handler208(void);
extern void interrupts_handler209(void);
extern void interrupts_handler210(void);
extern void interrupts_handler211(void);
extern void interrupts_handler212(void);
extern void interrupts_handler213(void);
extern void interrupts_handler214(void);
extern void interrupts_handler215(void);
extern void interrupts_handler216(void);
extern void interrupts_handler217(void);
extern void interrupts_handler218(void);
extern void interrupts_handler219(void);
extern void interrupts_handler220(void);
extern void interrupts_handler221(void);
extern void interrupts_handler222(void);
extern void interrupts_handler223(void);
extern void interrupts_handler224(void);
extern void interrupts_handler225(void);
extern void interrupts_handler226(void);
extern void interrupts_handler227(void);
extern void interrupts_handler228(void);
extern void interrupts_handler229(void);
extern void interrupts_handler230(void);
extern void interrupts_handler231(void);
extern void interrupts_handler232(void);
extern void interrupts_handler233(void);
extern void interrupts_handler234(void);
extern void interrupts_handler235(void);
extern void interrupts_handler236(void);
extern void interrupts_handler237(void);
extern void interrupts_handler238(void);
extern void interrupts_handler239(void);
extern void interrupts_handler240(void);
extern void interrupts_handler241(void);
extern void interrupts_handler242(void);
extern void interrupts_handler243(void);
extern void interrupts_handler244(void);
extern void interrupts_handler245(void);
extern void interrupts_handler246(void);
extern void interrupts_handler247(void);
extern void interrupts_handler248(void);
extern void interrupts_handler249(void);
extern void interrupts_handler250(void);
extern void interrupts_handler251(void);
extern void interrupts_handler252(void);
extern void interrupts_handler253(void);
extern void interrupts_handler254(void);
extern void interrupts_handler255(void);
extern void __attribute__((fastcall)) idt_load(void*);

struct idt_entry idt_entries[256];

static void set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
	idt_entries[num].base_lo = base & 0xFFFF;
	idt_entries[num].base_hi = (base >> 16) & 0xFFFF;

	idt_entries[num].sel     = sel;
	idt_entries[num].always0 = 0;
	// We must uncomment the OR below when we get to using user-mode.
	// It sets the interrupt gate's privilege level to 3.
	idt_entries[num].flags   = flags /* | 0x60 */;
}

void idt_init() {
	idt_array.limit = sizeof(struct idt_entry) * 256 -1;
	idt_array.base = (uint32_t)&idt_entries;

	bzero(&idt_entries, sizeof(idt_entries));

	// Initialize master PIC
	outb(0x20, 0x11); // The PICs initialization command
	outb(0x21, 0x20); // Intnum for IRQ 0
	outb(0x21, 0x04); // IRQ 2 = Slave
	outb(0x21, 0x01); // ICW 4

	// Initialize slave PIC
	outb(0xa0, 0x11); // The PICs initialization command
	outb(0xa1, 0x28); // Intnum for IRQ 8
	outb(0xa1, 0x02); // IRQ 2 = Slave
	outb(0xa1, 0x01); // ICW 4

	// Activate all IRQs (demask)
	outb(0x20, 0x0);
	outb(0xa0, 0x0);

	// preprocessor for would be quite cool here ;)
	set_gate(0, (uint32_t)interrupts_handler0, 0x08, 0x8E);
	set_gate(1, (uint32_t)interrupts_handler1, 0x08, 0x8E);
	set_gate(2, (uint32_t)interrupts_handler2, 0x08, 0x8E);
	set_gate(3, (uint32_t)interrupts_handler3, 0x08, 0x8E);
	set_gate(4, (uint32_t)interrupts_handler4, 0x08, 0x8E);
	set_gate(5, (uint32_t)interrupts_handler5, 0x08, 0x8E);
	set_gate(6, (uint32_t)interrupts_handler6, 0x08, 0x8E);
	set_gate(7, (uint32_t)interrupts_handler7, 0x08, 0x8E);
	set_gate(8, (uint32_t)interrupts_handler8, 0x08, 0x8E);
	set_gate(9, (uint32_t)interrupts_handler9, 0x08, 0x8E);
	set_gate(10, (uint32_t)interrupts_handler10, 0x08, 0x8E);
	set_gate(11, (uint32_t)interrupts_handler11, 0x08, 0x8E);
	set_gate(12, (uint32_t)interrupts_handler12, 0x08, 0x8E);
	set_gate(13, (uint32_t)interrupts_handler13, 0x08, 0x8E);
	set_gate(14, (uint32_t)interrupts_handler14, 0x08, 0x8E);
	set_gate(15, (uint32_t)interrupts_handler15, 0x08, 0x8E);
	set_gate(16, (uint32_t)interrupts_handler16, 0x08, 0x8E);
	set_gate(17, (uint32_t)interrupts_handler17, 0x08, 0x8E);
	set_gate(18, (uint32_t)interrupts_handler18, 0x08, 0x8E);
	set_gate(19, (uint32_t)interrupts_handler19, 0x08, 0x8E);
	set_gate(20, (uint32_t)interrupts_handler20, 0x08, 0x8E);
	set_gate(21, (uint32_t)interrupts_handler21, 0x08, 0x8E);
	set_gate(22, (uint32_t)interrupts_handler22, 0x08, 0x8E);
	set_gate(23, (uint32_t)interrupts_handler23, 0x08, 0x8E);
	set_gate(24, (uint32_t)interrupts_handler24, 0x08, 0x8E);
	set_gate(25, (uint32_t)interrupts_handler25, 0x08, 0x8E);
	set_gate(26, (uint32_t)interrupts_handler26, 0x08, 0x8E);
	set_gate(27, (uint32_t)interrupts_handler27, 0x08, 0x8E);
	set_gate(28, (uint32_t)interrupts_handler28, 0x08, 0x8E);
	set_gate(29, (uint32_t)interrupts_handler29, 0x08, 0x8E);
	set_gate(30, (uint32_t)interrupts_handler30, 0x08, 0x8E);
	set_gate(31, (uint32_t)interrupts_handler31, 0x08, 0x8E);
	set_gate(32, (uint32_t)interrupts_handler32, 0x08, 0x8E);
	set_gate(33, (uint32_t)interrupts_handler33, 0x08, 0x8E);
	set_gate(34, (uint32_t)interrupts_handler34, 0x08, 0x8E);
	set_gate(35, (uint32_t)interrupts_handler35, 0x08, 0x8E);
	set_gate(36, (uint32_t)interrupts_handler36, 0x08, 0x8E);
	set_gate(37, (uint32_t)interrupts_handler37, 0x08, 0x8E);
	set_gate(38, (uint32_t)interrupts_handler38, 0x08, 0x8E);
	set_gate(39, (uint32_t)interrupts_handler39, 0x08, 0x8E);
	set_gate(40, (uint32_t)interrupts_handler40, 0x08, 0x8E);
	set_gate(41, (uint32_t)interrupts_handler41, 0x08, 0x8E);
	set_gate(42, (uint32_t)interrupts_handler42, 0x08, 0x8E);
	set_gate(43, (uint32_t)interrupts_handler43, 0x08, 0x8E);
	set_gate(44, (uint32_t)interrupts_handler44, 0x08, 0x8E);
	set_gate(45, (uint32_t)interrupts_handler45, 0x08, 0x8E);
	set_gate(46, (uint32_t)interrupts_handler46, 0x08, 0x8E);
	set_gate(47, (uint32_t)interrupts_handler47, 0x08, 0x8E);
	set_gate(48, (uint32_t)interrupts_handler48, 0x08, 0x8F);
	set_gate(49, (uint32_t)interrupts_handler49, 0x08, 0x8E);
	set_gate(50, (uint32_t)interrupts_handler50, 0x08, 0x8E);
	set_gate(51, (uint32_t)interrupts_handler51, 0x08, 0x8E);
	set_gate(52, (uint32_t)interrupts_handler52, 0x08, 0x8E);
	set_gate(53, (uint32_t)interrupts_handler53, 0x08, 0x8E);
	set_gate(54, (uint32_t)interrupts_handler54, 0x08, 0x8E);
	set_gate(55, (uint32_t)interrupts_handler55, 0x08, 0x8E);
	set_gate(56, (uint32_t)interrupts_handler56, 0x08, 0x8E);
	set_gate(57, (uint32_t)interrupts_handler57, 0x08, 0x8E);
	set_gate(58, (uint32_t)interrupts_handler58, 0x08, 0x8E);
	set_gate(59, (uint32_t)interrupts_handler59, 0x08, 0x8E);
	set_gate(60, (uint32_t)interrupts_handler60, 0x08, 0x8E);
	set_gate(61, (uint32_t)interrupts_handler61, 0x08, 0x8E);
	set_gate(62, (uint32_t)interrupts_handler62, 0x08, 0x8E);
	set_gate(63, (uint32_t)interrupts_handler63, 0x08, 0x8E);
	set_gate(64, (uint32_t)interrupts_handler64, 0x08, 0x8E);
	set_gate(65, (uint32_t)interrupts_handler65, 0x08, 0x8E);
	set_gate(66, (uint32_t)interrupts_handler66, 0x08, 0x8E);
	set_gate(67, (uint32_t)interrupts_handler67, 0x08, 0x8E);
	set_gate(68, (uint32_t)interrupts_handler68, 0x08, 0x8E);
	set_gate(69, (uint32_t)interrupts_handler69, 0x08, 0x8E);
	set_gate(70, (uint32_t)interrupts_handler70, 0x08, 0x8E);
	set_gate(71, (uint32_t)interrupts_handler71, 0x08, 0x8E);
	set_gate(72, (uint32_t)interrupts_handler72, 0x08, 0x8E);
	set_gate(73, (uint32_t)interrupts_handler73, 0x08, 0x8E);
	set_gate(74, (uint32_t)interrupts_handler74, 0x08, 0x8E);
	set_gate(75, (uint32_t)interrupts_handler75, 0x08, 0x8E);
	set_gate(76, (uint32_t)interrupts_handler76, 0x08, 0x8E);
	set_gate(77, (uint32_t)interrupts_handler77, 0x08, 0x8E);
	set_gate(78, (uint32_t)interrupts_handler78, 0x08, 0x8E);
	set_gate(79, (uint32_t)interrupts_handler79, 0x08, 0x8E);
	set_gate(80, (uint32_t)interrupts_handler80, 0x08, 0x8E);
	set_gate(81, (uint32_t)interrupts_handler81, 0x08, 0x8E);
	set_gate(82, (uint32_t)interrupts_handler82, 0x08, 0x8E);
	set_gate(83, (uint32_t)interrupts_handler83, 0x08, 0x8E);
	set_gate(84, (uint32_t)interrupts_handler84, 0x08, 0x8E);
	set_gate(85, (uint32_t)interrupts_handler85, 0x08, 0x8E);
	set_gate(86, (uint32_t)interrupts_handler86, 0x08, 0x8E);
	set_gate(87, (uint32_t)interrupts_handler87, 0x08, 0x8E);
	set_gate(88, (uint32_t)interrupts_handler88, 0x08, 0x8E);
	set_gate(89, (uint32_t)interrupts_handler89, 0x08, 0x8E);
	set_gate(90, (uint32_t)interrupts_handler90, 0x08, 0x8E);
	set_gate(91, (uint32_t)interrupts_handler91, 0x08, 0x8E);
	set_gate(92, (uint32_t)interrupts_handler92, 0x08, 0x8E);
	set_gate(93, (uint32_t)interrupts_handler93, 0x08, 0x8E);
	set_gate(94, (uint32_t)interrupts_handler94, 0x08, 0x8E);
	set_gate(95, (uint32_t)interrupts_handler95, 0x08, 0x8E);
	set_gate(96, (uint32_t)interrupts_handler96, 0x08, 0x8E);
	set_gate(97, (uint32_t)interrupts_handler97, 0x08, 0x8E);
	set_gate(98, (uint32_t)interrupts_handler98, 0x08, 0x8E);
	set_gate(99, (uint32_t)interrupts_handler99, 0x08, 0x8E);
	set_gate(100, (uint32_t)interrupts_handler100, 0x08, 0x8E);
	set_gate(101, (uint32_t)interrupts_handler101, 0x08, 0x8E);
	set_gate(102, (uint32_t)interrupts_handler102, 0x08, 0x8E);
	set_gate(103, (uint32_t)interrupts_handler103, 0x08, 0x8E);
	set_gate(104, (uint32_t)interrupts_handler104, 0x08, 0x8E);
	set_gate(105, (uint32_t)interrupts_handler105, 0x08, 0x8E);
	set_gate(106, (uint32_t)interrupts_handler106, 0x08, 0x8E);
	set_gate(107, (uint32_t)interrupts_handler107, 0x08, 0x8E);
	set_gate(108, (uint32_t)interrupts_handler108, 0x08, 0x8E);
	set_gate(109, (uint32_t)interrupts_handler109, 0x08, 0x8E);
	set_gate(110, (uint32_t)interrupts_handler110, 0x08, 0x8E);
	set_gate(111, (uint32_t)interrupts_handler111, 0x08, 0x8E);
	set_gate(112, (uint32_t)interrupts_handler112, 0x08, 0x8E);
	set_gate(113, (uint32_t)interrupts_handler113, 0x08, 0x8E);
	set_gate(114, (uint32_t)interrupts_handler114, 0x08, 0x8E);
	set_gate(115, (uint32_t)interrupts_handler115, 0x08, 0x8E);
	set_gate(116, (uint32_t)interrupts_handler116, 0x08, 0x8E);
	set_gate(117, (uint32_t)interrupts_handler117, 0x08, 0x8E);
	set_gate(118, (uint32_t)interrupts_handler118, 0x08, 0x8E);
	set_gate(119, (uint32_t)interrupts_handler119, 0x08, 0x8E);
	set_gate(120, (uint32_t)interrupts_handler120, 0x08, 0x8E);
	set_gate(121, (uint32_t)interrupts_handler121, 0x08, 0x8E);
	set_gate(122, (uint32_t)interrupts_handler122, 0x08, 0x8E);
	set_gate(123, (uint32_t)interrupts_handler123, 0x08, 0x8E);
	set_gate(124, (uint32_t)interrupts_handler124, 0x08, 0x8E);
	set_gate(125, (uint32_t)interrupts_handler125, 0x08, 0x8E);
	set_gate(126, (uint32_t)interrupts_handler126, 0x08, 0x8E);
	set_gate(127, (uint32_t)interrupts_handler127, 0x08, 0x8E);
	set_gate(128, (uint32_t)interrupts_handler128, 0x08, 0x8F);
	set_gate(129, (uint32_t)interrupts_handler129, 0x08, 0x8E);
	set_gate(130, (uint32_t)interrupts_handler130, 0x08, 0x8E);
	set_gate(131, (uint32_t)interrupts_handler131, 0x08, 0x8E);
	set_gate(132, (uint32_t)interrupts_handler132, 0x08, 0x8E);
	set_gate(133, (uint32_t)interrupts_handler133, 0x08, 0x8E);
	set_gate(134, (uint32_t)interrupts_handler134, 0x08, 0x8E);
	set_gate(135, (uint32_t)interrupts_handler135, 0x08, 0x8E);
	set_gate(136, (uint32_t)interrupts_handler136, 0x08, 0x8E);
	set_gate(137, (uint32_t)interrupts_handler137, 0x08, 0x8E);
	set_gate(138, (uint32_t)interrupts_handler138, 0x08, 0x8E);
	set_gate(139, (uint32_t)interrupts_handler139, 0x08, 0x8E);
	set_gate(140, (uint32_t)interrupts_handler140, 0x08, 0x8E);
	set_gate(141, (uint32_t)interrupts_handler141, 0x08, 0x8E);
	set_gate(142, (uint32_t)interrupts_handler142, 0x08, 0x8E);
	set_gate(143, (uint32_t)interrupts_handler143, 0x08, 0x8E);
	set_gate(144, (uint32_t)interrupts_handler144, 0x08, 0x8E);
	set_gate(145, (uint32_t)interrupts_handler145, 0x08, 0x8E);
	set_gate(146, (uint32_t)interrupts_handler146, 0x08, 0x8E);
	set_gate(147, (uint32_t)interrupts_handler147, 0x08, 0x8E);
	set_gate(148, (uint32_t)interrupts_handler148, 0x08, 0x8E);
	set_gate(149, (uint32_t)interrupts_handler149, 0x08, 0x8E);
	set_gate(150, (uint32_t)interrupts_handler150, 0x08, 0x8E);
	set_gate(151, (uint32_t)interrupts_handler151, 0x08, 0x8E);
	set_gate(152, (uint32_t)interrupts_handler152, 0x08, 0x8E);
	set_gate(153, (uint32_t)interrupts_handler153, 0x08, 0x8E);
	set_gate(154, (uint32_t)interrupts_handler154, 0x08, 0x8E);
	set_gate(155, (uint32_t)interrupts_handler155, 0x08, 0x8E);
	set_gate(156, (uint32_t)interrupts_handler156, 0x08, 0x8E);
	set_gate(157, (uint32_t)interrupts_handler157, 0x08, 0x8E);
	set_gate(158, (uint32_t)interrupts_handler158, 0x08, 0x8E);
	set_gate(159, (uint32_t)interrupts_handler159, 0x08, 0x8E);
	set_gate(160, (uint32_t)interrupts_handler160, 0x08, 0x8E);
	set_gate(161, (uint32_t)interrupts_handler161, 0x08, 0x8E);
	set_gate(162, (uint32_t)interrupts_handler162, 0x08, 0x8E);
	set_gate(163, (uint32_t)interrupts_handler163, 0x08, 0x8E);
	set_gate(164, (uint32_t)interrupts_handler164, 0x08, 0x8E);
	set_gate(165, (uint32_t)interrupts_handler165, 0x08, 0x8E);
	set_gate(166, (uint32_t)interrupts_handler166, 0x08, 0x8E);
	set_gate(167, (uint32_t)interrupts_handler167, 0x08, 0x8E);
	set_gate(168, (uint32_t)interrupts_handler168, 0x08, 0x8E);
	set_gate(169, (uint32_t)interrupts_handler169, 0x08, 0x8E);
	set_gate(170, (uint32_t)interrupts_handler170, 0x08, 0x8E);
	set_gate(171, (uint32_t)interrupts_handler171, 0x08, 0x8E);
	set_gate(172, (uint32_t)interrupts_handler172, 0x08, 0x8E);
	set_gate(173, (uint32_t)interrupts_handler173, 0x08, 0x8E);
	set_gate(174, (uint32_t)interrupts_handler174, 0x08, 0x8E);
	set_gate(175, (uint32_t)interrupts_handler175, 0x08, 0x8E);
	set_gate(176, (uint32_t)interrupts_handler176, 0x08, 0x8E);
	set_gate(177, (uint32_t)interrupts_handler177, 0x08, 0x8E);
	set_gate(178, (uint32_t)interrupts_handler178, 0x08, 0x8E);
	set_gate(179, (uint32_t)interrupts_handler179, 0x08, 0x8E);
	set_gate(180, (uint32_t)interrupts_handler180, 0x08, 0x8E);
	set_gate(181, (uint32_t)interrupts_handler181, 0x08, 0x8E);
	set_gate(182, (uint32_t)interrupts_handler182, 0x08, 0x8E);
	set_gate(183, (uint32_t)interrupts_handler183, 0x08, 0x8E);
	set_gate(184, (uint32_t)interrupts_handler184, 0x08, 0x8E);
	set_gate(185, (uint32_t)interrupts_handler185, 0x08, 0x8E);
	set_gate(186, (uint32_t)interrupts_handler186, 0x08, 0x8E);
	set_gate(187, (uint32_t)interrupts_handler187, 0x08, 0x8E);
	set_gate(188, (uint32_t)interrupts_handler188, 0x08, 0x8E);
	set_gate(189, (uint32_t)interrupts_handler189, 0x08, 0x8E);
	set_gate(190, (uint32_t)interrupts_handler190, 0x08, 0x8E);
	set_gate(191, (uint32_t)interrupts_handler191, 0x08, 0x8E);
	set_gate(192, (uint32_t)interrupts_handler192, 0x08, 0x8E);
	set_gate(193, (uint32_t)interrupts_handler193, 0x08, 0x8E);
	set_gate(194, (uint32_t)interrupts_handler194, 0x08, 0x8E);
	set_gate(195, (uint32_t)interrupts_handler195, 0x08, 0x8E);
	set_gate(196, (uint32_t)interrupts_handler196, 0x08, 0x8E);
	set_gate(197, (uint32_t)interrupts_handler197, 0x08, 0x8E);
	set_gate(198, (uint32_t)interrupts_handler198, 0x08, 0x8E);
	set_gate(199, (uint32_t)interrupts_handler199, 0x08, 0x8E);
	set_gate(200, (uint32_t)interrupts_handler200, 0x08, 0x8E);
	set_gate(201, (uint32_t)interrupts_handler201, 0x08, 0x8E);
	set_gate(202, (uint32_t)interrupts_handler202, 0x08, 0x8E);
	set_gate(203, (uint32_t)interrupts_handler203, 0x08, 0x8E);
	set_gate(204, (uint32_t)interrupts_handler204, 0x08, 0x8E);
	set_gate(205, (uint32_t)interrupts_handler205, 0x08, 0x8E);
	set_gate(206, (uint32_t)interrupts_handler206, 0x08, 0x8E);
	set_gate(207, (uint32_t)interrupts_handler207, 0x08, 0x8E);
	set_gate(208, (uint32_t)interrupts_handler208, 0x08, 0x8E);
	set_gate(209, (uint32_t)interrupts_handler209, 0x08, 0x8E);
	set_gate(210, (uint32_t)interrupts_handler210, 0x08, 0x8E);
	set_gate(211, (uint32_t)interrupts_handler211, 0x08, 0x8E);
	set_gate(212, (uint32_t)interrupts_handler212, 0x08, 0x8E);
	set_gate(213, (uint32_t)interrupts_handler213, 0x08, 0x8E);
	set_gate(214, (uint32_t)interrupts_handler214, 0x08, 0x8E);
	set_gate(215, (uint32_t)interrupts_handler215, 0x08, 0x8E);
	set_gate(216, (uint32_t)interrupts_handler216, 0x08, 0x8E);
	set_gate(217, (uint32_t)interrupts_handler217, 0x08, 0x8E);
	set_gate(218, (uint32_t)interrupts_handler218, 0x08, 0x8E);
	set_gate(219, (uint32_t)interrupts_handler219, 0x08, 0x8E);
	set_gate(220, (uint32_t)interrupts_handler220, 0x08, 0x8E);
	set_gate(221, (uint32_t)interrupts_handler221, 0x08, 0x8E);
	set_gate(222, (uint32_t)interrupts_handler222, 0x08, 0x8E);
	set_gate(223, (uint32_t)interrupts_handler223, 0x08, 0x8E);
	set_gate(224, (uint32_t)interrupts_handler224, 0x08, 0x8E);
	set_gate(225, (uint32_t)interrupts_handler225, 0x08, 0x8E);
	set_gate(226, (uint32_t)interrupts_handler226, 0x08, 0x8E);
	set_gate(227, (uint32_t)interrupts_handler227, 0x08, 0x8E);
	set_gate(228, (uint32_t)interrupts_handler228, 0x08, 0x8E);
	set_gate(229, (uint32_t)interrupts_handler229, 0x08, 0x8E);
	set_gate(230, (uint32_t)interrupts_handler230, 0x08, 0x8E);
	set_gate(231, (uint32_t)interrupts_handler231, 0x08, 0x8E);
	set_gate(232, (uint32_t)interrupts_handler232, 0x08, 0x8E);
	set_gate(233, (uint32_t)interrupts_handler233, 0x08, 0x8E);
	set_gate(234, (uint32_t)interrupts_handler234, 0x08, 0x8E);
	set_gate(235, (uint32_t)interrupts_handler235, 0x08, 0x8E);
	set_gate(236, (uint32_t)interrupts_handler236, 0x08, 0x8E);
	set_gate(237, (uint32_t)interrupts_handler237, 0x08, 0x8E);
	set_gate(238, (uint32_t)interrupts_handler238, 0x08, 0x8E);
	set_gate(239, (uint32_t)interrupts_handler239, 0x08, 0x8E);
	set_gate(240, (uint32_t)interrupts_handler240, 0x08, 0x8E);
	set_gate(241, (uint32_t)interrupts_handler241, 0x08, 0x8E);
	set_gate(242, (uint32_t)interrupts_handler242, 0x08, 0x8E);
	set_gate(243, (uint32_t)interrupts_handler243, 0x08, 0x8E);
	set_gate(244, (uint32_t)interrupts_handler244, 0x08, 0x8E);
	set_gate(245, (uint32_t)interrupts_handler245, 0x08, 0x8E);
	set_gate(246, (uint32_t)interrupts_handler246, 0x08, 0x8E);
	set_gate(247, (uint32_t)interrupts_handler247, 0x08, 0x8E);
	set_gate(248, (uint32_t)interrupts_handler248, 0x08, 0x8E);
	set_gate(249, (uint32_t)interrupts_handler249, 0x08, 0x8E);
	set_gate(250, (uint32_t)interrupts_handler250, 0x08, 0x8E);
	set_gate(251, (uint32_t)interrupts_handler251, 0x08, 0x8E);
	set_gate(252, (uint32_t)interrupts_handler252, 0x08, 0x8E);
	set_gate(253, (uint32_t)interrupts_handler253, 0x08, 0x8E);
	set_gate(254, (uint32_t)interrupts_handler254, 0x08, 0x8E);
	set_gate(255, (uint32_t)interrupts_handler255, 0x08, 0x8E);

	idt_load(&idt_array);
}
