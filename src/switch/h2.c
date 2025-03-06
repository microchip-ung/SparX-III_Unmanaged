//Copyright (c) 2004-2025 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include "common.h"     /* Always include common.h at the first place of user-defined herder files */

#include "vtss_common_os.h"
#include "sysutil.h"
#include "vtss_luton26_reg.h"
#include "h2io.h"
#include "h2.h"
#include "timer.h"
#include "main.h"
#include "phydrv.h"
#include "phytsk.h"
#include "phymap.h"
#include "h2flowc.h"
#include "misc1.h"
#include "misc2.h"
#include "hwport.h"
#include "h2mactab.h"
#include "h2vlan.h"

#define VTSS_COMMON_ASSERT(EXPR) /* Go away */


#ifndef NDEBUG
#include "txt.h"
#endif /* NDEBUG */
#include "h2sdcfg.h"
#include "print.h"
#include "ledtsk.h"
#if MAC_TO_MEDIA
#include "h2pcs1g.h"
#endif

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

enum {
    VTSS_PGID_DEST_MASK_START   =   0,
    VTSS_PGID_AGGR_MASK_START   =  64,
    VTSS_PGID_SOURCE_MASK_START =  80
};

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


/*****************************************************************************
 *
 *
 * Local functions
 *
 *
 *
 ****************************************************************************/

/**
 * Determines ingress forwarding is allowed or not.
 *
 * In unmanaged code base, this function always returns TRUE. In managed code,
 * the return value depends on the result from spanning-tree and 802.1x.
 */
static BOOL _ingr_forwarding (vtss_sport_no_t sport_no)
{

    sport_no = sport_no; /* avoid compiler warning */

    return TRUE;
}


/**
 * Sets up how to access the switch chip.
 */
static void _h2_setup_cpu_if (void) small
{
}


