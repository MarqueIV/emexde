/*
 SPDX-License-Identifier: AGPL-3.0-or-later

 Copyright (C) 2025 - 2026 emexlab

 This file is part of Nyxian.

 Nyxian is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Nyxian is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU Affero General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with Nyxian. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef LIVESHIMPREFS_H
#define LIVESHIMPREFS_H

#include <stdint.h>
#include <mach/mach.h>
#include <sys/syscall.h>

typedef struct syscall_client syscall_client_t;

extern syscall_client_t *syscallProxy;

syscall_client_t *liveshim_syscall_client_create(mach_port_t port);
int64_t liveshim_syscall_invoke(syscall_client_t *client, uint32_t syscall_num, int64_t *args, mach_port_t *in_ports, uint32_t in_ports_cnt, mach_msg_type_name_t in_type, mach_port_t **out_ports, uint32_t out_ports_cnt);

int64_t liveshim_syscall(uint32_t syscall_num, ...);

#endif /* LIVESHIMPREFS_H */
