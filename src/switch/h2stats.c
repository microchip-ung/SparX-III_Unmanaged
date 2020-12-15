//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include "common.h"     /* Always include common.h at the first place of user-defined herder files */
#include "vtss_luton26_reg.h"
#include "h2io.h"
#include "h2stats.h"
#include "txt.h"
#include "print.h"

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

/*****************************************************************************
 *
 *
 * Local data
 *
 *
 *
 ****************************************************************************/


static const ushort tx_counters[6] = {  CNT_TX_64,
                                        CNT_TX_65_TO_127,
                                        CNT_TX_128_TO_255,
                                        CNT_TX_256_TO_511,
                                        CNT_TX_512_TO_1023,
                                        CNT_TX_1024_TO_1526 };


static const ushort rx_counters[6] = {  CNT_RX_64,
                                        CNT_RX_65_TO_127,
                                        CNT_RX_128_TO_255,
                                        CNT_RX_256_TO_511,
                                        CNT_RX_512_TO_1023,
                                        CNT_RX_1024_TO_1526 };

static ulong h2_stats_counter_get_private (
    uchar               port_no,
    port_statistics_t   counter_id
)
{
    ulong               temp, tgt, port_offset;

    /* Reserved */
    if (counter_id >= 0x7000)
        return 0;

    if (counter_id >= 0xC00)
        port_offset = 18UL * ((ulong) port_no);
    else if (counter_id >= 0x800)
        port_offset = 31UL * ((ulong) port_no);
    else
        port_offset = 43UL * ((ulong) port_no);

    tgt = VTSS_SYS_STAT_CNT(counter_id + port_offset);

    H2_READ(tgt, temp);

    return temp;
}

ulong h2_stats_counter_get (
    uchar               port_no,
    port_statistics_t   counter_id
)
{
    ulong               cnt;
    uchar               i;
    ushort              *p;

    if (counter_id == CNT_TX_PKTS)
        p = &tx_counters[0];
    else
    if (counter_id == CNT_RX_PKTS)
        p = &rx_counters[0];
    else {
        return h2_stats_counter_get_private( port_no, counter_id);
    }

    /*
     * For Luton26 there is no counter that counts all frames,
     * so we have to calculate it ourselves.
     */

    for (i = 0, cnt = 0; i < 6; i++)
        cnt += h2_stats_counter_get_private( port_no, p[i] );

    return cnt;
}