static void _l26_buf_conf_set(void)
{
    ulong port_no, q, dp, i = 0;
    ulong buf_q_rsrv_i, buf_q_rsrv_e, ref_q_rsrv_i, ref_q_rsrv_e, buf_prio_shr_i[8], buf_prio_shr_e[8], ref_prio_shr_i[8], ref_prio_shr_e[8];
    ulong buf_p_rsrv_i, buf_p_rsrv_e, ref_p_rsrv_i, ref_p_rsrv_e, buf_col_shr_i, buf_col_shr_e, ref_col_shr_i, ref_col_shr_e;
    ulong prio_mem_rsrv, prio_ref_rsrv, mem, ref, value;

    /*  SYS::RES_CFG : 1024 watermarks for 512 kB shared buffer, unit is 48 byte */
    /*  Is divided into 4 resource consumptions, ingress and egress memory (BUF) and frame reference (REF) blocks */

    /* BUF_xxx_Ingress starts @ offset 0   */
    /* REF_xxx_Ingress starts @ offset 256 */
    /* BUF_xxx_Egress  starts @ offset 512 */
    /* BUF_xxx_Egress  starts @ offset 768 */
    /* xxx = q_rsrv, prio_shr, p_rsrv, col_shr */

    /* Queue reserved (q_rsrv) : starts @ offset 0 within in each BUF and REF */
    /* Prio shared (prio_shr)  : starts @ offset 216 within in each BUF and REF */
    /* Port reserved (p_rsrv)  : starts @ offset 224 within in each BUF and REF */
    /* Colour shared (col_shr) : starts @ offset 254 within in each BUF and REF */

    /* WM values  */
    buf_q_rsrv_i = 10;     /* 500/48 Guarantees reception of at least one frame to all queues  */
    ref_q_rsrv_i = 8;      /* 4 frames can be pending at each ingress port              */
    buf_q_rsrv_e = 4;      /* 200/48 Guarantees all priorities to non-congested traffic stream */
    ref_q_rsrv_e = 8;      /* 4 frames can be pending to each egress port               */

    prio_mem_rsrv = 7000;
    prio_ref_rsrv = 50;

    /* Subtract the reserved amount from the total amount */
    mem = 512000-(MAX_PORT + 1)*(0+10000+8*500+8*200);
    ref = 5500-(MAX_PORT + 1)*(20+20+8*8+8*8);

#ifndef VTSS_PRIOS
#define VTSS_PRIOS 8
#endif

    for (q = 0; q < VTSS_PRIOS; q++) {
        value = (mem-(7-q)*prio_mem_rsrv)/48;
        if (value >= 1024) {
            value = 1024 + value/16;
        }
        buf_prio_shr_i[q] = value;
        ref_prio_shr_i[q] = ref-(7-q)*prio_ref_rsrv;
        value = (mem-(7-q)*prio_mem_rsrv)/48;
        if (value >= 1024) {
            value = 1024 + value/16;
        }
        buf_prio_shr_e[q] = value;
        ref_prio_shr_e[q] = ref-(7-q)*prio_ref_rsrv;
    }

    buf_p_rsrv_i = 0;        /* No quaranteed extra space for ingress ports         */
    ref_p_rsrv_i = 20;       /* 20 extra frames can be pending shared between prios */
    buf_p_rsrv_e = 10000/48; /* 10kB reserved for each egress port                  */
    ref_p_rsrv_e = 20;       /* 20 extra frames can be pending shared between prios */

    buf_col_shr_i = 0x7FF; /* WM max - never reached */
    ref_col_shr_i = 0x7FF; /* WM max - never reached */
    buf_col_shr_e = 0x7FF; /* WM max - never reached */
    ref_col_shr_e = 0x7FF; /* WM max - never reached */

    i = 0;
    do { /* Reset default WM */
        H2_WRITE(VTSS_SYS_RES_CTRL_RES_CFG(i), 0);
        i++;
    } while (i<1024);

    /* Configure reserved space for all QoS classes per port */
    for (port_no = MIN_PORT; port_no <= MAX_PORT; port_no++) {
        for (q = 0; q < VTSS_PRIOS; q++) {
            H2_WRITE(VTSS_SYS_RES_CTRL_RES_CFG(port_no * VTSS_PRIOS + q + 0),   buf_q_rsrv_i);
            H2_WRITE(VTSS_SYS_RES_CTRL_RES_CFG(port_no * VTSS_PRIOS + q + 256), ref_q_rsrv_i);
            H2_WRITE(VTSS_SYS_RES_CTRL_RES_CFG(port_no * VTSS_PRIOS + q + 512), buf_q_rsrv_e);
            H2_WRITE(VTSS_SYS_RES_CTRL_RES_CFG(port_no * VTSS_PRIOS + q + 768), ref_q_rsrv_e);
        }
    }

    /* Configure shared space for all QoS classes */
    for (q = 0; q < VTSS_PRIOS; q++) {
        H2_WRITE(VTSS_SYS_RES_CTRL_RES_CFG((q + 216 + 0)),   buf_prio_shr_i[q]);
        H2_WRITE(VTSS_SYS_RES_CTRL_RES_CFG((q + 216 + 256)), ref_prio_shr_i[q]);
        H2_WRITE(VTSS_SYS_RES_CTRL_RES_CFG((q + 216 + 512)), buf_prio_shr_e[q]);
        H2_WRITE(VTSS_SYS_RES_CTRL_RES_CFG((q + 216 + 768)), ref_prio_shr_e[q]);
        H2_READ(VTSS_SYS_RES_CTRL_RES_CFG((q + 216 + 0)),   dp);
    }

    /* Configure reserved space for all ports */
    for (port_no = MIN_PORT; port_no <= MAX_PORT; port_no++) {
        H2_WRITE(VTSS_SYS_RES_CTRL_RES_CFG(port_no + 224 +   0), buf_p_rsrv_i);
        H2_WRITE(VTSS_SYS_RES_CTRL_RES_CFG(port_no + 224 + 256), ref_p_rsrv_i);
        H2_WRITE(VTSS_SYS_RES_CTRL_RES_CFG(port_no + 224 + 512), buf_p_rsrv_e);
        H2_WRITE(VTSS_SYS_RES_CTRL_RES_CFG(port_no + 224 + 768), ref_p_rsrv_e);
    }

    /* Configure shared space for  both DP levels (green:0 yellow:1) */
    for (dp = 0; dp < 2; dp++) {
        H2_WRITE(VTSS_SYS_RES_CTRL_RES_CFG(dp + 254 +   0), buf_col_shr_i);
        H2_WRITE(VTSS_SYS_RES_CTRL_RES_CFG(dp + 254 + 256), ref_col_shr_i);
        H2_WRITE(VTSS_SYS_RES_CTRL_RES_CFG(dp + 254 + 512), buf_col_shr_e);
        H2_WRITE(VTSS_SYS_RES_CTRL_RES_CFG(dp + 254 + 768), ref_col_shr_e);
    }
    return;
}


