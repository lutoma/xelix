/* idt.c: Initialization of the IDT
 * Copyright © 2010 Christoph Sünderhauf
 * Copyright © 2011-2018 Lukas Martini
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
#include <log.h>
#include <string.h>

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

	set_gate(0, &interrupts_handler0, 0x8E);
	set_gate(1, &interrupts_handler1, 0x8E);
	set_gate(2, &interrupts_handler2, 0x8E);
	set_gate(3, &interrupts_handler3, 0x8E);
	set_gate(4, &interrupts_handler4, 0x8E);
	set_gate(5, &interrupts_handler5, 0x8E);
	set_gate(6, &interrupts_handler6, 0x8E);
	set_gate(7, &interrupts_handler7, 0x8E);
	set_gate(8, &interrupts_handler8, 0x8E);
	set_gate(9, &interrupts_handler9, 0x8E);
	set_gate(10, &interrupts_handler10, 0x8E);
	set_gate(11, &interrupts_handler11, 0x8E);
	set_gate(12, &interrupts_handler12, 0x8E);
	set_gate(13, &interrupts_handler13, 0x8E);
	set_gate(14, &interrupts_handler14, 0x8E);
	set_gate(15, &interrupts_handler15, 0x8E);
	set_gate(16, &interrupts_handler16, 0x8E);
	set_gate(17, &interrupts_handler17, 0x8E);
	set_gate(18, &interrupts_handler18, 0x8E);
	set_gate(19, &interrupts_handler19, 0x8E);
	set_gate(20, &interrupts_handler20, 0x8E);
	set_gate(21, &interrupts_handler21, 0x8E);
	set_gate(22, &interrupts_handler22, 0x8E);
	set_gate(23, &interrupts_handler23, 0x8E);
	set_gate(24, &interrupts_handler24, 0x8E);
	set_gate(25, &interrupts_handler25, 0x8E);
	set_gate(26, &interrupts_handler26, 0x8E);
	set_gate(27, &interrupts_handler27, 0x8E);
	set_gate(28, &interrupts_handler28, 0x8E);
	set_gate(29, &interrupts_handler29, 0x8E);
	set_gate(30, &interrupts_handler30, 0x8E);
	set_gate(31, &interrupts_handler31, 0x8E);
	set_gate(32, &interrupts_handler32, 0x8E);
	set_gate(33, &interrupts_handler33, 0x8E);
	set_gate(34, &interrupts_handler34, 0x8E);
	set_gate(35, &interrupts_handler35, 0x8E);
	set_gate(36, &interrupts_handler36, 0x8E);
	set_gate(37, &interrupts_handler37, 0x8E);
	set_gate(38, &interrupts_handler38, 0x8E);
	set_gate(39, &interrupts_handler39, 0x8E);
	set_gate(40, &interrupts_handler40, 0x8E);
	set_gate(41, &interrupts_handler41, 0x8E);
	set_gate(42, &interrupts_handler42, 0x8E);
	set_gate(43, &interrupts_handler43, 0x8E);
	set_gate(44, &interrupts_handler44, 0x8E);
	set_gate(45, &interrupts_handler45, 0x8E);
	set_gate(46, &interrupts_handler46, 0x8E);
	set_gate(47, &interrupts_handler47, 0x8E);
	set_gate(48, &interrupts_handler48, 0x8E);
	// Scheduler yield
	set_gate(49, &interrupts_handler49, 0x8E | 0x60);
	set_gate(50, &interrupts_handler50, 0x8E);
	set_gate(51, &interrupts_handler51, 0x8E);
	set_gate(52, &interrupts_handler52, 0x8E);
	set_gate(53, &interrupts_handler53, 0x8E);
	set_gate(54, &interrupts_handler54, 0x8E);
	set_gate(55, &interrupts_handler55, 0x8E);
	set_gate(56, &interrupts_handler56, 0x8E);
	set_gate(57, &interrupts_handler57, 0x8E);
	set_gate(58, &interrupts_handler58, 0x8E);
	set_gate(59, &interrupts_handler59, 0x8E);
	set_gate(60, &interrupts_handler60, 0x8E);
	set_gate(61, &interrupts_handler61, 0x8E);
	set_gate(62, &interrupts_handler62, 0x8E);
	set_gate(63, &interrupts_handler63, 0x8E);
	set_gate(64, &interrupts_handler64, 0x8E);
	set_gate(65, &interrupts_handler65, 0x8E);
	set_gate(66, &interrupts_handler66, 0x8E);
	set_gate(67, &interrupts_handler67, 0x8E);
	set_gate(68, &interrupts_handler68, 0x8E);
	set_gate(69, &interrupts_handler69, 0x8E);
	set_gate(70, &interrupts_handler70, 0x8E);
	set_gate(71, &interrupts_handler71, 0x8E);
	set_gate(72, &interrupts_handler72, 0x8E);
	set_gate(73, &interrupts_handler73, 0x8E);
	set_gate(74, &interrupts_handler74, 0x8E);
	set_gate(75, &interrupts_handler75, 0x8E);
	set_gate(76, &interrupts_handler76, 0x8E);
	set_gate(77, &interrupts_handler77, 0x8E);
	set_gate(78, &interrupts_handler78, 0x8E);
	set_gate(79, &interrupts_handler79, 0x8E);
	set_gate(80, &interrupts_handler80, 0x8E);
	set_gate(81, &interrupts_handler81, 0x8E);
	set_gate(82, &interrupts_handler82, 0x8E);
	set_gate(83, &interrupts_handler83, 0x8E);
	set_gate(84, &interrupts_handler84, 0x8E);
	set_gate(85, &interrupts_handler85, 0x8E);
	set_gate(86, &interrupts_handler86, 0x8E);
	set_gate(87, &interrupts_handler87, 0x8E);
	set_gate(88, &interrupts_handler88, 0x8E);
	set_gate(89, &interrupts_handler89, 0x8E);
	set_gate(90, &interrupts_handler90, 0x8E);
	set_gate(91, &interrupts_handler91, 0x8E);
	set_gate(92, &interrupts_handler92, 0x8E);
	set_gate(93, &interrupts_handler93, 0x8E);
	set_gate(94, &interrupts_handler94, 0x8E);
	set_gate(95, &interrupts_handler95, 0x8E);
	set_gate(96, &interrupts_handler96, 0x8E);
	set_gate(97, &interrupts_handler97, 0x8E);
	set_gate(98, &interrupts_handler98, 0x8E);
	set_gate(99, &interrupts_handler99, 0x8E);
	set_gate(100, &interrupts_handler100, 0x8E);
	set_gate(101, &interrupts_handler101, 0x8E);
	set_gate(102, &interrupts_handler102, 0x8E);
	set_gate(103, &interrupts_handler103, 0x8E);
	set_gate(104, &interrupts_handler104, 0x8E);
	set_gate(105, &interrupts_handler105, 0x8E);
	set_gate(106, &interrupts_handler106, 0x8E);
	set_gate(107, &interrupts_handler107, 0x8E);
	set_gate(108, &interrupts_handler108, 0x8E);
	set_gate(109, &interrupts_handler109, 0x8E);
	set_gate(110, &interrupts_handler110, 0x8E);
	set_gate(111, &interrupts_handler111, 0x8E);
	set_gate(112, &interrupts_handler112, 0x8E);
	set_gate(113, &interrupts_handler113, 0x8E);
	set_gate(114, &interrupts_handler114, 0x8E);
	set_gate(115, &interrupts_handler115, 0x8E);
	set_gate(116, &interrupts_handler116, 0x8E);
	set_gate(117, &interrupts_handler117, 0x8E);
	set_gate(118, &interrupts_handler118, 0x8E);
	set_gate(119, &interrupts_handler119, 0x8E);
	set_gate(120, &interrupts_handler120, 0x8E);
	set_gate(121, &interrupts_handler121, 0x8E);
	set_gate(122, &interrupts_handler122, 0x8E);
	set_gate(123, &interrupts_handler123, 0x8E);
	set_gate(124, &interrupts_handler124, 0x8E);
	set_gate(125, &interrupts_handler125, 0x8E);
	set_gate(126, &interrupts_handler126, 0x8E);
	set_gate(127, &interrupts_handler127, 0x8E);
	// Syscall, make this one callable from ring 3.
	set_gate(128, &interrupts_handler128, 0x8E | 0x60);
	set_gate(129, &interrupts_handler129, 0x8E);
	set_gate(130, &interrupts_handler130, 0x8E);
	set_gate(131, &interrupts_handler131, 0x8E);
	set_gate(132, &interrupts_handler132, 0x8E);
	set_gate(133, &interrupts_handler133, 0x8E);
	set_gate(134, &interrupts_handler134, 0x8E);
	set_gate(135, &interrupts_handler135, 0x8E);
	set_gate(136, &interrupts_handler136, 0x8E);
	set_gate(137, &interrupts_handler137, 0x8E);
	set_gate(138, &interrupts_handler138, 0x8E);
	set_gate(139, &interrupts_handler139, 0x8E);
	set_gate(140, &interrupts_handler140, 0x8E);
	set_gate(141, &interrupts_handler141, 0x8E);
	set_gate(142, &interrupts_handler142, 0x8E);
	set_gate(143, &interrupts_handler143, 0x8E);
	set_gate(144, &interrupts_handler144, 0x8E);
	set_gate(145, &interrupts_handler145, 0x8E);
	set_gate(146, &interrupts_handler146, 0x8E);
	set_gate(147, &interrupts_handler147, 0x8E);
	set_gate(148, &interrupts_handler148, 0x8E);
	set_gate(149, &interrupts_handler149, 0x8E);
	set_gate(150, &interrupts_handler150, 0x8E);
	set_gate(151, &interrupts_handler151, 0x8E);
	set_gate(152, &interrupts_handler152, 0x8E);
	set_gate(153, &interrupts_handler153, 0x8E);
	set_gate(154, &interrupts_handler154, 0x8E);
	set_gate(155, &interrupts_handler155, 0x8E);
	set_gate(156, &interrupts_handler156, 0x8E);
	set_gate(157, &interrupts_handler157, 0x8E);
	set_gate(158, &interrupts_handler158, 0x8E);
	set_gate(159, &interrupts_handler159, 0x8E);
	set_gate(160, &interrupts_handler160, 0x8E);
	set_gate(161, &interrupts_handler161, 0x8E);
	set_gate(162, &interrupts_handler162, 0x8E);
	set_gate(163, &interrupts_handler163, 0x8E);
	set_gate(164, &interrupts_handler164, 0x8E);
	set_gate(165, &interrupts_handler165, 0x8E);
	set_gate(166, &interrupts_handler166, 0x8E);
	set_gate(167, &interrupts_handler167, 0x8E);
	set_gate(168, &interrupts_handler168, 0x8E);
	set_gate(169, &interrupts_handler169, 0x8E);
	set_gate(170, &interrupts_handler170, 0x8E);
	set_gate(171, &interrupts_handler171, 0x8E);
	set_gate(172, &interrupts_handler172, 0x8E);
	set_gate(173, &interrupts_handler173, 0x8E);
	set_gate(174, &interrupts_handler174, 0x8E);
	set_gate(175, &interrupts_handler175, 0x8E);
	set_gate(176, &interrupts_handler176, 0x8E);
	set_gate(177, &interrupts_handler177, 0x8E);
	set_gate(178, &interrupts_handler178, 0x8E);
	set_gate(179, &interrupts_handler179, 0x8E);
	set_gate(180, &interrupts_handler180, 0x8E);
	set_gate(181, &interrupts_handler181, 0x8E);
	set_gate(182, &interrupts_handler182, 0x8E);
	set_gate(183, &interrupts_handler183, 0x8E);
	set_gate(184, &interrupts_handler184, 0x8E);
	set_gate(185, &interrupts_handler185, 0x8E);
	set_gate(186, &interrupts_handler186, 0x8E);
	set_gate(187, &interrupts_handler187, 0x8E);
	set_gate(188, &interrupts_handler188, 0x8E);
	set_gate(189, &interrupts_handler189, 0x8E);
	set_gate(190, &interrupts_handler190, 0x8E);
	set_gate(191, &interrupts_handler191, 0x8E);
	set_gate(192, &interrupts_handler192, 0x8E);
	set_gate(193, &interrupts_handler193, 0x8E);
	set_gate(194, &interrupts_handler194, 0x8E);
	set_gate(195, &interrupts_handler195, 0x8E);
	set_gate(196, &interrupts_handler196, 0x8E);
	set_gate(197, &interrupts_handler197, 0x8E);
	set_gate(198, &interrupts_handler198, 0x8E);
	set_gate(199, &interrupts_handler199, 0x8E);
	set_gate(200, &interrupts_handler200, 0x8E);
	set_gate(201, &interrupts_handler201, 0x8E);
	set_gate(202, &interrupts_handler202, 0x8E);
	set_gate(203, &interrupts_handler203, 0x8E);
	set_gate(204, &interrupts_handler204, 0x8E);
	set_gate(205, &interrupts_handler205, 0x8E);
	set_gate(206, &interrupts_handler206, 0x8E);
	set_gate(207, &interrupts_handler207, 0x8E);
	set_gate(208, &interrupts_handler208, 0x8E);
	set_gate(209, &interrupts_handler209, 0x8E);
	set_gate(210, &interrupts_handler210, 0x8E);
	set_gate(211, &interrupts_handler211, 0x8E);
	set_gate(212, &interrupts_handler212, 0x8E);
	set_gate(213, &interrupts_handler213, 0x8E);
	set_gate(214, &interrupts_handler214, 0x8E);
	set_gate(215, &interrupts_handler215, 0x8E);
	set_gate(216, &interrupts_handler216, 0x8E);
	set_gate(217, &interrupts_handler217, 0x8E);
	set_gate(218, &interrupts_handler218, 0x8E);
	set_gate(219, &interrupts_handler219, 0x8E);
	set_gate(220, &interrupts_handler220, 0x8E);
	set_gate(221, &interrupts_handler221, 0x8E);
	set_gate(222, &interrupts_handler222, 0x8E);
	set_gate(223, &interrupts_handler223, 0x8E);
	set_gate(224, &interrupts_handler224, 0x8E);
	set_gate(225, &interrupts_handler225, 0x8E);
	set_gate(226, &interrupts_handler226, 0x8E);
	set_gate(227, &interrupts_handler227, 0x8E);
	set_gate(228, &interrupts_handler228, 0x8E);
	set_gate(229, &interrupts_handler229, 0x8E);
	set_gate(230, &interrupts_handler230, 0x8E);
	set_gate(231, &interrupts_handler231, 0x8E);
	set_gate(232, &interrupts_handler232, 0x8E);
	set_gate(233, &interrupts_handler233, 0x8E);
	set_gate(234, &interrupts_handler234, 0x8E);
	set_gate(235, &interrupts_handler235, 0x8E);
	set_gate(236, &interrupts_handler236, 0x8E);
	set_gate(237, &interrupts_handler237, 0x8E);
	set_gate(238, &interrupts_handler238, 0x8E);
	set_gate(239, &interrupts_handler239, 0x8E);
	set_gate(240, &interrupts_handler240, 0x8E);
	set_gate(241, &interrupts_handler241, 0x8E);
	set_gate(242, &interrupts_handler242, 0x8E);
	set_gate(243, &interrupts_handler243, 0x8E);
	set_gate(244, &interrupts_handler244, 0x8E);
	set_gate(245, &interrupts_handler245, 0x8E);
	set_gate(246, &interrupts_handler246, 0x8E);
	set_gate(247, &interrupts_handler247, 0x8E);
	set_gate(248, &interrupts_handler248, 0x8E);
	set_gate(249, &interrupts_handler249, 0x8E);
	set_gate(250, &interrupts_handler250, 0x8E);
	set_gate(251, &interrupts_handler251, 0x8E);
	set_gate(252, &interrupts_handler252, 0x8E);
	set_gate(253, &interrupts_handler253, 0x8E);
	set_gate(254, &interrupts_handler254, 0x8E);
	set_gate(255, &interrupts_handler255, 0x8E);

	idt_load(&lidt_pointer);
}
