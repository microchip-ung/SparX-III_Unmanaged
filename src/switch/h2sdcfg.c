//Copyright (c) 2004-2024 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include "common.h"     /* Always include common.h at the first place of user-defined herder files */
#include "vtss_luton26_reg.h"
#include "h2sdcfg.h"
#include "timer.h"
#include "misc2.h"
#include "h2io.h"

/*****************************************************************************
 *
 *
 * Defines
 *
 *
 *
 ****************************************************************************/
#define RCOMP_CFG0 VTSS_IOREG(VTSS_TO_MACRO_CTRL,0x8)



// Defines for better be able to share code from the managed system.
#define L26_WRM H2_WRITE_MASKED
#define L26_WR H2_WRITE
#define L26_RD(reg, value) H2_READ(reg, *value)
#define u32 ulong
#define l26_wr h2_write
#define VTSS_RC(x) x
#define l26_sd6g_write(addr, wait) h2_sd6g_write(addr)
#define l26_sd1g_write(addr, wait) h2_sd1g_write(addr)
#define L26_WRM_SET(reg, bit) H2_WRITE_MASKED(reg, bit, bit)
#define L26_WRM_CLR(reg, bit) H2_WRITE_MASKED(reg, 0, bit)

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
void h2_sd6g_cfg_100fx(ulong addr);
/*****************************************************************************
 *
 *
 * Local data
 *
 *
 *
 ****************************************************************************/

/* ************************************************************************ */
void h2_sd6g_write(ulong addr)
/* ------------------------------------------------------------------------ --
 * Purpose     : Serdes6G write data
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/

{
    ulong dat, mask;

    mask = VTSS_F_MACRO_CTRL_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG_SERDES6G_WR_ONE_SHOT;

    /* Transfers data from MCB master (CSR target) to MCB slave */
    H2_WRITE(VTSS_MACRO_CTRL_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG,
            VTSS_F_MACRO_CTRL_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG_SERDES6G_ADDR(addr) | mask);

    do { /* Wait until write operation is completed  */
        H2_READ(VTSS_MACRO_CTRL_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG, dat);
    } while(dat & mask);

    delay_1(2);
}

/* ************************************************************************ */
void h2_sd6g_read(ulong addr)
/* ------------------------------------------------------------------------ --
 * Purpose     : Serdes6G write data
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/

{
    ulong dat, mask;

    mask = VTSS_F_MACRO_CTRL_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG_SERDES6G_RD_ONE_SHOT;

    /* Transfers data from MCB master (CSR target) to MCB slave */
    H2_WRITE(VTSS_MACRO_CTRL_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG,
            VTSS_F_MACRO_CTRL_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG_SERDES6G_ADDR(addr) | mask);

    do { /* Wait until write operation is completed  */
        H2_READ(VTSS_MACRO_CTRL_MCB_SERDES6G_CFG_MCB_SERDES6G_ADDR_CFG, dat);
    } while(dat & mask);

    delay_1(2);
}

