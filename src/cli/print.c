//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include <string.h>

#include "common.h"     /* Always include common.h at the first place of user-defined herder files */
#include "print.h"
#include "uartdrv.h"
#include "misc1.h"
#include "hwport.h"

#if VTSS_FEATURE_FTIME
#include "timer.h"
#endif

#ifndef NO_DEBUG_IF
#include "l26_txtdef.h"

/*****************************************************************************
 *
 *
 * Defines
 *
 *
 *
 ****************************************************************************/

#define LEFT  0
#define RIGHT 1

#if VTSS_FEATURE_FTIME
#define ONE_DAY     ( 24 * 60 * 60 )
#define TEN_HOURS   ( 10 * 60 * 60 )
#define ONE_HOUR    (      60 * 60 )
#define TEN_MINUTES (      10 * 60 )
#define ONE_MINUTE  (           60 )
#define TEN_SECONDS (           10 )
#define HUNDRED_MS  (          100 )
#define TEN_MS      (           10 )
#endif

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

static void print_dec_32 (ulong value, uchar adjust, uchar fieldwidth);

/*****************************************************************************
 *
 *
 * Local data
 *
 *
 ****************************************************************************/

#if VTSS_FEATURE_FTIME
struct timeb    g_delta_time;
#endif

/*****************************************************************************
 *
 *
 * Public Functions
 *
 *
 ****************************************************************************/

/* ************************************************************************ */
void print_str (const char *s)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print a 0-terminated string.
 * Remarks     : s points to string.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    while (*s != 0) {
        uart_put_byte(*s++);
    }
}

/* ************************************************************************ */
void println_str (const char *s)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print a 0-terminated string.
 * Remarks     : s points to string.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    print_str(s);
    print_cr_lf();
}

/* ************************************************************************ */
void print_hex_b (uchar value)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print value of a byte as 2 hex nibbles.
 * Remarks     : value holds byte value to print.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uart_put_byte(hex_to_ascii_nib(value >> 4));
    uart_put_byte(hex_to_ascii_nib(value & 0x0f));
}

/* ************************************************************************ */
void print_hex_w (ushort value)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print value of a word (16-bit integer) as 4 hex nibbles.
 * Remarks     : value holds word value to print.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    print_hex_b(value >> 8);
    print_hex_b(value & 0xff);
}

/* ************************************************************************ */
void print_hex_dw (ulong value)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print value of a dword (32-bit integer) as 8 hex nibbles.
 * Remarks     : value holds dword value to print.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    print_hex_w(value >> 16);
    print_hex_w(value & 0xffff);
}

/* ************************************************************************ */
void print_dec (ulong value)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print value of a dword (32-bit integer) as a decimal number.
 * Remarks     : value holds dword value to print.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uchar buf [10];
    uchar no_of_digits;

    /* Determine number of significant digits and isolate digits */
    no_of_digits = 0;
    while (value > 0) {
        buf[no_of_digits] = value % 10;
        value = value / 10;
        no_of_digits++;
    }

    /* Print each significant digit */
    if (no_of_digits == 0) {
        uart_put_byte('0');
    } else {
        no_of_digits--;
        while (no_of_digits != 0xff) {
            uart_put_byte(buf[no_of_digits] + '0');
            no_of_digits--;
        }
    }
}

#ifndef UNMANAGED_REDUCED_DEBUG_IF
/* ************************************************************************ */
void print_dec_8_right_2 (uchar value)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print value of a byte as a 2-digit decimal number.
 * Remarks     : value holds byte value to print.
 * Restrictions: Value may not exceed 99.
 * See also    :
 * Example     :
 ****************************************************************************/
{
    if (value > 9) {
        uart_put_byte(((value / 10) % 10) + '0');
    } else {
        uart_put_byte(' ');
    }
    uart_put_byte((value % 10) + '0');
}
#endif

