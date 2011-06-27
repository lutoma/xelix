#pragma once

/* Copyright © 2011 Fritz Grimpen
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include <lib/generic.h>

void pci_init();
void pci_scan();

uint32_t pci_configRead(uint16_t bus, uint16_t dev, uint16_t func, uint16_t offset);
void pci_configWrite(uint16_t bus, uint16_t dev, uint16_t func, uint16_t offset, uint32_t val);
uint16_t pci_getVendorId(uint16_t bus, uint16_t dev, uint16_t func);
uint16_t pci_getDeviceId(uint16_t bus, uint16_t dev, uint16_t func);