static void _setup_port (uchar port_no, uchar link_mode)
{
    if (link_mode != LINK_MODE_DOWN) {

        /* Set max frame length */
#if JUMBO
        H2_WRITE(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_MAXLEN_CFG(VTSS_TO_DEV(port_no)), JUMBO_SIZE);
#else
        H2_WRITE(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_MAXLEN_CFG(VTSS_TO_DEV(port_no)), 1518);
#endif  /* JUMBO */

        /* Set up flow control */
        h2_setup_flow_control(port_no, link_mode);

        h2_enable_exc_col_drop(port_no, 0);


        /* Core: Enable port for frame transfer */
        H2_WRITE_MASKED(VTSS_SYS_SYSTEM_SWITCH_PORT_MODE(port_no), VTSS_F_SYS_SYSTEM_SWITCH_PORT_MODE_PORT_ENA,
                        VTSS_F_SYS_SYSTEM_SWITCH_PORT_MODE_PORT_ENA);

        /* Core: Enable/disable system HDX */
        H2_WRITE_MASKED(VTSS_SYS_SYSTEM_FRONT_PORT_MODE(port_no),
                        ((link_mode & LINK_MODE_FDX_MASK) ? 0UL : VTSS_F_SYS_SYSTEM_FRONT_PORT_MODE_HDX_MODE),
                        VTSS_F_SYS_SYSTEM_FRONT_PORT_MODE_HDX_MODE);

        /* Take MAC, Port, Phy (intern) and PCS (SGMII/Serdes) clock out of reset */
        if (port_no > 9) {
            H2_WRITE(VTSS_DEV_PORT_MODE_CLOCK_CFG(VTSS_TO_DEV(port_no)),
            (link_mode & LINK_MODE_SPEED_MASK) == 3? 1: VTSS_F_DEV_PORT_MODE_CLOCK_CFG_LINK_SPEED(3UL-(link_mode & LINK_MODE_SPEED_MASK)));
        } else {
            H2_WRITE(VTSS_DEV_GMII_PORT_MODE_CLOCK_CFG(VTSS_TO_DEV(port_no)),0UL);
        }


        /* Enable MAC module */
        H2_WRITE(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_ENA_CFG(VTSS_TO_DEV(port_no)),
                 VTSS_F_DEV_MAC_CFG_STATUS_MAC_ENA_CFG_RX_ENA |
                 VTSS_F_DEV_MAC_CFG_STATUS_MAC_ENA_CFG_TX_ENA);
    }
    else {
        if (port_no < 10) {
            H2_WRITE(VTSS_DEV_GMII_PORT_MODE_CLOCK_CFG(VTSS_TO_DEV(port_no)),0xe);
        } else if (port_no < 12) {
            H2_WRITE(VTSS_DEV_PORT_MODE_CLOCK_CFG(VTSS_TO_DEV(port_no)),
                     VTSS_F_DEV_PORT_MODE_CLOCK_CFG_MAC_TX_RST |
                     VTSS_F_DEV_PORT_MODE_CLOCK_CFG_MAC_RX_RST |
                     VTSS_F_DEV_PORT_MODE_CLOCK_CFG_PCS_TX_RST |
                     VTSS_F_DEV_PORT_MODE_CLOCK_CFG_PCS_RX_RST |
                     VTSS_F_DEV_PORT_MODE_CLOCK_CFG_PORT_RST);
        } else {
            /* This register controls reset of different blocks in the device...
             when a *_RST field is set e1f the corresponding block is kept in reset */
            H2_WRITE(VTSS_DEV_PORT_MODE_CLOCK_CFG(VTSS_TO_DEV(port_no)), 0x1);
        }
    }
    VTSS_UPDATE_MASKS_DEBUG();
    vtss_update_masks();
}


