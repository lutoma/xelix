/* fault.c: Catch and process CPU fault interrupts
 * Copyright © 2019 Lukas Martini
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

#include <panic.h>
#include <printf.h>

void decode_fault_status(uint32_t fsr) {
    uint32_t status = ((fsr & (1 << 10)) >> 6) | (fsr & 0x7);
    switch (status) {
    case 0b00001:
	serial_printf("alignment fault\n");
	break;
    case 0b00100:
	serial_printf("fault on instruction cache maintenance\n");
	break;
    case 0b01100:
	serial_printf("synchronnous external abort, first level translation fault\n");
	break;
    case 0b01110:
	serial_printf("synchronnous external abort, second level translation fault\n");
	break;
    case 0b11100:
	serial_printf("synchronnous parity error, first level translation fault\n");
	break;
    case 0b11110:
	serial_printf("synchronnous parity error, second level translation fault\n");
	break;

    case 0b00101:
	serial_printf("first level translation fault\n");
	break;
    case 0b00111:
	serial_printf("second level translation fault\n");
	break;
    case 0b00011:
	serial_printf("first level access flag fault\n");
	break;
    case 0b00110:
	serial_printf("second level access flag fault\n");
	break;
    case 0b01001:
	serial_printf("first level domain fault, domain %ld\n", (fsr & 0xF0) >> 4);
	break;
    case 0b01011:
	serial_printf("second level domain fault, domain %ld\n", (fsr & 0xF0) >> 4);
	break;
    case 0b01101:
	serial_printf("first level permission fault\n");
	break;
    case 0b01111:
	serial_printf("second level permission fault\n");
	break;
    case 0b00010:
	serial_printf("debug event\n");
	break;
    case 0b01000:
	serial_printf("synchronous external abort\n");
	break;
    case 0b10000:
	serial_printf("TLB conflict event\n");
	break;
    case 0b11001:
	serial_printf("synchronous parity error on memory access\n");
	break;
    case 0b10110:
	serial_printf("asynchronous external abort\n");
	break;
    case 0b11000:
	serial_printf("asynchronous parity error on memory access\n");
	break;
    default:
	serial_printf("unknown / reserved fault\n");
    }
}


void cpu_fault_handler(uint32_t ifsr, uint32_t ifar) {
    serial_printf("ifsr %#x, ifar %#x\n", ifsr, ifar);
    decode_fault_status(ifsr);
	panic("CPU fault");
}
