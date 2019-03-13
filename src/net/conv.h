#pragma once

/* Copyright © 2019 Lukas Martini
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

#include <net/socket.h>
#include <pico_socket.h>

int net_bsd_to_pico_addr(union pico_address *addr, const struct sockaddr *_saddr, socklen_t socklen);
uint16_t net_bsd_to_pico_port(const struct sockaddr *_saddr, socklen_t socklen);
int net_conv_pico2bsd(struct sockaddr *_saddr, socklen_t socklen, union pico_address *addr, uint16_t port);