/* ************************************************************************ */
void h2_sd6g_cfg (vtss_serdes_mode_t mode, ulong addr)
/* ------------------------------------------------------------------------ --
 * Purpose     : Serdes6G setup (Disable/2G5/QSGMII/SGMII)
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    u32 ib_rf, ctrl_data, if_mode=1, ob_ena_cas, ob_lev, ib_vbac=5, ib_vbcom=4, rcomp_val, ob_post_0=0, ib_ic_ac=0, ib_c=15, ib_chf=0;
    u32 ob_sr = 7;
    BOOL ena_lane=1, ena_rot=0, qrate, hrate, ob_ena1v, if_100fx=0, ib_cterm_ena;


//  VTSS_D("addr: 0x%lx, mode: %s", addr, l26_serdes_mode_txt(mode));
    ob_ena1v = 1; /* Based on Ref board design */
    ib_cterm_ena = 1; /* Based on Ref board design */

    switch (mode) {
    case VTSS_SERDES_MODE_2G5:
        /* Seredes6g_ob_cfg  */
        ob_post_0 = 2;
        /* Seredes6g_ob_cfg1 */
        ob_ena_cas = 1;
        ob_lev = ob_ena1v ? 48 : 63;
        /* Seredes6g_des_cfg --> see code */
        /* Seredes6g_ib_cfg */
        ib_ic_ac = ob_ena1v ? 2 : 0;
        ib_vbac  = ob_ena1v ? 4 : 5;
        ib_rf    = ob_ena1v ? 2 : 10;
        ib_vbcom = ob_ena1v ? 4 : 5;
        /* Seredes6g_ib_cfg1 */
        ib_c = ob_ena1v ? 6 : 10;
        ib_chf = ob_ena1v ? 1 : 0;
        /* Seredes6g_pll_cfg */
        ena_rot = 1;
        ctrl_data = 48;
        /* Seredes6g_common_cfg */
        qrate = 0;
        hrate = 1;
        break;
    case VTSS_SERDES_MODE_QSGMII:
        /* Seredes6g_ob_cfg  */
        ob_sr = 0; /* Based on Ref board design */
        ob_post_0 = 2;  /* Based on Ref board design */
        /* Seredes6g_ob_cfg1 */
        ob_ena_cas = 1;
        ob_lev = 24;
        /* Seredes6g_ib_cfg */
        ib_rf = 4;
        /* Seredes6g_ib_cfg1 */
        ib_c = 4;
        /* Seredes6g_pll_cfg */
        /* Seredes6g_pll_cfg */
        ctrl_data = 120;
        if_mode = 3;
        qrate = 0;
        hrate = 0;
        break;
    case VTSS_SERDES_MODE_SGMII:
        ob_lev = 48;
        ob_ena_cas = 2;
        ib_rf = 15;
        ctrl_data = 60;
        qrate = 1;
        hrate = 0;
        break;
    case VTSS_SERDES_MODE_100FX:
        ob_lev = 48;
        ob_ena_cas = 1;
        ib_rf = 15;
        ctrl_data = 60;
        qrate = 1;
        hrate = 0;
        if_100fx = 1;
        break;
    case VTSS_SERDES_MODE_1000BaseX:
        ob_lev = 48;
        ob_ena_cas = 2;
        ib_rf = 15;
        ctrl_data = 60;
        qrate = 1;
        hrate = 0;
        break;
    case VTSS_SERDES_MODE_DISABLE:
        ob_lev = 0;
        ob_ena_cas = 0;
        ib_rf = 0;
        ib_vbcom = 0;
        ena_rot = 0;
        ctrl_data = 0;
        qrate = 0;
        hrate = 0;
        break;
    default:
//        VTSS_E("Serdes6g mode %s not supported", l26_serdes_mode_txt(mode));
        return;
    }
    /* RCOMP_CFG0.MODE_SEL = 2 */
    VTSS_RC(l26_wr(RCOMP_CFG0,0x3<<8));

    /* RCOMP_CFG0.RUN_CAL = 1 */
    VTSS_RC(l26_wr(RCOMP_CFG0, 0x3<<8|1<<12));

    do { /* Wait for calibration to finish */
        L26_RD(VTSS_MACRO_CTRL_RCOMP_STATUS_RCOMP_STATUS, &rcomp_val);
    } while(rcomp_val & VTSS_F_MACRO_CTRL_RCOMP_STATUS_RCOMP_STATUS_BUSY);

    L26_RD(VTSS_MACRO_CTRL_RCOMP_STATUS_RCOMP_STATUS, &rcomp_val);
    rcomp_val = VTSS_X_MACRO_CTRL_RCOMP_STATUS_RCOMP_STATUS_RCOMP(rcomp_val);

    /* 1. Configure macro, apply reset */
    /* OB_CFG  */
    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG,
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_POL |
            VTSS_ENCODE_BITFIELD(rcomp_val+1,4,4) | /* RCOMP: bit 4-7 */
            VTSS_ENCODE_BITFIELD(ob_sr,0,4) |       /* SR:    bit 0-3 */
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_POST0(ob_post_0) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_SR_H |
            (ob_ena1v ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_ENA1V_MODE : 0),
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_POL |
            VTSS_ENCODE_BITMASK(4,4) | /* RCOMP: bit 4-7 */
            VTSS_ENCODE_BITMASK(0,4) | /* SR:    bit 0-3 */
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_POST0 |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_SR_H |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG_OB_ENA1V_MODE);

    /* OB_CFG1 */
    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1,
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1_OB_ENA_CAS(ob_ena_cas) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1_OB_LEV(ob_lev),
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1_OB_ENA_CAS |
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_OB_CFG1_OB_LEV);

     /* IB_CFG */
    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG,
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_IC_AC(ib_ic_ac) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_RT(15) |
            VTSS_ENCODE_BITFIELD(ib_vbac,7,3) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_RESISTOR_CTRL(rcomp_val+2) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_VBCOM(ib_vbcom) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_RF(ib_rf),
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_IC_AC |
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_RT |
            VTSS_ENCODE_BITMASK(7,3) |
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_RESISTOR_CTRL |
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_VBCOM |
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG_IB_RF);

    /* IB_CFG1 */
    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1,
            (ib_cterm_ena ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_CTERM_ENA : 0) |
            (ib_chf ? VTSS_BIT(7) : 0 ) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_C(ib_c) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_DIS_EQ |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_ENA_OFFSAC |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_ENA_OFFSDC |
            (if_100fx ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_FX100_ENA : 0) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_RST,
            VTSS_BIT(7) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_CTERM_ENA |
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_C |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_DIS_EQ |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_ENA_OFFSAC |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_ENA_OFFSDC |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_FX100_ENA |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_RST);

    /* DES_CFG */
    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG,
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_PHS_CTRL(6) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_MBTR_CTRL(2) |
           (if_100fx ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_CPMD_SEL(2) : 0) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_BW_HYST(5) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_BW_ANA(5),
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_PHS_CTRL |
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_MBTR_CTRL |
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_CPMD_SEL |
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_BW_HYST |
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_BW_ANA);

    /* PLL_CFG */
    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG,
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_FSM_CTRL_DATA(ctrl_data) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_FSM_ENA |
            (ena_rot ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_ENA_ROT : 0),
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_FSM_CTRL_DATA |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_FSM_ENA |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_PLL_CFG_PLL_ENA_ROT);

    /* Write masked to avoid changing RECO_SEL_* fields used by SyncE */
    /* COMMON_CFG */
    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG,
            (ena_lane ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_ENA_LANE : 0) |
            (hrate ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_HRATE : 0) |
            (qrate ? VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_QRATE : 0) |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_IF_MODE(if_mode),
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_SYS_RST |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_ENA_LANE |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_HRATE |
            VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_QRATE |
            VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_IF_MODE);


    /* MISC_CFG */
    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG,
            (if_100fx ? VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_DES_100FX_CPMD_ENA : 0),
            VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_DES_100FX_CPMD_ENA);

    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG,
            (if_100fx ? VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_DES_100FX_CPMD_ENA : 0) |
            VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_LANE_RST,
            VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_DES_100FX_CPMD_ENA |
            VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_LANE_RST);

    VTSS_RC(l26_sd6g_write(addr, L26_SERDES_WAIT));

    /* 2. Release PLL reset */
    L26_WRM_SET(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG,
                VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_COMMON_CFG_SYS_RST);
    VTSS_RC(l26_sd6g_write(addr, L26_SERDES_WAIT));

    /* 3. Release digital reset */
    L26_WRM_CLR(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1,
                VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_RST);

    L26_WRM(VTSS_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG, 0,
            VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_LANE_RST);
    VTSS_RC(l26_sd6g_write(addr, 0));
}