static void _setup_pcs (uchar port_no, uchar link_mode)
{
    uchar sd_internal, sd_active_high, sd_enable, if_100fx, if_sgmii, if_type;
    ulong value;

    link_mode = link_mode; /* Happy compilier for future usage */

    /* Fixme: should use a MARCO configuration for user to define instead of static? */
    sd_internal = TRUE;
    sd_active_high = TRUE;
    sd_enable = FALSE;

    if_type = phy_map_miim_no(port_no);
    /* see define for PHY_MAP_MIIM_NO */
    if (if_type == 8) {
        if_100fx = TRUE;
        if_sgmii = FALSE;
    } else if (if_type <= 2) {
        if_100fx = FALSE;
        if_sgmii = TRUE;
    } else {
        if_100fx = FALSE;
        if_sgmii = FALSE;
    }

    /* PSC settings for 100fx/SGMII/SERDES */
    if (if_100fx) {
        /* 100FX PCS */
        value = VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_PCS_ENA |
                (sd_internal    ? 0 : VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_SEL) |
                (sd_active_high ? 1<<25 : 0)  |  /* VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_SD_POL [DBG] */
                (sd_enable      ? VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_ENA : 0);

        H2_WRITE_MASKED(VTSS_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG(VTSS_TO_DEV(port_no)),value,
                        VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_PCS_ENA |
                        VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_SEL |
                        (1<<25) | /* VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_POL [DBG] */
                        VTSS_F_DEV_PCS_FX100_CONFIGURATION_PCS_FX100_CFG_SD_ENA);
    } else {
        /* Choose SGMII or Serdes PCS mode */
        H2_WRITE(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_MODE_CFG(VTSS_TO_DEV(port_no)),
                 (if_sgmii ? VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_MODE_CFG_SGMII_MODE_ENA : 0));
        H2_WRITE(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG(VTSS_TO_DEV(port_no)),
                 (if_sgmii ? VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_ANEG_CFG_SW_RESOLVE_ENA : 0));
        H2_WRITE(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_SD_CFG(VTSS_TO_DEV(port_no)),
                 (sd_active_high ? VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_SD_CFG_SD_POL : 0) |
                 (sd_enable ? VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_SD_CFG_SD_ENA : 0) |
                 (sd_internal ? 0 : VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_SD_CFG_SD_SEL));

        /* Enable/disable PCS */
        H2_WRITE(VTSS_DEV_PCS1G_CFG_STATUS_PCS1G_CFG(VTSS_TO_DEV(port_no)),
                 VTSS_F_DEV_PCS1G_CFG_STATUS_PCS1G_CFG_PCS_ENA);
    }
}


