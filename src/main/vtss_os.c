//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include "common.h"     /* Always include common.h at the first place of user-defined herder files */
#include "vtss_common_os.h"
#include "sysutil.h"

#include "main.h"
#include "phytsk.h"
#include "hwport.h"
#include "h2.h"
#include "print.h"
#include "timer.h"
#include "h2txrx.h"
#include "uartdrv.h"
#if TRANSIT_LLDP
#include "lldp.h"
#endif /* TRANSIT_LLDP */
#if TRANSIT_EEE_LLDP
#include "eee_api.h"
#endif /* TRANSIT_EEE_LLDP */


vtss_common_trlevel_t vtss_os_trace_level = VTSS_COMMON_TRLVL_DEBUG;

#ifndef NDEBUG
static void print_unsigned(unsigned v)
{
    uchar buf[6];
    uchar i;

    if (v == 0) {
        print_ch('0');
        return;
    }
    i = 0;
    while (v) {
        buf[i++] = ('0' + (v % 10));
        v /= 10;
    }
    while (i--)
        print_ch(buf[i]);
}

void vtss_vprintf(const char *fmt, va_list ap)
{
    char *cp, *xp, ch;
    int fieldw = -1;
    bit hflg = 0, lflg = 0, nfw = 0;

    for (cp = fmt; *cp; cp++) {
        ch = *cp;
        if (ch == '%') {
            lflg = hflg = 0;
morearg:
            ch = *++cp;
            switch (ch) {
            case 'l' :
                lflg = 1;
                goto morearg;
            case 'h' :
                hflg = 1;
                goto morearg;
            case '-' :
                nfw = 1;
                goto morearg;
            case '0' :
            case '1' :
            case '2' :
            case '3' :
            case '4' :
            case '5' :
            case '6' :
            case '7' :
            case '8' :
            case '9' :
                if (fieldw < 0)
                    fieldw = 0;
                fieldw = fieldw * 10 + (ch - '0');
                goto morearg;
            case 's' :
                xp = va_arg(ap, char *);
                print_str(xp);
                break;
            case 'u' :
                if (lflg)
                    print_dec(va_arg(ap, ulong));
                else
                    print_unsigned(hflg ? va_arg(ap, uchar) : va_arg(ap, unsigned));
                break;
            case 'd' :
                if (lflg) {
                    long dv = va_arg(ap, long);
                    if (dv < 0) {
                        print_ch('-');
                        dv = -dv;
                    }
                    print_dec(dv);
                } else {
                    int dv = hflg ? va_arg(ap, char) : va_arg(ap, int);
                    if (dv < 0) {
                        print_ch('-');
                        dv = -dv;
                    }
                    print_unsigned(dv);
                }
                break;
            case 'x' :
                if (lflg)
                    print_hex_dw(va_arg(ap, ulong));
                else if (hflg)
                    print_hex_b(va_arg(ap, uchar));
                else
                    print_hex_w(va_arg(ap, ushort));
                break;
            case 'c' :
                print_ch(va_arg(ap, char));
                break;
            case '\0' :
                cp--;
                /*FALLTHROUGH*/
            case '%' :
                print_ch('%');
                break;
            default :
                print_ch('%');
                print_ch(ch);
                break;
            }
        } else {
            if (ch == '\r' && cp[1] != '\n')
                print_ch(ch);
            else if (ch == '\n')
                print_cr_lf();
            else
                print_ch(ch);
        }
    }
}

void vtss_printf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vtss_vprintf(fmt, ap);
    va_end(ap);
}
#endif

#ifndef VTSS_COMMON_NDEBUG
VTSS_COMMON_DATA_ATTRIB char _common_file[80] = __FILE__;
ushort _common_line = 0;
ushort _common_retaddr = 0;
vtss_common_trlevel_t _common_lvl = VTSS_COMMON_TRLVL_NOISE;

static const char *basenm(const char *fname)
{
    const char *cp;

    cp = strrchr(fname, '\\');
    if (cp == NULL)
        cp = strrchr(fname, '/');
    return (cp == NULL) ? fname : cp + 1;
}

void vtss_os_trace(const char *fmt, ...)
{
    va_list ap;

    if (_common_lvl > vtss_os_trace_level)
        return;

    vtss_printf("%s(%u) %c: ", basenm(_common_file),
                (unsigned)_common_line,
                "0EWDN"[_common_lvl]);
    va_start(ap, fmt);
    vtss_vprintf(fmt, ap);
    va_end(ap);
    if (fmt[strlen(fmt) - 1] != '\n')
        print_cr_lf();
}

