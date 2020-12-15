//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include "common.h"     /* Always include common.h at the first place of user-defined herder files */

#if MAC_TO_MEDIA
#include "vtss_luton26_reg.h"
#include "h2pcs1g.h"
#include "main.h"
#include "timer.h"
#include "misc2.h"
#include "h2io.h"
#include "h2sdcfg.h"
#include "print.h"
#include "phymap.h"
#include "phydrv.h"

/*****************************************************************************
 *
 *
 * Defines
 *
 *
 *
 ****************************************************************************/

/**
 * Test whether a bitfield is set in value.
 */
#define BF(__field__, __value__) (((__field__ & __value__) == __field__) ? 1 : 0)

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
void h2_pcs1g_clause_37_control_set(const uchar port_no)
/* ------------------------------------------------------------------------ --
 * Purpose     : Set 1000Base-X Fiber Auto-negotiation (Clause 37)
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{

    ulong tgt = VTSS_TO_DEV(port_no);
    uchar mac_if = phy_map_miim_no(port_no);

    /* Set aneg capabilities */
    if(MAC_IF_SGMII == mac_if) {
        H2_WRITE_MASKED(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(tgt), 0x00010003, 0xffff0003);
        return;
    }
	H2_WRITE_MASKED(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(tgt), 0x00a00003, 0xffff0003);
}

/* ************************************************************************ */
uchar h2_pcs1g_clause_37_status_get(const uchar port_no)
/* ------------------------------------------------------------------------ --
 * Purpose     : Get 1000Base-X Fiber Auto-negotiation status (Clause 37)
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    ulong value;
    uchar link;
    uchar complete, in_progress, ii;
    uchar lm;
    ulong tgt       = VTSS_TO_DEV(port_no);
    uchar mac_if    = phy_map_miim_no(port_no);
    bool  synced_status;
    /* Get the link state 'down' sticky bit  */
    H2_READ(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY(tgt), value);

    lm = LINK_MODE_DOWN;

    link = BF(VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY_LINK_DOWN_STICKY, value) ? 0 : 1;
	H2_READ(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS(tgt), value);

    /* Get the link state 'down' sticky bit  */
    if (link) {
        H2_WRITE(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY(tgt),
                 VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY_LINK_DOWN_STICKY |
                 VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_STICKY_OUT_OF_SYNC_STICKY);
    } else {
        H2_READ(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS(tgt), value);

        link = BF(VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS_LINK_STATUS, value) &&
               BF(VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS_SYNC_STATUS, value);
    }

    synced_status = BF(VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_LINK_STATUS_SYNC_STATUS, value);


    /* Get PCS ANEG status register */
    H2_READ(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS(tgt), value);

    for (ii = 0; ii < 3; ii++) {
        /* Get 'Aneg in_progress'   */
        in_progress = BF(VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS_PR, value);
        if (in_progress) {
            break;
        }    
		delay_1(5); 
    }


    /* Get 'Aneg complete'   */
    complete = BF(VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS_ANEG_COMPLETE, value);

    /* Workaround 1: for a Serdes issue, when aneg completes with FDX capability=0 */
    /* Workaround 2: for a Serdes issue, when aneg UN-completes but synced status is up */

    if (MAC_IF_SERDES == mac_if) {
        if (((complete) && (((value >> 21) & 0x1) == 0)) ||
			(synced_status && (!complete))) {

                H2_WRITE_MASKED(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_CFG(tgt),
                                0,
                                VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_CFG_PCS_ENA);
                H2_WRITE(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_CFG(tgt),
                         VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_CFG_PCS_ENA);
                /* Restart Aneg */
                h2_pcs1g_clause_37_control_set(port_no);

                VTSS_MSLEEP(50);

                H2_READ(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS(tgt), value);
                complete = BF(VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS_ANEG_COMPLETE, value);
        }

    }

    /* Return partner advertisement ability */
    value = VTSS_X_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_STATUS_LP_ADV_ABILITY(value);

    if (MAC_IF_SGMII == mac_if) {
        uchar sgmii_link = BF((1<<15), value);

        lm = ((value >> 10) & 3);
        if (VTSS_BOOL(value & (1<<12))) {
            lm |= LINK_MODE_FDX_MASK;
        }
        if (VTSS_BOOL(value & (1<<7))) {
            lm |= LINK_MODE_PAUSE_MASK;
        }

        if (link) {
            link = sgmii_link;
        }

        if (!link) {
            lm = LINK_MODE_DOWN;
        }
    } else if (link && complete) {
        lm = LINK_MODE_FDX_1000;
        if (VTSS_BOOL(value & (1<<7))) {
            lm |= LINK_MODE_PAUSE_MASK;
        }
    }

    return lm;
}

