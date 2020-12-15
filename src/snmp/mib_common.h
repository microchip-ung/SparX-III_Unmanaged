//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#ifndef MIB_COMMON_H
#define MIB_COMMON_H

void mib_common_get_if_descr (uchar port_no, uchar xdata * dest);
ulong mib_common_get_ip_if_index (void);
void mib_common_get_sys_descr (uchar xdata * dest);
#endif