/**
 * Configures the Serdes1G/Serdes6G blocks based on mux mode
 * and Target.
 */
void h2_serdes_macro_config (void)
{
	/* Mux ports together with serdes macros */
    /*  00 - mode 0 - 3*QSGMII, 1*2G5, 1*SGMII */
    /*  01 - mode 1 - 2*2G5, 10*SGMII          */
    /*  10 - mode 2 - 2*QSGMII, 8*SGMII        */

#if defined(LUTON26_L25)
    H2_WRITE_MASKED(VTSS_DEVCPU_GCB_MISC_MISC_CFG,
       	    		VTSS_F_DEVCPU_GCB_MISC_MISC_CFG_SW_MODE(0UL),
       	    		VTSS_M_DEVCPU_GCB_MISC_MISC_CFG_SW_MODE);
    h2_sd6g_cfg(VTSS_SERDES_MODE_QSGMII, 0xE);     /* Enable QSGMII, Serdes6g (3-1) */
    h2_sd6g_cfg(VTSS_SERDES_MODE_SGMII, 0x1);      /* Enable SGMII, Serdes6g (0) */
#elif defined(LUTON26_L25UN)
    H2_WRITE_MASKED(VTSS_DEVCPU_GCB_MISC_MISC_CFG,
                    VTSS_F_DEVCPU_GCB_MISC_MISC_CFG_SW_MODE(0UL),
                    VTSS_M_DEVCPU_GCB_MISC_MISC_CFG_SW_MODE);
    h2_sd6g_cfg(VTSS_SERDES_MODE_QSGMII, 0xE);     /* Enable QSGMII, Serdes6g (3-1) */
    h2_sd6g_cfg(VTSS_SERDES_MODE_SGMII, 0x1);      /* Enable SGMII, Serdes6g (0) */
#elif defined(LUTON26_L16)
#ifdef LUTON26_L16_QSGMII_EXT_PHY
    H2_WRITE_MASKED(VTSS_DEVCPU_GCB_MISC_MISC_CFG,
                    VTSS_F_DEVCPU_GCB_MISC_MISC_CFG_SW_MODE(0UL),
                    VTSS_M_DEVCPU_GCB_MISC_MISC_CFG_SW_MODE);
    h2_sd6g_cfg(VTSS_SERDES_MODE_QSGMII, 0xF);     /* Enable QSGMII, Serdes6g (3-0) */
#else
    H2_WRITE_MASKED(VTSS_DEVCPU_GCB_MISC_MISC_CFG,
       	    		VTSS_F_DEVCPU_GCB_MISC_MISC_CFG_SW_MODE(1UL),
       	    		VTSS_M_DEVCPU_GCB_MISC_MISC_CFG_SW_MODE);
    h2_sd6g_cfg(VTSS_SERDES_MODE_SGMII, 0xF);      /* Enable SGMII, Serdes6g (3-0) */
#endif /* LUTON26_L16_QSGMII_EXT_PHY */
#elif defined(LUTON26_L10)
    H2_WRITE_MASKED(VTSS_DEVCPU_GCB_MISC_MISC_CFG,
       	    		VTSS_F_DEVCPU_GCB_MISC_MISC_CFG_SW_MODE(1UL),
       	    		VTSS_M_DEVCPU_GCB_MISC_MISC_CFG_SW_MODE);
    h2_sd6g_cfg(VTSS_SERDES_MODE_SGMII, 0x3);      /* Enable SGMII, Serdes1g (1-0) */
#endif
}