/* ************************************************************************ */
uchar h2_pcs1g_100fx_status_get(const uchar port_no)
/* ------------------------------------------------------------------------ --
 * Purpose     : Get the 100FX (fiber) port status (no autonegotiation)
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    ulong value;
    ulong tgt = VTSS_TO_DEV(port_no);
    uchar link_mode = 0;

    /* Get the PCS status  */
    H2_READ(VTSS_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS(tgt), value);

    if (BF(VTSS_F_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_SYNC_LOST_STICKY, value) ||
        BF(VTSS_F_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_SSD_ERROR_STICKY, value) ||
        BF(VTSS_F_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_FEF_FOUND_STICKY, value) ||
        BF(VTSS_F_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_PCS_ERROR_STICKY, value))
    {
        /* The link has been down. Clear the sticky bit */
        H2_WRITE(VTSS_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS(tgt), 0xFFFF);
        H2_READ(VTSS_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS(tgt), value);
    }

    if (BF(VTSS_F_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_SYNC_STATUS, value) &&
        !BF(VTSS_F_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_SYNC_LOST_STICKY, value) &&
        !BF(VTSS_F_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_SSD_ERROR_STICKY, value) &&
        !BF(VTSS_F_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_FEF_FOUND_STICKY, value) &&
        !BF(VTSS_F_DEV_PCS_FX100_STATUS_PCS_FX100_STATUS_PCS_ERROR_STICKY, value))
    {
        link_mode = LINK_MODE_FDX_100;
    }
    else
    {
        link_mode = LINK_MODE_DOWN;
    }

    return link_mode;
}

/* ************************************************************************ */
void h2_pcs1g_clock_stop (uchar port_no)
/* ------------------------------------------------------------------------ --
 * Purpose     : Stop PCS clock
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    ulong tgt = VTSS_TO_DEV(port_no);
    H2_WRITE_MASKED(VTSS_DEV_PORT_MODE_CLOCK_CFG(tgt), 0x00000038, 0x0000003b);
}

/* ************************************************************************ */
void h2_pcs1g_setup (uchar port_no, uchar mode)
/* ------------------------------------------------------------------------ --
 * Purpose     : Enable psc1g serdes, sgmii or 100fx
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    ulong tgt = VTSS_TO_DEV(port_no);

    switch(mode) {
    case MAC_IF_SERDES:
        H2_WRITE_MASKED(VTSS_DEV_PORT_MODE_CLOCK_CFG(tgt), 0x00000001, 0x0000003b); // Restart clock
        //H2_WRITE_MASKED(VTSS_DEV_MAC_CFG_STATUS_MAC_MODE_CFG(tgt), 0x00000011, 0x00000011); //giga & fdx
        H2_WRITE_MASKED(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_MODE_CFG(tgt), 0x00000011, 0x00000011); //giga & fdx
        H2_WRITE(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_CFG(tgt), 0x00000001);
        H2_WRITE(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_MODE_CFG(tgt), 0x00000000);
        H2_WRITE(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_SD_CFG(tgt), 0x00000000);
        H2_WRITE_MASKED(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(tgt), 0, 0x00000100);
        H2_WRITE_MASKED(VTSS_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG(tgt), 0x00000000, 0x00000001); // Disable 100FX PCS
        return;
    case MAC_IF_SGMII:
        H2_WRITE_MASKED(VTSS_DEV_PORT_MODE_CLOCK_CFG(tgt), 0x00000001, 0x0000003b); // Restart clock
        /* --left to setup depends on link speed mode
        H2_WRITE_MASKED(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_MODE_CFG(tgt), 0x00000011, 0x00000011); //giga & fdx
        */
        H2_WRITE(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_CFG(tgt), 0x00000001);
        H2_WRITE(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_MODE_CFG(tgt), 0x00000001);
        H2_WRITE_MASKED(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(tgt), 0x00000100, 0x00000100);
        H2_WRITE_MASKED(VTSS_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG(tgt), 0x00000000, 0x00000001); // Disable 100FX PCS
        return;
    case MAC_IF_100FX:
        H2_WRITE_MASKED(VTSS_DEV_PORT_MODE_CLOCK_CFG(tgt), 0x00000002, 0x0000003b);
        //H2_WRITE_MASKED(VTSS_DEV_MAC_CFG_STATUS_MAC_MODE_CFG(tgt), 0x00000000, 0x00000010);
        H2_WRITE_MASKED(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_MODE_CFG(tgt), 0x00000001, 0x00000011);
        H2_WRITE_MASKED(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(tgt), 0, 0x00000100);
        H2_WRITE_MASKED(VTSS_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG(tgt), 0x00000001, 0x07000001);
        return;
    default:
#if 0
        print_str("error : mode is not supported");
        print_cr_lf();
#endif
        ;
    }
}
#endif