static void _setup_mac(uchar port_no, link_mode)
{
    u32 fdx_gap, hdx_gap_1, hdx_gap_2, value;
    u8  link_spd_dpx;

    if (link_mode != LINK_MODE_DOWN) {
        link_spd_dpx = link_mode & LINK_MODE_SPEED_AND_FDX_MASK;

        /* Bugzilla 4388: disabling frame aging when in HDX */
        H2_WRITE_MASKED(VTSS_REW_PORT_PORT_CFG(port_no),
                        ((link_mode & LINK_MODE_FDX_MASK) ? 0 : VTSS_F_REW_PORT_PORT_CFG_AGE_DIS),
                        VTSS_F_REW_PORT_PORT_CFG_AGE_DIS);

        /* GIG/FDX mode */
        if ((link_spd_dpx == LINK_MODE_FDX_1000) || (link_spd_dpx == LINK_MODE_FDX_2500)){
            value = VTSS_F_DEV_MAC_CFG_STATUS_MAC_MODE_CFG_FDX_ENA |
                    VTSS_F_DEV_MAC_CFG_STATUS_MAC_MODE_CFG_GIGA_MODE_ENA;
        } else if (link_spd_dpx & LINK_MODE_FDX_MASK) {
            value = VTSS_F_DEV_MAC_CFG_STATUS_MAC_MODE_CFG_FDX_ENA;
        } else {
            value = 0;
        }

        /* Set GIG/FDX mode */
        H2_WRITE(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_MODE_CFG(VTSS_TO_DEV(port_no)), value);

        /* Default FDX gaps */
        if ((link_spd_dpx & LINK_MODE_SPEED_MASK) == LINK_MODE_SPEED_1000) {
            fdx_gap = 5;
        } else if (port_no < 10) {
            fdx_gap = 16; //James: the default value is 17 but the osc is little slower now
        } else if (port_no < 12) {
            if (link_spd_dpx == LINK_MODE_HDX_10 || link_spd_dpx == LINK_MODE_HDX_100)
                fdx_gap = 14; //James: the default value is 17 but the osc is little slower now
            else
                fdx_gap = 16; //James: the default value is 17 but the osc is little slower now
        } else {
            fdx_gap = 15;
        }

        /* Default HDX gaps */
        if (link_spd_dpx == LINK_MODE_HDX_10) {
            hdx_gap_1 = 11;
            hdx_gap_2 = 9;
        } else if (link_spd_dpx == LINK_MODE_HDX_100) {
            hdx_gap_1 = 7;
            hdx_gap_2 = 9;
        } else {
            hdx_gap_1 = 0;
            hdx_gap_2 = 0;
        }

        /* TBD: What about reduced_tx_ifg ? */
        /* Set MAC IFG Gaps */
        H2_WRITE(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_IFG_CFG(VTSS_TO_DEV(port_no)),
                 VTSS_F_DEV_MAC_CFG_STATUS_MAC_IFG_CFG_TX_IFG(fdx_gap) |
                 VTSS_F_DEV_MAC_CFG_STATUS_MAC_IFG_CFG_RX_IFG1(hdx_gap_1) |
                 VTSS_F_DEV_MAC_CFG_STATUS_MAC_IFG_CFG_RX_IFG2(hdx_gap_2));

        /* Set MAC HDX Late collision and load PCS seed. */
        H2_WRITE_MASKED(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_HDX_CFG(VTSS_TO_DEV(port_no)),
                        VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_LATE_COL_POS((port_no < 12)?64UL:67UL) |
                        VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_SEED(port_no) |
                        VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_SEED_LOAD,
                        VTSS_M_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_LATE_COL_POS|
                        VTSS_M_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_SEED|
                        VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_SEED_LOAD);

        /* Clear seed-load after a small delay (rising edge is sampled in rx-clock domain). */
        delay_1(2);
        H2_WRITE_MASKED(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_HDX_CFG(VTSS_TO_DEV(port_no)),
                        0,
                        VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_SEED_LOAD);

        /* Check point: Should be 9 or 11 no matter L10, L16 and L25? */
        if (port_no > 11) {
#if MAC_TO_MEDIA
            u8   mac_if = phy_map_miim_no(port_no);

            if ((mac_if != MAC_IF_SERDES) &&
                (mac_if != MAC_IF_100FX)  &&
				(mac_if != MAC_IF_SERDES_2_5G) &&
                (mac_if != MAC_IF_SGMII))
#endif
                _setup_pcs(port_no, link_mode);
        }

        if (link_spd_dpx == LINK_MODE_HDX_10 ||
            link_spd_dpx == LINK_MODE_HDX_100)
        {
            H2_WRITE_MASKED(VTSS_REW_PORT_PORT_CFG(port_no),
                            VTSS_F_REW_PORT_PORT_CFG_AGE_DIS,
                            VTSS_F_REW_PORT_PORT_CFG_AGE_DIS);
        } else {
            H2_WRITE_MASKED(VTSS_REW_PORT_PORT_CFG(port_no),
                            0,
                            VTSS_F_REW_PORT_PORT_CFG_AGE_DIS);
        }
    }
}


/*****************************************************************************
 *
 *
 * Public functions
 *
 *
 *
 ****************************************************************************/

/**
 * Reset switch chip.
 */
void h2_reset (void) small
{
    H2_WRITE_MASKED(VTSS_DEVCPU_GCB_DEVCPU_RST_REGS_SOFT_CHIP_RST,
                    VTSS_F_DEVCPU_GCB_DEVCPU_RST_REGS_SOFT_CHIP_RST_SOFT_CHIP_RST,
                    VTSS_F_DEVCPU_GCB_DEVCPU_RST_REGS_SOFT_CHIP_RST_SOFT_CHIP_RST);
    while (TRUE) {
    }
}


