//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT


#include "common.h"     /* Always include common.h at the first place of user-defined herder files */

#include "uartdrv.h"
#include "print.h"
#include "snmpconfig.h"
#include "txt.h"
#include "version.h"

#ifndef TRANSIT_SNMP
#define TRANSIT_SNMP 0
#endif

#if TRANSIT_SNMP || TRANSIT_LLDP
/*****************************************************************************
 *
 *
 * Defines
 *
 *
 *
 ****************************************************************************/

/*****************************************************************************
 *
 *
 * Typedefs and enums
 *
 *
 *
 ****************************************************************************/

/*****************************************************************************
 *
 *
 * Prototypes for local functions
 *
 *
 *
 ****************************************************************************/

#if !TRANSIT_LLDP_REDUCED
void mib_common_get_if_descr (uchar port_no, uchar xdata * dest)
{
    uart_redirect(dest);
    print_str(SNMP_IFTABLE_DESCR_PREFIX);
    print_dec(port_no);
    uart_redirect(0);
}

ulong mib_common_get_ip_if_index (void)
{
    return SNMP_IFTABLE_CPU_INDEX;
}

void mib_common_get_sys_descr (uchar xdata * dest)
{
    uart_redirect(dest);
    print_txt(TXT_NO_SWITCH_NAME);
    print_spaces(1);
    print_ch('-');
    print_spaces(1);
    print_str(sw_version);
    uart_redirect(0);
}
#endif

#endif