/**
 * Serdes6G setup (Disable/2G5/QSGMII/SGMII).
 */
void h2_sd6g_cfg_change (vtss_serdes_mode_t mode, ulong addr)
{
    h2_sd6g_read(addr);
    delay(MSEC_20);
    h2_sd6g_cfg(mode, addr);
    h2_sd6g_write(addr);
    delay_1(1);
    if(mode == VTSS_SERDES_MODE_100FX) {
        h2_sd6g_cfg_100fx(addr);
    }
}

/* ************************************************************************ */
void h2_sd6g_cfg_100fx(ulong addr)
/* ------------------------------------------------------------------------ --
 * Purpose     : Configures the sd6g 100fx mode
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    h2_sd6g_read(addr);
    H2_WRITE_MASKED(VTSS_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG,
                    VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_DES_100FX_CPMD_ENA,
                    VTSS_F_MACRO_CTRL_SERDES6G_DIG_CFG_SERDES6G_MISC_CFG_DES_100FX_CPMD_ENA);

    H2_WRITE_MASKED(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1,
                    VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_FX100_ENA,
                    VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_IB_CFG1_IB_FX100_ENA);

    H2_WRITE_MASKED(VTSS_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG,
                    VTSS_F_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_CPMD_SEL(2UL),
                    VTSS_M_MACRO_CTRL_SERDES6G_ANA_CFG_SERDES6G_DES_CFG_DES_CPMD_SEL);
    h2_sd6g_write(addr);
}

/* ************************************************************************ */
uchar h2_serdes_macro_phase_loop_locked (void)
/* ------------------------------------------------------------------------ --
 * Purpose     : Init PLL5G
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    ulong locked;
    const ulong code pll5g_cfg2 = VTSS_MACRO_CTRL_PLL5G_CFG_PLL5G_CFG2;

    H2_WRITE(VTSS_MACRO_CTRL_PLL5G_CFG_PLL5G_CFG4, 0x00007ae0);
    delay_1(2);
    H2_WRITE(pll5g_cfg2, 0x00610400);
    delay_1(2);
    H2_WRITE(pll5g_cfg2, 0x00610c00);
    delay_1(2);
    H2_WRITE(pll5g_cfg2, 0x00610800);
    delay_1(2);
    H2_WRITE(pll5g_cfg2, 0x00610000);
    delay_1(2);
    H2_READ(VTSS_MACRO_CTRL_PLL5G_STATUS_PLL5G_STATUS0, locked);
    return (locked & VTSS_F_MACRO_CTRL_PLL5G_STATUS_PLL5G_STATUS0_LOCK_STATUS);
}