/**
 * Do basic initializations of chip needed after a chip reset.
 */
void h2_post_reset (void)
{
    ulong cmd;
    uchar locked;

    _h2_setup_cpu_if();

    /* Setup port MUX */
    h2_serdes_macro_config();


    /* Initialize memories, MAC-table, and VLAN-table + wait for done */
    H2_WRITE(VTSS_SYS_SYSTEM_RESET_CFG,
             VTSS_F_SYS_SYSTEM_RESET_CFG_MEM_ENA |
             VTSS_F_SYS_SYSTEM_RESET_CFG_MEM_INIT);
    start_timer(MSEC_100);

    do {
        H2_READ(VTSS_SYS_SYSTEM_RESET_CFG, cmd);
        if (timeout()) {
            sysutil_set_fatal_error(SYSTEM_INIT_FAILURE);
            return;
        }
    } while ((cmd & VTSS_F_SYS_SYSTEM_RESET_CFG_MEM_INIT) != 0);

    /* Enable the switch core */
    H2_WRITE_MASKED(VTSS_SYS_SYSTEM_RESET_CFG,
                    VTSS_F_SYS_SYSTEM_RESET_CFG_CORE_ENA,
                    VTSS_F_SYS_SYSTEM_RESET_CFG_CORE_ENA);

    /* Setup HSIO PLL */
    locked = h2_serdes_macro_phase_loop_locked();
    if(!locked) {
        sysutil_set_fatal_error(SYSTEM_INIT_FAILURE);
        return;
    }
    /* Format memories */
    h2_mactab_clear();
    h2_vlan_clear_tab();
    delay(MSEC_40);

    h2_mactab_agetime_set();

    _l26_buf_conf_set();


    /* Initialize leaky buckets */
    H2_WRITE(VTSS_SYS_SCH_SCH_LB_CTRL, VTSS_F_SYS_SCH_SCH_LB_CTRL_LB_INIT);

    /* Setup frame ageing - "2 sec" - in 4ns units */
    H2_WRITE(VTSS_SYS_SYSTEM_FRM_AGING, 0x1dcd6500);

    do { /* Wait until leaky buckets initialization is completed  */
        H2_READ(VTSS_SYS_SCH_SCH_LB_CTRL, cmd);
    } while(cmd & VTSS_F_SYS_SCH_SCH_LB_CTRL_LB_INIT);

    H2_WRITE(VTSS_ANA_ANA_TABLES_ANMOVED, 0);

}


/**
 * Presets link mode of all ports.
 */
void h2_init_ports (void)
{
    uchar port_no;

    for (port_no = MIN_PORT; port_no < MAX_PORT; port_no++) {
        /* Check point: only source mask to setup ? */
        H2_WRITE(VTSS_ANA_ANA_TABLES_PGID(port_no + VTSS_PGID_SOURCE_MASK_START), 0); /* Silence, please */
        h2_setup_port(port_no, LINK_MODE_DOWN);
    }

    VTSS_UPDATE_MASKS_DEBUG();
    vtss_update_masks();
}


/**
 * Set up port including MAC according to link_mode parameter.
 *
 * @see Please see main.h for a description of link_mode.
 */
void h2_setup_port (uchar port_no, uchar link_mode)
{
    h2_setup_mac(port_no, link_mode);
    _setup_port(port_no, link_mode);
}


/**
 * Port disable and flush procedure.
 *
 * @see     l26_port_conf_set() in Microchip API.
 */
