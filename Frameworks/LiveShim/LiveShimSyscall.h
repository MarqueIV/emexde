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

/* additional nyxian syscalls for now */
#define SYS_proctb      750         /* MARK: deprecated.. use SYS_sysctl instead */
#define SYS_getent      751         /* getting processes entitlements */
#define SYS_gethostname 752         /* MARK: deprecated.. use SYS_sysctl instead */
#define SYS_sethostname 753         /* MARK: deprecated.. use SYS_sysctl instead */
#define SYS_gettask     754         /* gets task port */
#define SYS_procpath    755         /* gets process path of a pid */
#define SYS_procbsd     756         /* MARK: deprecated.. use SYS_sysctl instead */
#define SYS_handoffep   757         /* handoff exception port to kvirt */
#define SYS_setent      758         /* sets entitlements (sanitized ofc) */
#define SYS_waittask    759         /* waits till task port of a task is available */
#define SYS_pectl       760         /* utility for many proc environment operations */

/* launch services */
#define PECTL_LS_SET_ENDPOINT   0b00000000  /* sets the endpoint of a launch service identifier (i.e. com.mycompany.daemon) */
#define PECTL_LS_GET_ENDPOINT   0b00000001  /* gets the endpoint of a launch service identifier (i.e. org.emexlabs.containerd) */

/* environment */
#define PECTL_PE_SET_BAMSET     0b00000010  /* sets background audio mode (i.e Spotify playing music in background) MARK: noop currently */

/* code signing */
#define PECTL_CS_GET_PUBKEY     0b00000011  /* getting the code signature public key                                        */
#define PECTL_CS_GET_PRVKEY     0b00000100  /* noop                                                                         */
#define PECTL_CS_SIGN_PATH      0b00000101  /* signs executable at a specific path                                          */

#define PECTL_PE_UIAPP_RUN      0b00000110

#define PECTL_CS_GET_CDHASH     0b00000111  /* gets cdhash of currently running executable */
#define PECTL_CS_FALLBACK_ENT   0b00001000  /* sets entitlements to none as a fallback */

#define PECTL_USREBOOT          0b00001001  /* reboots userspace (platform processes only) */
#define PECTL_GET_USMODE        0b00001010  /* gets userspace mode */

#define PECTL_GET_BTYPE         0b00001011  /* gets build type */
#define PECTL_GET_ALLENT        0b00001100  /* gets a entitlements mask with all currently supported entitlements */

typedef struct syscall_client syscall_client_t;

extern syscall_client_t *syscallProxy;

syscall_client_t *liveshim_syscall_client_create(mach_port_t port);
int64_t liveshim_syscall_invoke(syscall_client_t *client, uint32_t syscall_num, int64_t *args, mach_port_t *in_ports, uint32_t in_ports_cnt, mach_msg_type_name_t in_type, mach_port_t **out_ports, uint32_t out_ports_cnt);

int64_t liveshim_syscall(uint32_t syscall_num, ...);

#endif /* LIVESHIMPREFS_H */
