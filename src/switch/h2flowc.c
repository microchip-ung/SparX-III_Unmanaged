//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include "common.h"     /* Always include common.h at the first place of user-defined herder files */
#include "phytsk.h"
#include "vtss_luton26_reg.h"
#include "h2io.h"
#include "main.h"
#include "hwport.h"
#include "misc2.h"

/*****************************************************************************
 *
 *
 * Public data
 *
 *
 *
 ****************************************************************************/



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




/* ************************************************************************ */
void h2_setup_flow_control (uchar port_no, uchar link_mode)
/* ------------------------------------------------------------------------ --
 * Purpose     : Setup flow control according to configuration and link
 *               partner's advertisement.
 * Remarks     : Please see main.h for a description of link_mode.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    mac_addr_t mac_addr;
    ulong pause_start, pause_stop, rsrv_raw, rsrv_total, atop_wm;
    ulong tgt;
    uchar port, lm;
    uchar local, remote, fc;
#if (!VTSS_ATOM12_A) && (!VTSS_ATOM12_B)
    /* check pause flag */
    uchar mask = LINK_MODE_PAUSE_MASK;
#else
    /* check BOTH pause flag and full duplex flag */
    /* Note: in some chipset revisions, half duplex backpressure do not work
       on SERDES port. */
    uchar mask = LINK_MODE_FDX_AND_PAUSE_MASK;
#endif


    local  = TRUE; /* Local has always advertised support of pause frames */

    remote      = ((link_mode & mask) == mask);

    fc          = (local && remote);

    rsrv_total  = 0;
    pause_start = 0x7ff;
    pause_stop  = 0x7ff;

#if JUMBO
    /* FC disabled and jumbo */
    rsrv_raw  = 250;      /* 12000 / 48 */

    if (fc) {
        /* FC enabled and jumbo */
        pause_start = 221;    /* 7 x 1518 / 48 */
        pause_stop  = 158;    /* 5 x 1518 / 48 */
        rsrv_raw    = 284;    /* 9 x 1518 / 48 */
    }
#else
    /* FC disabled and no jumbo */
    rsrv_raw = 0;

    if (fc) {
        /* FC enabled and no jumbo */
        pause_start = 190;    /* 6 x 1518 / 48 */
        pause_stop  = 127;    /* 4 x 1518 / 48 */
        rsrv_raw    = 253;    /* 8 x 1518 / 48 */
    }
#endif

    /* Calculate the total reserved space for all ports*/
    for (port = MIN_PORT; port < MAX_PORT; port++) {
        lm     = phy_get_link_mode_raw(port);
        remote = ((lm & mask) == mask);
#if JUMBO
        if (local && remote) {
            rsrv_total +=  13662; /* 9*1518 */
        } else {
            rsrv_total +=  12000;
        }
#else
        if (local && remote) {
            rsrv_total +=  12144; /* 8*1518 */
        }
#endif
    }

    /* Set Pause WM hysteresis*/
    H2_WRITE(VTSS_SYS_PAUSE_CFG_PAUSE_CFG(port_no),
             VTSS_F_SYS_PAUSE_CFG_PAUSE_CFG_PAUSE_START(pause_start) |
             VTSS_F_SYS_PAUSE_CFG_PAUSE_CFG_PAUSE_STOP(pause_stop) |
             (fc ? VTSS_F_SYS_PAUSE_CFG_PAUSE_CFG_PAUSE_ENA : 0));

    atop_wm = (512000 - rsrv_total)/48;
    if (atop_wm >= 1024UL) {
        atop_wm = 1024UL + atop_wm/16;
    }

    /*  When 'port ATOP' and 'common ATOP_TOT' are exceeded, tail dropping is activated on a port */
    H2_WRITE(VTSS_SYS_PAUSE_CFG_ATOP_TOT_CFG, atop_wm);
    H2_WRITE(VTSS_SYS_PAUSE_CFG_ATOP(port_no), rsrv_raw);


    /* Set SMAC of Pause frame */
    get_mac_addr(port_no, mac_addr);
    tgt = VTSS_TO_DEV(port_no);
    H2_WRITE(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_FC_MAC_HIGH_CFG(tgt),(mac_addr[0]<<16) | (mac_addr[1]<<8) | mac_addr[2]);
    H2_WRITE(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_FC_MAC_LOW_CFG(tgt), (mac_addr[3]<<16) | (mac_addr[4]<<8) | mac_addr[5]);

    /* Enable/disable FC incl. pause value and zero pause */
    H2_WRITE(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_FC_CFG(tgt),
             VTSS_F_DEV_MAC_CFG_STATUS_MAC_FC_CFG_ZERO_PAUSE_ENA |
             (fc ? VTSS_F_DEV_MAC_CFG_STATUS_MAC_FC_CFG_TX_FC_ENA : 0) |
             (fc ? VTSS_F_DEV_MAC_CFG_STATUS_MAC_FC_CFG_RX_FC_ENA  : 0) |
             VTSS_F_DEV_MAC_CFG_STATUS_MAC_FC_CFG_PAUSE_VAL_CFG(0xff) |
             VTSS_F_DEV_MAC_CFG_STATUS_MAC_FC_CFG_FC_LATENCY_CFG(63));/* changed from 7 to 63 : disable flow control latency */
}