void h2_setup_mac(uchar port_no, link_mode)
{
    u32 value;
    u8  mac_if = phy_map_miim_no(port_no);

    /* Port disable and flush procedure: */
    /* 0.1: Reset the PCS */
    if (port_no > 9) {
        H2_WRITE_MASKED(VTSS_DEV_PORT_MODE_CLOCK_CFG(VTSS_TO_DEV(port_no)), 0,
                        VTSS_F_DEV_PORT_MODE_CLOCK_CFG_PCS_RX_RST);
    }

    /* 1: Disable MAC frame reception */
    H2_WRITE_MASKED(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_ENA_CFG(VTSS_TO_DEV(port_no)), 0,
                    VTSS_F_DEV_MAC_CFG_STATUS_MAC_ENA_CFG_RX_ENA);


    /* 2: Disable traffic being sent to or from switch port */
    H2_WRITE_MASKED(VTSS_SYS_SYSTEM_SWITCH_PORT_MODE(port_no), 0,
                    VTSS_F_SYS_SYSTEM_SWITCH_PORT_MODE_PORT_ENA);

    /* 3: Disable dequeuing from the egress queues *\/ */
    H2_WRITE_MASKED(VTSS_SYS_SYSTEM_PORT_MODE(port_no), VTSS_F_SYS_SYSTEM_PORT_MODE_DEQUEUE_DIS,
                    VTSS_F_SYS_SYSTEM_PORT_MODE_DEQUEUE_DIS);

    /* 4: Wait a worst case time 8ms (jumbo/10Mbit) *\/ */
    delay_1(10);

    /* 5: Disable HDX backpressure (Bugzilla 3203) */
    H2_WRITE_MASKED(VTSS_SYS_SYSTEM_FRONT_PORT_MODE(port_no), 0UL,
                    VTSS_F_SYS_SYSTEM_FRONT_PORT_MODE_HDX_MODE);

    /* 6: Flush the queues accociated with the port */
    H2_WRITE_MASKED(VTSS_REW_PORT_PORT_CFG(port_no), VTSS_F_REW_PORT_PORT_CFG_FLUSH_ENA,
                    VTSS_F_REW_PORT_PORT_CFG_FLUSH_ENA);

    /* 7: Enable dequeuing from the egress queues */
    H2_WRITE_MASKED(VTSS_SYS_SYSTEM_PORT_MODE(port_no), 0UL,
                    VTSS_F_SYS_SYSTEM_PORT_MODE_DEQUEUE_DIS);

    /* 9: Reset the clock */
    if (port_no > 9) {
#if MAC_TO_MEDIA
        if (mac_if == MAC_IF_100FX) {
            H2_WRITE_MASKED(VTSS_DEV_PORT_MODE_CLOCK_CFG(VTSS_TO_DEV(port_no)),
                 VTSS_F_DEV_PORT_MODE_CLOCK_CFG_MAC_TX_RST |
                 VTSS_F_DEV_PORT_MODE_CLOCK_CFG_MAC_RX_RST |
                 VTSS_F_DEV_PORT_MODE_CLOCK_CFG_PHY_RST, 0x000000c4);
        } else 
#endif        
        {	
            H2_WRITE(VTSS_DEV_PORT_MODE_CLOCK_CFG(VTSS_TO_DEV(port_no)),
                 VTSS_F_DEV_PORT_MODE_CLOCK_CFG_MAC_TX_RST |
                 VTSS_F_DEV_PORT_MODE_CLOCK_CFG_MAC_RX_RST |
                 VTSS_F_DEV_PORT_MODE_CLOCK_CFG_PORT_RST);
        }                 
    } else {
        H2_WRITE(VTSS_DEV_GMII_PORT_MODE_CLOCK_CFG(VTSS_TO_DEV(port_no)),
                 VTSS_F_DEV_GMII_PORT_MODE_CLOCK_CFG_MAC_TX_RST |
                 VTSS_F_DEV_GMII_PORT_MODE_CLOCK_CFG_MAC_RX_RST |
                 VTSS_F_DEV_GMII_PORT_MODE_CLOCK_CFG_PORT_RST);
    }

    /* 8. Wait until flushing is complete */

    start_timer(MSEC_2000);

    do {
        H2_READ(VTSS_SYS_SYSTEM_SW_STATUS(port_no), value);
    } while ((value & VTSS_M_SYS_SYSTEM_SW_STATUS_EQ_AVAIL) && !timeout());

    if (timeout()) {
        println_str("flush timeout");
    }

    /* 10: Clear flushing */
    H2_WRITE_MASKED(VTSS_REW_PORT_PORT_CFG(port_no), 0, VTSS_F_REW_PORT_PORT_CFG_FLUSH_ENA);

    /* The port is disabled and flushed, now set up the port in the new operating mode */
    if (link_mode != LINK_MODE_DOWN) {
        _setup_mac(port_no, link_mode);
    }

    if (link_mode == LINK_MODE_DOWN)
        h2_mactab_flush_port(port_no);
}