#if UNMANAGED_PORT_STATISTICS_IF
/* ************************************************************************ */
void print_dec_right (ulong value)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print value of a dword (32-bit integer) as a decimal number
 *               right adjusted in a 10-char field.
 * Remarks     : value holds dword value to print.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    print_dec_32(value, RIGHT, 10);
}
#endif


#if UNMANAGED_EEE_DEBUG_IF
/* ************************************************************************ */
void print_dec_nright (ulong value, uchar fieldwidth)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print value of a dword (32-bit integer) as a decimal number
 *               right adjusted in a <fieldwidth>-char field.
 * Remarks     : value holds dword value to print.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    print_dec_32(value, RIGHT, fieldwidth);
}
#endif

#if TRANSIT_VERIPHY || TRANSIT_FAN_CONTROL
#if UNMANAGED_TRANSIT_VERIPHY_DEBUG_IF || UNMANAGED_FAN_DEBUG_IF
void print_dec_16_right (ushort value, uchar width)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print value of a word (16-bit integer) as a decimal number
 *               right adjusted in a field with width specified by parameter.
 * Remarks     : value holds word value to print. width specifies width of
 *               field (range 1-5).
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uchar str [5];
    uchar j;

    if (width > 5) {
        width = 5;
    } else if (width == 0) {
        width = 1;
    }

    str[0] = '0';
    memset(&str[1], ' ', 4);

    j = 0;
    while (value != 0) {
        str[j] = (value % 10) + '0';
        value /= 10;
        j++;
    }

    while (width-- > 0) {
        uart_put_byte(str[width]);
    }
}
#endif
#endif

/* ************************************************************************ */
void print_hex_prefix (void)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print a hex prefix, i.e. print "0x".
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uart_put_byte('0');
    uart_put_byte('x');
}

/* ************************************************************************ */
void print_cr_lf (void)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print a <CR> (0x0d) and a <LF> (0x0a).
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uart_put_byte('\r');
    uart_put_byte('\n');
}

/* ************************************************************************ */
void print_ch (uchar ch)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print a single char.
 * Remarks     : ch holds char to print in ASCII format.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uart_put_byte(ch);
}

#ifndef UNMANAGED_REDUCED_DEBUG_IF
/* ************************************************************************ */
void print_spaces (uchar count)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print a specified number of spaces.
 * Remarks     : count holds number of spaces to print.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    while (count-- > 0) {
        uart_put_byte(' ');
    }
}
#endif

#if UNMANAGED_LLDP_DEBUG_IF
/* ************************************************************************ */
void print_ip_addr (const uchar xdata *ip_addr)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print ip address in format xxx.xxx.xxx.xxx.
 * Remarks     : ip_addr points to a 4-byte array holding ip address.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uchar j;

    for (j = 0; j < 4; j++) {
        print_dec(*ip_addr++);
        if (j < 3) {
            print_ch('.');
        }
    }
}
#endif

/* ************************************************************************ */
void print_mac_addr (const uchar *mac_addr)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print mac address in format xx-xx-xx-xx-xx-xx.
 * Remarks     : mac_addr points to a 6-byte array holding mac address.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uchar j;

    for (j = 0; j < 6; j++) {
        print_hex_b(*mac_addr++);
        if (j < 5) {
            print_ch('-');
        }
    }
}

#ifndef UNMANAGED_REDUCED_DEBUG_IF
/* ************************************************************************ */
void print_port_mac_addr (uchar port_no)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print mac address for the specified port in format
 *               xx-xx-xx-xx-xx-xx.
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    mac_addr_t mac_addr;
    get_mac_addr(port_no, mac_addr);
    print_mac_addr(mac_addr);
}
#endif

#if UNMANAGED_PORT_STATISTICS_IF
/* ************************************************************************ */
void print_line (uchar width)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print a line ('-') with specified width.
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    while (width > 0) {
        uart_put_byte('-');
        width--;
    }
}
#endif

