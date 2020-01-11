/* idt.c: Initialization of the IDT
 * Copyright © 2010 Christoph Sünderhauf
 * Copyright © 2011-2019 Lukas Martini
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

#include "i386-idt.h"
#include <int/int.h>
#include <log.h>
#include <string.h>
#include <portio.h>

struct idt_entry {
	uint16_t base_lo;	// The lower 16 bits of the address to jump to when this interrupt fires.
	uint16_t sel;		// Kernel segment selector.
	uint8_t  always0;	// This must always be zero.
	uint8_t  flags;		// More flags. See documentation.
	uint16_t base_hi;	// The upper 16 bits of the address to jump to.
} __attribute__((packed));

struct {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) lidt_pointer;

extern void int_i386_handler0(void);
extern void int_i386_handler1(void);
extern void int_i386_handler2(void);
extern void int_i386_handler3(void);
extern void int_i386_handler4(void);
extern void int_i386_handler5(void);
extern void int_i386_handler6(void);
extern void int_i386_handler7(void);
extern void int_i386_handler8(void);
extern void int_i386_handler9(void);
extern void int_i386_handler10(void);
extern void int_i386_handler11(void);
extern void int_i386_handler12(void);
extern void int_i386_handler13(void);
extern void int_i386_handler14(void);
extern void int_i386_handler15(void);
extern void int_i386_handler16(void);
extern void int_i386_handler17(void);
extern void int_i386_handler18(void);
extern void int_i386_handler19(void);
extern void int_i386_handler20(void);
extern void int_i386_handler21(void);
extern void int_i386_handler22(void);
extern void int_i386_handler23(void);
extern void int_i386_handler24(void);
extern void int_i386_handler25(void);
extern void int_i386_handler26(void);
extern void int_i386_handler27(void);
extern void int_i386_handler28(void);
extern void int_i386_handler29(void);
extern void int_i386_handler30(void);
extern void int_i386_handler31(void);
extern void int_i386_handler32(void);
extern void int_i386_handler33(void);
extern void int_i386_handler34(void);
extern void int_i386_handler35(void);
extern void int_i386_handler36(void);
extern void int_i386_handler37(void);
extern void int_i386_handler38(void);
extern void int_i386_handler39(void);
extern void int_i386_handler40(void);
extern void int_i386_handler41(void);
extern void int_i386_handler42(void);
extern void int_i386_handler43(void);
extern void int_i386_handler44(void);
extern void int_i386_handler45(void);
extern void int_i386_handler46(void);
extern void int_i386_handler47(void);
extern void int_i386_handler48(void);
extern void int_i386_handler49(void);
extern void int_i386_handler50(void);
extern void int_i386_handler51(void);
extern void int_i386_handler52(void);
extern void int_i386_handler53(void);
extern void int_i386_handler54(void);
extern void int_i386_handler55(void);
extern void int_i386_handler56(void);
extern void int_i386_handler57(void);
extern void int_i386_handler58(void);
extern void int_i386_handler59(void);
extern void int_i386_handler60(void);
extern void int_i386_handler61(void);
extern void int_i386_handler62(void);
extern void int_i386_handler63(void);
extern void int_i386_handler64(void);
extern void int_i386_handler65(void);
extern void int_i386_handler66(void);
extern void int_i386_handler67(void);
extern void int_i386_handler68(void);
extern void int_i386_handler69(void);
extern void int_i386_handler70(void);
extern void int_i386_handler71(void);
extern void int_i386_handler72(void);
extern void int_i386_handler73(void);
extern void int_i386_handler74(void);
extern void int_i386_handler75(void);
extern void int_i386_handler76(void);
extern void int_i386_handler77(void);
extern void int_i386_handler78(void);
extern void int_i386_handler79(void);
extern void int_i386_handler80(void);
extern void int_i386_handler81(void);
extern void int_i386_handler82(void);
extern void int_i386_handler83(void);
extern void int_i386_handler84(void);
extern void int_i386_handler85(void);
extern void int_i386_handler86(void);
extern void int_i386_handler87(void);
extern void int_i386_handler88(void);
extern void int_i386_handler89(void);
extern void int_i386_handler90(void);
extern void int_i386_handler91(void);
extern void int_i386_handler92(void);
extern void int_i386_handler93(void);
extern void int_i386_handler94(void);
extern void int_i386_handler95(void);
extern void int_i386_handler96(void);
extern void int_i386_handler97(void);
extern void int_i386_handler98(void);
extern void int_i386_handler99(void);
extern void int_i386_handler100(void);
extern void int_i386_handler101(void);
extern void int_i386_handler102(void);
extern void int_i386_handler103(void);
extern void int_i386_handler104(void);
extern void int_i386_handler105(void);
extern void int_i386_handler106(void);
extern void int_i386_handler107(void);
extern void int_i386_handler108(void);
extern void int_i386_handler109(void);
extern void int_i386_handler110(void);
extern void int_i386_handler111(void);
extern void int_i386_handler112(void);
extern void int_i386_handler113(void);
extern void int_i386_handler114(void);
extern void int_i386_handler115(void);
extern void int_i386_handler116(void);
extern void int_i386_handler117(void);
extern void int_i386_handler118(void);
extern void int_i386_handler119(void);
extern void int_i386_handler120(void);
extern void int_i386_handler121(void);
extern void int_i386_handler122(void);
extern void int_i386_handler123(void);
extern void int_i386_handler124(void);
extern void int_i386_handler125(void);
extern void int_i386_handler126(void);
extern void int_i386_handler127(void);
extern void int_i386_handler128(void);
extern void int_i386_handler129(void);
extern void int_i386_handler130(void);
extern void int_i386_handler131(void);
extern void int_i386_handler132(void);
extern void int_i386_handler133(void);
extern void int_i386_handler134(void);
extern void int_i386_handler135(void);
extern void int_i386_handler136(void);
extern void int_i386_handler137(void);
extern void int_i386_handler138(void);
extern void int_i386_handler139(void);
extern void int_i386_handler140(void);
extern void int_i386_handler141(void);
extern void int_i386_handler142(void);
extern void int_i386_handler143(void);
extern void int_i386_handler144(void);
extern void int_i386_handler145(void);
extern void int_i386_handler146(void);
extern void int_i386_handler147(void);
extern void int_i386_handler148(void);
extern void int_i386_handler149(void);
extern void int_i386_handler150(void);
extern void int_i386_handler151(void);
extern void int_i386_handler152(void);
extern void int_i386_handler153(void);
extern void int_i386_handler154(void);
extern void int_i386_handler155(void);
extern void int_i386_handler156(void);
extern void int_i386_handler157(void);
extern void int_i386_handler158(void);
extern void int_i386_handler159(void);
extern void int_i386_handler160(void);
extern void int_i386_handler161(void);
extern void int_i386_handler162(void);
extern void int_i386_handler163(void);
extern void int_i386_handler164(void);
extern void int_i386_handler165(void);
extern void int_i386_handler166(void);
extern void int_i386_handler167(void);
extern void int_i386_handler168(void);
extern void int_i386_handler169(void);
extern void int_i386_handler170(void);
extern void int_i386_handler171(void);
extern void int_i386_handler172(void);
extern void int_i386_handler173(void);
extern void int_i386_handler174(void);
extern void int_i386_handler175(void);
extern void int_i386_handler176(void);
extern void int_i386_handler177(void);
extern void int_i386_handler178(void);
extern void int_i386_handler179(void);
extern void int_i386_handler180(void);
extern void int_i386_handler181(void);
extern void int_i386_handler182(void);
extern void int_i386_handler183(void);
extern void int_i386_handler184(void);
extern void int_i386_handler185(void);
extern void int_i386_handler186(void);
extern void int_i386_handler187(void);
extern void int_i386_handler188(void);
extern void int_i386_handler189(void);
extern void int_i386_handler190(void);
extern void int_i386_handler191(void);
extern void int_i386_handler192(void);
extern void int_i386_handler193(void);
extern void int_i386_handler194(void);
extern void int_i386_handler195(void);
extern void int_i386_handler196(void);
extern void int_i386_handler197(void);
extern void int_i386_handler198(void);
extern void int_i386_handler199(void);
extern void int_i386_handler200(void);
extern void int_i386_handler201(void);
extern void int_i386_handler202(void);
extern void int_i386_handler203(void);
extern void int_i386_handler204(void);
extern void int_i386_handler205(void);
extern void int_i386_handler206(void);
extern void int_i386_handler207(void);
extern void int_i386_handler208(void);
extern void int_i386_handler209(void);
extern void int_i386_handler210(void);
extern void int_i386_handler211(void);
extern void int_i386_handler212(void);
extern void int_i386_handler213(void);
extern void int_i386_handler214(void);
extern void int_i386_handler215(void);
extern void int_i386_handler216(void);
extern void int_i386_handler217(void);
extern void int_i386_handler218(void);
extern void int_i386_handler219(void);
extern void int_i386_handler220(void);
extern void int_i386_handler221(void);
extern void int_i386_handler222(void);
extern void int_i386_handler223(void);
extern void int_i386_handler224(void);
extern void int_i386_handler225(void);
extern void int_i386_handler226(void);
extern void int_i386_handler227(void);
extern void int_i386_handler228(void);
extern void int_i386_handler229(void);
extern void int_i386_handler230(void);
extern void int_i386_handler231(void);
extern void int_i386_handler232(void);
extern void int_i386_handler233(void);
extern void int_i386_handler234(void);
extern void int_i386_handler235(void);
extern void int_i386_handler236(void);
extern void int_i386_handler237(void);
extern void int_i386_handler238(void);
extern void int_i386_handler239(void);
extern void int_i386_handler240(void);
extern void int_i386_handler241(void);
extern void int_i386_handler242(void);
extern void int_i386_handler243(void);
extern void int_i386_handler244(void);
extern void int_i386_handler245(void);
extern void int_i386_handler246(void);
extern void int_i386_handler247(void);
extern void int_i386_handler248(void);
extern void int_i386_handler249(void);
extern void int_i386_handler250(void);
extern void int_i386_handler251(void);
extern void int_i386_handler252(void);
extern void int_i386_handler253(void);
extern void int_i386_handler254(void);
extern void int_i386_handler255(void);

struct idt_entry idt_entries[256];

static void set_gate(uint8_t num, void (*handler)(void), uint8_t flags) {
	idt_entries[num].base_lo = (intptr_t)handler & 0xFFFF;
	idt_entries[num].base_hi = ((intptr_t)handler >> 16) & 0xFFFF;

	idt_entries[num].sel = 0x08;
	idt_entries[num].always0 = 0;

	idt_entries[num].flags = flags;
}

void idt_init() {
	lidt_pointer.limit = sizeof(struct idt_entry) * 256 -1;
	lidt_pointer.base = (uint32_t)&idt_entries;

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

	set_gate(0, &int_i386_handler0, 0x8E);
	set_gate(1, &int_i386_handler1, 0x8E);
	set_gate(2, &int_i386_handler2, 0x8E);
	set_gate(3, &int_i386_handler3, 0x8E);
	set_gate(4, &int_i386_handler4, 0x8E);
	set_gate(5, &int_i386_handler5, 0x8E);
	set_gate(6, &int_i386_handler6, 0x8E);
	set_gate(7, &int_i386_handler7, 0x8E);
	set_gate(8, &int_i386_handler8, 0x8E);
	set_gate(9, &int_i386_handler9, 0x8E);
	set_gate(10, &int_i386_handler10, 0x8E);
	set_gate(11, &int_i386_handler11, 0x8E);
	set_gate(12, &int_i386_handler12, 0x8E);
	set_gate(13, &int_i386_handler13, 0x8E);
	set_gate(14, &int_i386_handler14, 0x8E);
	set_gate(15, &int_i386_handler15, 0x8E);
	set_gate(16, &int_i386_handler16, 0x8E);
	set_gate(17, &int_i386_handler17, 0x8E);
	set_gate(18, &int_i386_handler18, 0x8E);
	set_gate(19, &int_i386_handler19, 0x8E);
	set_gate(20, &int_i386_handler20, 0x8E);
	set_gate(21, &int_i386_handler21, 0x8E);
	set_gate(22, &int_i386_handler22, 0x8E);
	set_gate(23, &int_i386_handler23, 0x8E);
	set_gate(24, &int_i386_handler24, 0x8E);
	set_gate(25, &int_i386_handler25, 0x8E);
	set_gate(26, &int_i386_handler26, 0x8E);
	set_gate(27, &int_i386_handler27, 0x8E);
	set_gate(28, &int_i386_handler28, 0x8E);
	set_gate(29, &int_i386_handler29, 0x8E);
	set_gate(30, &int_i386_handler30, 0x8E);
	set_gate(31, &int_i386_handler31, 0x8E);
	set_gate(32, &int_i386_handler32, 0x8E);
	set_gate(33, &int_i386_handler33, 0x8E);
	set_gate(34, &int_i386_handler34, 0x8E);
	set_gate(35, &int_i386_handler35, 0x8E);
	set_gate(36, &int_i386_handler36, 0x8E);
	set_gate(37, &int_i386_handler37, 0x8E);
	set_gate(38, &int_i386_handler38, 0x8E);
	set_gate(39, &int_i386_handler39, 0x8E);
	set_gate(40, &int_i386_handler40, 0x8E);
	set_gate(41, &int_i386_handler41, 0x8E);
	set_gate(42, &int_i386_handler42, 0x8E);
	set_gate(43, &int_i386_handler43, 0x8E);
	set_gate(44, &int_i386_handler44, 0x8E);
	set_gate(45, &int_i386_handler45, 0x8E);
	set_gate(46, &int_i386_handler46, 0x8E);
	set_gate(47, &int_i386_handler47, 0x8E);
	set_gate(48, &int_i386_handler48, 0x8E);
	// Scheduler yield
	set_gate(49, &int_i386_handler49, 0x8E | 0x60);
	set_gate(50, &int_i386_handler50, 0x8E);
	set_gate(51, &int_i386_handler51, 0x8E);
	set_gate(52, &int_i386_handler52, 0x8E);
	set_gate(53, &int_i386_handler53, 0x8E);
	set_gate(54, &int_i386_handler54, 0x8E);
	set_gate(55, &int_i386_handler55, 0x8E);
	set_gate(56, &int_i386_handler56, 0x8E);
	set_gate(57, &int_i386_handler57, 0x8E);
	set_gate(58, &int_i386_handler58, 0x8E);
	set_gate(59, &int_i386_handler59, 0x8E);
	set_gate(60, &int_i386_handler60, 0x8E);
	set_gate(61, &int_i386_handler61, 0x8E);
	set_gate(62, &int_i386_handler62, 0x8E);
	set_gate(63, &int_i386_handler63, 0x8E);
	set_gate(64, &int_i386_handler64, 0x8E);
	set_gate(65, &int_i386_handler65, 0x8E);
	set_gate(66, &int_i386_handler66, 0x8E);
	set_gate(67, &int_i386_handler67, 0x8E);
	set_gate(68, &int_i386_handler68, 0x8E);
	set_gate(69, &int_i386_handler69, 0x8E);
	set_gate(70, &int_i386_handler70, 0x8E);
	set_gate(71, &int_i386_handler71, 0x8E);
	set_gate(72, &int_i386_handler72, 0x8E);
	set_gate(73, &int_i386_handler73, 0x8E);
	set_gate(74, &int_i386_handler74, 0x8E);
	set_gate(75, &int_i386_handler75, 0x8E);
	set_gate(76, &int_i386_handler76, 0x8E);
	set_gate(77, &int_i386_handler77, 0x8E);
	set_gate(78, &int_i386_handler78, 0x8E);
	set_gate(79, &int_i386_handler79, 0x8E);
	set_gate(80, &int_i386_handler80, 0x8E);
	set_gate(81, &int_i386_handler81, 0x8E);
	set_gate(82, &int_i386_handler82, 0x8E);
	set_gate(83, &int_i386_handler83, 0x8E);
	set_gate(84, &int_i386_handler84, 0x8E);
	set_gate(85, &int_i386_handler85, 0x8E);
	set_gate(86, &int_i386_handler86, 0x8E);
	set_gate(87, &int_i386_handler87, 0x8E);
	set_gate(88, &int_i386_handler88, 0x8E);
	set_gate(89, &int_i386_handler89, 0x8E);
	set_gate(90, &int_i386_handler90, 0x8E);
	set_gate(91, &int_i386_handler91, 0x8E);
	set_gate(92, &int_i386_handler92, 0x8E);
	set_gate(93, &int_i386_handler93, 0x8E);
	set_gate(94, &int_i386_handler94, 0x8E);
	set_gate(95, &int_i386_handler95, 0x8E);
	set_gate(96, &int_i386_handler96, 0x8E);
	set_gate(97, &int_i386_handler97, 0x8E);
	set_gate(98, &int_i386_handler98, 0x8E);
	set_gate(99, &int_i386_handler99, 0x8E);
	set_gate(100, &int_i386_handler100, 0x8E);
	set_gate(101, &int_i386_handler101, 0x8E);
	set_gate(102, &int_i386_handler102, 0x8E);
	set_gate(103, &int_i386_handler103, 0x8E);
	set_gate(104, &int_i386_handler104, 0x8E);
	set_gate(105, &int_i386_handler105, 0x8E);
	set_gate(106, &int_i386_handler106, 0x8E);
	set_gate(107, &int_i386_handler107, 0x8E);
	set_gate(108, &int_i386_handler108, 0x8E);
	set_gate(109, &int_i386_handler109, 0x8E);
	set_gate(110, &int_i386_handler110, 0x8E);
	set_gate(111, &int_i386_handler111, 0x8E);
	set_gate(112, &int_i386_handler112, 0x8E);
	set_gate(113, &int_i386_handler113, 0x8E);
	set_gate(114, &int_i386_handler114, 0x8E);
	set_gate(115, &int_i386_handler115, 0x8E);
	set_gate(116, &int_i386_handler116, 0x8E);
	set_gate(117, &int_i386_handler117, 0x8E);
	set_gate(118, &int_i386_handler118, 0x8E);
	set_gate(119, &int_i386_handler119, 0x8E);
	set_gate(120, &int_i386_handler120, 0x8E);
	set_gate(121, &int_i386_handler121, 0x8E);
	set_gate(122, &int_i386_handler122, 0x8E);
	set_gate(123, &int_i386_handler123, 0x8E);
	set_gate(124, &int_i386_handler124, 0x8E);
	set_gate(125, &int_i386_handler125, 0x8E);
	set_gate(126, &int_i386_handler126, 0x8E);
	set_gate(127, &int_i386_handler127, 0x8E);
	// Syscall, make this one callable from ring 3.
	set_gate(128, &int_i386_handler128, 0x8E | 0x60);
	set_gate(129, &int_i386_handler129, 0x8E);
	set_gate(130, &int_i386_handler130, 0x8E);
	set_gate(131, &int_i386_handler131, 0x8E);
	set_gate(132, &int_i386_handler132, 0x8E);
	set_gate(133, &int_i386_handler133, 0x8E);
	set_gate(134, &int_i386_handler134, 0x8E);
	set_gate(135, &int_i386_handler135, 0x8E);
	set_gate(136, &int_i386_handler136, 0x8E);
	set_gate(137, &int_i386_handler137, 0x8E);
	set_gate(138, &int_i386_handler138, 0x8E);
	set_gate(139, &int_i386_handler139, 0x8E);
	set_gate(140, &int_i386_handler140, 0x8E);
	set_gate(141, &int_i386_handler141, 0x8E);
	set_gate(142, &int_i386_handler142, 0x8E);
	set_gate(143, &int_i386_handler143, 0x8E);
	set_gate(144, &int_i386_handler144, 0x8E);
	set_gate(145, &int_i386_handler145, 0x8E);
	set_gate(146, &int_i386_handler146, 0x8E);
	set_gate(147, &int_i386_handler147, 0x8E);
	set_gate(148, &int_i386_handler148, 0x8E);
	set_gate(149, &int_i386_handler149, 0x8E);
	set_gate(150, &int_i386_handler150, 0x8E);
	set_gate(151, &int_i386_handler151, 0x8E);
	set_gate(152, &int_i386_handler152, 0x8E);
	set_gate(153, &int_i386_handler153, 0x8E);
	set_gate(154, &int_i386_handler154, 0x8E);
	set_gate(155, &int_i386_handler155, 0x8E);
	set_gate(156, &int_i386_handler156, 0x8E);
	set_gate(157, &int_i386_handler157, 0x8E);
	set_gate(158, &int_i386_handler158, 0x8E);
	set_gate(159, &int_i386_handler159, 0x8E);
	set_gate(160, &int_i386_handler160, 0x8E);
	set_gate(161, &int_i386_handler161, 0x8E);
	set_gate(162, &int_i386_handler162, 0x8E);
	set_gate(163, &int_i386_handler163, 0x8E);
	set_gate(164, &int_i386_handler164, 0x8E);
	set_gate(165, &int_i386_handler165, 0x8E);
	set_gate(166, &int_i386_handler166, 0x8E);
	set_gate(167, &int_i386_handler167, 0x8E);
	set_gate(168, &int_i386_handler168, 0x8E);
	set_gate(169, &int_i386_handler169, 0x8E);
	set_gate(170, &int_i386_handler170, 0x8E);
	set_gate(171, &int_i386_handler171, 0x8E);
	set_gate(172, &int_i386_handler172, 0x8E);
	set_gate(173, &int_i386_handler173, 0x8E);
	set_gate(174, &int_i386_handler174, 0x8E);
	set_gate(175, &int_i386_handler175, 0x8E);
	set_gate(176, &int_i386_handler176, 0x8E);
	set_gate(177, &int_i386_handler177, 0x8E);
	set_gate(178, &int_i386_handler178, 0x8E);
	set_gate(179, &int_i386_handler179, 0x8E);
	set_gate(180, &int_i386_handler180, 0x8E);
	set_gate(181, &int_i386_handler181, 0x8E);
	set_gate(182, &int_i386_handler182, 0x8E);
	set_gate(183, &int_i386_handler183, 0x8E);
	set_gate(184, &int_i386_handler184, 0x8E);
	set_gate(185, &int_i386_handler185, 0x8E);
	set_gate(186, &int_i386_handler186, 0x8E);
	set_gate(187, &int_i386_handler187, 0x8E);
	set_gate(188, &int_i386_handler188, 0x8E);
	set_gate(189, &int_i386_handler189, 0x8E);
	set_gate(190, &int_i386_handler190, 0x8E);
	set_gate(191, &int_i386_handler191, 0x8E);
	set_gate(192, &int_i386_handler192, 0x8E);
	set_gate(193, &int_i386_handler193, 0x8E);
	set_gate(194, &int_i386_handler194, 0x8E);
	set_gate(195, &int_i386_handler195, 0x8E);
	set_gate(196, &int_i386_handler196, 0x8E);
	set_gate(197, &int_i386_handler197, 0x8E);
	set_gate(198, &int_i386_handler198, 0x8E);
	set_gate(199, &int_i386_handler199, 0x8E);
	set_gate(200, &int_i386_handler200, 0x8E);
	set_gate(201, &int_i386_handler201, 0x8E);
	set_gate(202, &int_i386_handler202, 0x8E);
	set_gate(203, &int_i386_handler203, 0x8E);
	set_gate(204, &int_i386_handler204, 0x8E);
	set_gate(205, &int_i386_handler205, 0x8E);
	set_gate(206, &int_i386_handler206, 0x8E);
	set_gate(207, &int_i386_handler207, 0x8E);
	set_gate(208, &int_i386_handler208, 0x8E);
	set_gate(209, &int_i386_handler209, 0x8E);
	set_gate(210, &int_i386_handler210, 0x8E);
	set_gate(211, &int_i386_handler211, 0x8E);
	set_gate(212, &int_i386_handler212, 0x8E);
	set_gate(213, &int_i386_handler213, 0x8E);
	set_gate(214, &int_i386_handler214, 0x8E);
	set_gate(215, &int_i386_handler215, 0x8E);
	set_gate(216, &int_i386_handler216, 0x8E);
	set_gate(217, &int_i386_handler217, 0x8E);
	set_gate(218, &int_i386_handler218, 0x8E);
	set_gate(219, &int_i386_handler219, 0x8E);
	set_gate(220, &int_i386_handler220, 0x8E);
	set_gate(221, &int_i386_handler221, 0x8E);
	set_gate(222, &int_i386_handler222, 0x8E);
	set_gate(223, &int_i386_handler223, 0x8E);
	set_gate(224, &int_i386_handler224, 0x8E);
	set_gate(225, &int_i386_handler225, 0x8E);
	set_gate(226, &int_i386_handler226, 0x8E);
	set_gate(227, &int_i386_handler227, 0x8E);
	set_gate(228, &int_i386_handler228, 0x8E);
	set_gate(229, &int_i386_handler229, 0x8E);
	set_gate(230, &int_i386_handler230, 0x8E);
	set_gate(231, &int_i386_handler231, 0x8E);
	set_gate(232, &int_i386_handler232, 0x8E);
	set_gate(233, &int_i386_handler233, 0x8E);
	set_gate(234, &int_i386_handler234, 0x8E);
	set_gate(235, &int_i386_handler235, 0x8E);
	set_gate(236, &int_i386_handler236, 0x8E);
	set_gate(237, &int_i386_handler237, 0x8E);
	set_gate(238, &int_i386_handler238, 0x8E);
	set_gate(239, &int_i386_handler239, 0x8E);
	set_gate(240, &int_i386_handler240, 0x8E);
	set_gate(241, &int_i386_handler241, 0x8E);
	set_gate(242, &int_i386_handler242, 0x8E);
	set_gate(243, &int_i386_handler243, 0x8E);
	set_gate(244, &int_i386_handler244, 0x8E);
	set_gate(245, &int_i386_handler245, 0x8E);
	set_gate(246, &int_i386_handler246, 0x8E);
	set_gate(247, &int_i386_handler247, 0x8E);
	set_gate(248, &int_i386_handler248, 0x8E);
	set_gate(249, &int_i386_handler249, 0x8E);
	set_gate(250, &int_i386_handler250, 0x8E);
	set_gate(251, &int_i386_handler251, 0x8E);
	set_gate(252, &int_i386_handler252, 0x8E);
	set_gate(253, &int_i386_handler253, 0x8E);
	set_gate(254, &int_i386_handler254, 0x8E);
	set_gate(255, &int_i386_handler255, 0x8E);

	log(LOG_INFO, "interrupts: Setting IDT, descriptor=%#x, limit=%#x, base=%#x\n",
		&lidt_pointer, lidt_pointer.limit,lidt_pointer.base);

	asm volatile("lidt (%0);":: "m" (lidt_pointer));
}
