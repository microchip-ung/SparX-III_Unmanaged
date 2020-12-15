//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT


#ifndef __PRINT_H__
#define __PRINT_H__

#include "l26_txtdef.h"

void print_str (const char *s);
void println_str (const char *s);
void print_hex_b (uchar value);
void print_hex_w (ushort value);
void print_hex_dw (ulong value);
void print_dec (ulong value);
void print_hex_prefix (void);
void print_cr_lf (void);
void print_ch (uchar ch);
void print_spaces (uchar count);
void print_dec_right (ulong value);
void print_dec_nright (ulong value, uchar fieldwidth);
void print_dec_16_right (ushort value, uchar width);
void print_ip_addr (const uchar xdata *ip_addr);
void print_line (uchar width);
void print_mac_addr (const uchar *mac_addr);
void print_port_mac_addr (uchar port_no);
void print_dec_8_right_2 (uchar value);

#if VTSS_FEATURE_FTIME
void print_uptime(void);

/**
 * Print the delta time of consecutive checks.
 *
 * @param check Supply 0 to record current time and does not print any thing.
 *              Supply 1-9 to print the delta time from previous check.
 */
void print_delta(uchar check);
#endif

#endif /* __PRINT_H__*/