/**
 * Check health status of switch chip.
 *
 * @return  Returns 0, if chip is ok, otherwise <> 0.
 */
uchar h2_check (void) small
{
#if H2_ID_CHECK
    ulong chip_id;
    H2_READ(VTSS_DEVCPU_GCB_CHIP_REGS_CHIP_ID, chip_id);
#if !defined(LUTON26_L10) && !defined(LUTON26_L16)
#define EXPECTED_CHIPID 0x074220e9
#elif !defined(LUTON26_L10)
#define EXPECTED_CHIPID 0x074210E9
#else
#define EXPECTED_CHIPID 0x074200E9
#endif
    if ((chip_id & 0x0FFFFFFF) != EXPECTED_CHIPID) {
        return 1;
    }
#endif
    return 0;
}


/*****************************************************************************
 *
 *
 * Public register access functions (primarily for code space optimization)
 *
 *
 *
 ****************************************************************************/


void h2_enable_exc_col_drop (uchar port_no, uchar drop_enable)
{
    H2_WRITE_MASKED(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_HDX_CFG(VTSS_TO_DEV(port_no)),
                    (drop_enable? 0UL:VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_RETRY_AFTER_EXC_COL_ENA),
                    VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_RETRY_AFTER_EXC_COL_ENA);
}

/*****************************************************************************
 *
 *
 * Help functions
 *
 *
 *
 ****************************************************************************/


/**
 * In Unmanaged code, this function returns the the port mask without sport
 * set. In managed code, the result mask is determined by the User defined
 * Private VLAN group (with the aggregation in mind).
 */
static port_bit_mask_t _vtss_get_pvlan_mask(vtss_iport_no_t sport)
{
    return ~PORT_BIT_MASK(sport);
}

static void _vtss_update_src_mask(port_bit_mask_t link_mask)
{
    vtss_iport_no_t     sport;
    port_bit_mask_t     member;

    for (sport = MIN_PORT; sport < MAX_PORT; sport++)
    {
        /* STP and Authentication state allow forwarding from port. */
        if (_ingr_forwarding(sport))
            member = _vtss_get_pvlan_mask(sport);
        else
            /* Exclude all ports by default */
            member = 0;

        H2_WRITE_MASKED(VTSS_ANA_ANA_TABLES_PGID(sport + VTSS_PGID_SOURCE_MASK_START),
                        member & link_mask,
                        ALL_PORTS);
    }
}

static void _vtss_update_dest_mask(port_bit_mask_t link_mask)
{
    // Fix compilation warning.
    link_mask = link_mask;
}


static void _vtss_update_aggr_mask(port_bit_mask_t link_mask)
{
    // Fix compilation warning.
    link_mask = link_mask;
}

void vtss_update_masks(void)
{
    port_bit_mask_t     link_mask;

#ifndef VTSS_COMMON_NDEBUG
    vtss_printf("vtss_update_masks: from file \"%s\" line %u\n",
                _common_file, _common_line);
#endif /* !VTSS_COMMON_NDEBUG */

    link_mask = phy_get_link_mask();

    /*
     * Update source mask
     */

    _vtss_update_src_mask(link_mask);

    /*
     * Update destination table
     */
    _vtss_update_dest_mask(link_mask);

    /*
     * Update aggregation masks
     */
    _vtss_update_aggr_mask(link_mask);

}
#if 0 //OLD API
void vtss_update_masks_old(void)
{
    vtss_sport_no_t     sport;
    port_bit_mask_t     link_mask;

    link_mask = phy_get_link_mask();

    for (sport = MIN_PORT; sport < MAX_PORT; sport++)
    {
        H2_WRITE_MASKED(VTSS_ANA_ANA_TABLES_PGID(sport + VTSS_PGID_SOURCE_MASK_START),
                        ~PORT_BIT_MASK(sport) & link_mask,
                        ALL_PORTS);
    }
}
#endif

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