#if UNMANAGED_EEE_DEBUG_IF || UNMANAGED_PORT_STATISTICS_IF
/* ************************************************************************ */
static void print_dec_32 (ulong value, uchar adjust, uchar fieldwidth)
/* ------------------------------------------------------------------------ --
 * Purpose     : Print a 32-bit decimal value either left adjusted with no
 *               trailing spaces or right adjusted in a 10-char field with
 *               spaces in front.
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uchar buf [10];
    uchar no_of_digits;

    while(fieldwidth > 10) {
        uart_put_byte(' ');
        fieldwidth--;
    };

    memset(buf, ' ',sizeof(buf));

    /* Determine number of significant digits and isolate digits */
    no_of_digits = 0;
    while (value > 0) {
        buf[no_of_digits] = (value % 10) + '0';
        value = value / 10;
        no_of_digits++;
    }

    if (no_of_digits == 0) {
        buf[0] = '0';
        no_of_digits = 1;
    }

    if (adjust == RIGHT) {
        no_of_digits = fieldwidth;
    }

    while (no_of_digits-- > 0) {
        uart_put_byte(buf[no_of_digits]);
    }
}
#endif

#endif

#if VTSS_FEATURE_FTIME
void print_uptime(void)
{
    uchar           buf[13]; /* Buffer for: 12:34:56.789 */
    uchar           i = 0;
    struct timeb    t;

    ftime(&t);

    /* support less than 1 days */
    t.time = t.time % ONE_DAY;

    /* [0-2]_:__:__.___ */
    buf[i]    = '0';
    buf[i++] += t.time / TEN_HOURS;
    t.time    = t.time % TEN_HOURS;

    /* _[0-9]:__:__.___ */
    buf[i]    = '0';
    buf[i++] += t.time / ONE_HOUR;
    t.time    = t.time % ONE_HOUR;

    buf[i++]  = ':';

    /* __:[0-5]_:__.___ */
    buf[i]    = '0';
    buf[i++] += t.time / TEN_MINUTES;
    t.time    = t.time % TEN_MINUTES;

    /* __:_[0-9]:__.___ */
    buf[i]    = '0';
    buf[i++] += t.time / ONE_MINUTE;
    t.time    = t.time % ONE_MINUTE;

    buf[i++]  = ':';

    /* __:__:[0-5]_.___ */
    buf[i]    = '0';
    buf[i++] += t.time / TEN_SECONDS;
    t.time    = t.time % TEN_SECONDS;

    /* __:__:_[0-9].___ */
    buf[i]    = '0';
    buf[i++] += t.time;

    buf[i++]  = '.';

    /* __:__:__.[0-9]__ */
    buf[i]     = '0';
    buf[i++]  += t.millitm / HUNDRED_MS;
    t.millitm  = t.millitm % HUNDRED_MS;

    /* __:__:__._[0-9]_ */
    buf[i]     = '0';
    buf[i++]  += t.millitm / TEN_MS;
    t.millitm  = t.millitm % TEN_MS;

    /* __:__:__.__[0-9] */
    buf[i]     = '0';
    buf[i++]  += t.millitm;

    buf[i]    = 0; /* NULL-terminator */

    print_str(buf);
}

void print_delta(uchar check)
{
    if (check == 0)
        ftime(&g_delta_time);
    else {
        struct timeb    t;
        u16             delta;

        ftime(&t);

        delta  = (t.time - g_delta_time.time) * 1000;

        if (t.millitm >= g_delta_time.millitm)
            delta += t.millitm - g_delta_time.millitm;
        else
            delta -= g_delta_time.millitm - t.millitm;

        g_delta_time = t;

        if (delta) {
            print_str("check");
            print_ch ('0' + check);
            print_str(": ");
            print_dec(delta);
            print_cr_lf();
        }
    }
}
#endif /* VTSS_FEATURE_FTIME */