void vtss_os_assert(unsigned line_number)
{
    uchar ch;

    vtss_printf("\nAssert file %s line %u called from 0x%x failed.\nType c - continue, r - reboot h - hang: ",
                (const char *)basenm(_common_file), (unsigned)line_number,
                (unsigned)_common_retaddr);
    do {
        while (!uart_byte_ready())
            /* Wait for it */;
        ch = uart_get_byte();
        switch (ch) {
        case 'c' :
        case 'C' :
            vtss_printf("\n Continuing execution\n");
            return;
        case 'r' :
        case 'R' :
            sysutil_reboot();
            break;
        }
    } while (ch != 'h' && ch != 'H');
    vtss_printf("\n hanging.\n");
    sysutil_set_fatal_error(H2_GENERAL_FAILURE);
    sysutil_hang();
}

void vtss_common_dump_frame(const vtss_common_octet_t VTSS_COMMON_PTR_ATTRIB *frame, vtss_common_framelen_t len)
{
#define MBUF    ((const vtss_common_macheader_t VTSS_COMMON_PTR_ATTRIB *)frame)
    vtss_common_framelen_t i;

    vtss_printf("Frame len %u dst %s",
                (unsigned)len, vtss_common_str_macaddr(&MBUF->dst_mac));
    vtss_printf(" src %s type 0x%x",
                vtss_common_str_macaddr(&MBUF->src_mac), (unsigned)MBUF->eth_type);
    for (i = 0; i < len; i++) {
        if ((i & 0xF) == 0) {
            print_cr_lf();
            print_dec_16_right(i, 5);
            print_ch(':');
            print_spaces(1);
        }
        print_hex_b(frame[i]);
        print_spaces(1);
    }
    print_cr_lf();
#undef MBUF
}
#endif /* !VTSS_COMMON_NDEBUG */

#ifndef NDEBUG
const char *vtss_common_str_macaddr(const vtss_common_macaddr_t VTSS_COMMON_PTR_ATTRIB *mac)
{
    static char VTSS_COMMON_DATA_ATTRIB buf[24];

    uart_redirect(buf);
    print_mac_addr(mac->macaddr);
    uart_redirect(NULL);
    return buf;
}
#endif

/**
 * vtss_os_get_linkspeed - Return link speed (in Mbps) for a given physical port
 * If link state is "down" the speed is returned as 0.
 * @portno: Port number (1 - VTSS_LACP_MAX_PORTS)
 *
 */
vtss_common_linkspeed_t vtss_os_get_linkspeed(vtss_common_port_t portno)
{
    static const ushort speed[] = { 10, 100, 1000, 2500 };
    uchar lm;

    VTSS_COMMON_ASSERT(portno > 0 && portno <= NO_OF_PORTS);
    lm = phy_get_link_mode_raw(PROTO2OSINT(portno));
    if (lm == LINK_MODE_DOWN)
        return 0;
    return (vtss_common_linkspeed_t)speed[lm & LINK_MODE_SPEED_MASK];
}

#if TRANSIT_EEE_LLDP
/**
 * vtss_os_get_link_pwr_mngr_mode - Return layer 2 power management mode for a
 * given physical port.
 * If phy register shows L2 capability,
 * returned as VTSS_COMMON_LINK_PWR_MNGR_ENABLE.
 * @portno: Port number (1 - VTSS_LACP_MAX_PORTS)
 *
 */
vtss_common_pwr_mngr_t vtss_os_get_link_pwr_mngr_mode(vtss_common_port_t portno)
{
    uchar lm = VTSS_COMMON_LINK_PWR_MNGR_DISABLE;

    VTSS_COMMON_ASSERT(portno > 0 && portno <= NO_OF_PORTS);
    lm = phy_get_link_mode_raw(PROTO2OSINT(portno));
    if (lm != LINK_MODE_DOWN) {
        return ((lm & LINK_MODE_POWER_MASK) >> 6);
    }
    return VTSS_COMMON_LINK_PWR_MNGR_DISABLE;
}
#endif

/**
 * vtss_os_get_systemmac - Return MAC address associated with the system.
 * @system_macaddr: Return the value.
 *
 */
#if TRANSIT_LLDP
void vtss_os_get_systemmac(vtss_common_macaddr_t *system_macaddr)
{
    get_mac_addr(SYSTEM_MAC_ADDR, system_macaddr->macaddr);
}
#endif


/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