#if UNMANAGED_PORT_STATISTICS_IF
/* ************************************************************************ */
void print_port_statistics (uchar port_no)
/* ------------------------------------------------------------------------ --
 * Purpose     :
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
#define NO_OF_LINES 20
#define SIZE_COUNTERS_START   7
#define ERROR_COUNTERS_START 13

    code std_txt_t prefix_txt [2] = {TXT_NO_RX_PREFIX , TXT_NO_TX_PREFIX};
    code struct {
        std_txt_t txt_no [2];
        port_statistics_t counter_id [2];
    }  display_tab_1 [NO_OF_LINES] = {
        TXT_NO_PACKETS,    TXT_NO_PACKETS,    CNT_RX_PKTS,          CNT_TX_PKTS,
        TXT_NO_OCTETS,     TXT_NO_OCTETS,     CNT_RX_OCTETS,        CNT_TX_OCTETS,
        TXT_NO_BC_PACKETS, TXT_NO_BC_PACKETS, CNT_RX_BCAST_PKTS,    CNT_TX_BCAST_PKTS,
        TXT_NO_MC_PACKETS, TXT_NO_MC_PACKETS, CNT_RX_MCAST_PKTS,    CNT_RX_MCAST_PKTS,
        TXT_NO_PAUSE,      TXT_NO_PAUSE,      CNT_RX_PAUSE,         CNT_TX_PAUSE,
        TXT_NO_ERR_PACKETS,TXT_NO_ERR_PACKETS,CNT_RX_CRC_ALIGN_ERRS,CNT_TX_DROP,
        TXT_NO_MAC_CTRL,   TXT_NO_DASH,       CNT_RX_CTRL,          COUNTER_NONE,

        TXT_NO_64_BYTES,   TXT_NO_64_BYTES,   CNT_RX_64,            CNT_TX_64,
        TXT_NO_65_BYTES,   TXT_NO_65_BYTES,   CNT_RX_65_TO_127,     CNT_TX_65_TO_127,
        TXT_NO_128_BYTES,  TXT_NO_128_BYTES,  CNT_RX_128_TO_255,    CNT_TX_128_TO_255,
        TXT_NO_256_BYTES,  TXT_NO_256_BYTES,  CNT_RX_256_TO_511,    CNT_TX_256_TO_511,
        TXT_NO_512_BYTES,  TXT_NO_512_BYTES,  CNT_RX_512_TO_1023,   CNT_TX_512_TO_1023,
        TXT_NO_1024_BYTES, TXT_NO_1024_BYTES, CNT_RX_1024_TO_1526,  CNT_TX_1024_TO_1526,
        TXT_NO_CRC_ALIGN,  TXT_NO_COLLISIONS, CNT_RX_CRC_ALIGN_ERRS,CNT_TX_COLLISIONS,
        TXT_NO_UNDERSIZE,  TXT_NO_DROPS,      CNT_RX_UNDERSIZE_PKTS,CNT_TX_DROP,
        TXT_NO_OVERSIZE,   TXT_NO_OVERFLOW,   CNT_RX_OVERSIZE_PKTS, CNT_DROP_TAIL,
        TXT_NO_FRAGMENTS,  TXT_NO_AGED,       CNT_RX_FRAGMENTS,     CNT_TX_AGED,
        TXT_NO_JABBERS,    TXT_NO_DASH,       CNT_RX_JABBERS,       COUNTER_NONE,
        TXT_NO_DROPS,      TXT_NO_DASH,       CNT_DROP_LOCAL,       COUNTER_NONE,
        TXT_NO_CAT_DROPS,  TXT_NO_DASH,       CNT_RX_CAT_DROP,      COUNTER_NONE,

    };
    uchar j;
    uchar c;
    uchar display_header;
    uchar indentation;
    std_txt_t rx_txt_no;
    uchar spaces;
    std_txt_t tx_txt_no;
    ulong reg_addr;
    ulong reg_val;

    for (j = 0; j < NO_OF_LINES; j++) {

        /*
        ** Possibly display counter block header
        */
        display_header = FALSE;
        if (j == 0) {
            /* Total counters */
            display_header = TRUE;
            indentation = 10;
            rx_txt_no   = TXT_NO_RX_TOTAL;
            spaces      = 25;
            tx_txt_no   = TXT_NO_TX_TOTAL;
        } else if (j == SIZE_COUNTERS_START) {
            /* Size counters */
            display_header = TRUE;
            indentation = 6;
            rx_txt_no   = TXT_NO_RX_SIZE;
            spaces      = 17;
            tx_txt_no   = TXT_NO_TX_SIZE;
        } else if (j == ERROR_COUNTERS_START) {
            /* Error counters */
            display_header = TRUE;
            indentation = 6;
            rx_txt_no   = TXT_NO_RX_ERROR;
            spaces      = 17;
            tx_txt_no   = TXT_NO_TX_ERROR;
        }

        if (display_header) {
            print_cr_lf();
            print_spaces(indentation);
            print_txt(rx_txt_no);
            print_spaces(spaces);
            print_txt(tx_txt_no);
            print_cr_lf();

            print_spaces(indentation);
            print_line(txt_len(rx_txt_no));
            print_spaces(spaces);
            print_line(txt_len(tx_txt_no));
            print_cr_lf();
        }

        /*
        ** Display 2 columns of counters
        */
        for (c = 0; c < 2; c++) {
            reg_addr = display_tab_1[j].counter_id[c];
            print_txt(prefix_txt[c & 0x01]);
            print_txt_left(display_tab_1[j].txt_no[c], 23);
            if (reg_addr != COUNTER_NONE) {
                reg_val = h2_stats_counter_get(port_no, reg_addr);
                print_dec_right(reg_val);
            } else {
                print_str("         -");
            }
            print_spaces(2);
        }
        print_cr_lf();
    }
}
#endif








