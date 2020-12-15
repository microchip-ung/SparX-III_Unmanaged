//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT


#ifndef __H2SDCFG_H__
#define __H2SDCFG_H__

/** \brief Serdes macro mode */
typedef enum
{
    VTSS_SERDES_MODE_DISABLE,  /**< Disable serdes */
    VTSS_SERDES_MODE_2G5,      /**< 2.5G mode      */
    VTSS_SERDES_MODE_QSGMII,   /**< QSGMII mode    */
    VTSS_SERDES_MODE_SGMII,     /**< SGMII mode     */
    VTSS_SERDES_MODE_100FX,     /**< 100FX mode     */
    VTSS_SERDES_MODE_1000BaseX  /**< 1000BaseX mode */
} vtss_serdes_mode_t;

void h2_sd6g_write(ulong addr);
void h2_sd6g_read(ulong addr);
void h2_sd6g_cfg (vtss_serdes_mode_t mode, ulong addr);

void h2_serdes_macro_config (void);
void h2_sd6g_cfg_change (vtss_serdes_mode_t mode, ulong addr);
uchar h2_serdes_macro_phase_loop_locked (void);


#endif

