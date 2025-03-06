//Copyright (c) 2004-2025 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT


#ifndef __PHYCONF_H__
#define __PHYCONF_H__

#ifndef __INCLUDE_CONFIGS__
#error "phyconf.h is for common.h only"
#endif

#ifdef  LUTON26_L25UN
#define VSC5610EV_UN  1
//#define LUTON26_L25   0
#endif

/*****************************************************************************
 *
 *
 * Select PHY parts
 *
 *
 *
 ****************************************************************************/

#define VTSS_COBRA      0 /* Enables support of VSC8211, VSC8221 */

#define VTSS_QUATTRO    0 /* Enables support of VSC8224, VSC8234, VSC8244 */
#define VTSS_SPYDER     0 /* Enables support of VSC8538, VSC8558, VSC8658 */
#define VTSS_ENZO       0 /* Enables support of VSC8664 */

#define VTSS_ATOM12     1 /* Enables support of VSC8512, VSC8522 */
#define VTSS_ATOM12_A   0
#define VTSS_ATOM12_B   0
#define VTSS_ATOM12_C   0
#define VTSS_ATOM12_D   1

#define VTSS_TESLA      0 /* Enables support of VSC8504, VSC8552 */
#define VTSS_TESLA_A    0
#define VTSS_TESLA_B    0
#define VTSS_TESLA_D    0

#define VTSS_ELISE      0 /* Enables support of VSC8514 */
#if VTSS_ELISE
#define VTSS_ELISE_A    0
#endif // VTSS_ELISE

/*****************************************************************************
 *
 *
 * Frame gap settings
 *
 *
 *
 ****************************************************************************/

/*****************************************************************************
 *
 *
 * Mode and clock settings
 *
 *
 *
 ****************************************************************************/

/*****************************************************************************
 *
 *
 * PHY mode settings
 *
 *
 *
 ****************************************************************************/

#if VTSS_QUATTRO
#define VTSS_REG_23 0x1a24
#endif

/*****************************************************************************
 *
 *
 * Possibly macro for reading current speed and duplex of PHY
 *
 *
 *
 ****************************************************************************/

/*
** Define a macro that reads info about speed and duplex mode
** from the PHY associated with port_no.
** It must update link_mode bit 0 and 1 with the current speed:
** bit 1:0 = 00: 	 No link
** bit 1:0 = 01: 1000 Mbit/s
** bit 1:0 = 10:  100 Mbit/s
** bit 1:0 = 11:   10 Mbit/s
**
** and bit 4 with current duplex mode:
** bit 4 = 0: half duplex
** bit 4 = 1: full duplex
*/
#define PHY_READ_SPEED_AND_FDX(port_no, reg_val, link_mode) {     \
}

/*****************************************************************************
 *
 *
 * PHY register 9 settings
 *
 *
 *
 ****************************************************************************/

/*
 * Define pattern to write to PHY register 9 for 1000BASE-T control.
 */
#define PHY_REG_9_CONFIG 0x0600

/*****************************************************************************
 *
 *
 * PHY LED settings
 *
 *
 *
 ****************************************************************************/

/* LED0-2 display only copper status; LED3 display fiber status only */
#define PHY_LED_MODE     0x7031
#define PHY_LED_MODE_SFP 0x7eee

/*****************************************************************************
 *
 *
 * Mapping between physical ports and PHYs
 *
 *
 *
 ****************************************************************************/

#if !defined(LUTON26_L10) && !defined(LUTON26_L16)

/*
** Define mapping between port numbers and PHYs. For each port specify the
** PHY number of the PHY connected to the port. And for each port specify
** the MIIM number of the management bus for the connected PHY.

** Codes used in PHY_MAP_MIIM_NO:
   0 -- MII 0 to internal PHY
   1 -- MII 1 to external PHY
   2 -- SGMII directly via MAC
   7 -- Unconnected
   8 -- 100FX directly via MAC
   9 -- SerDes directly	via MAC
 0xa -- Auto selection via SFP info
 0xb -- SGMII_2G5 MAC-to-MAC
** Settings at PHY_MAP_PHY_NO only care when PHY_MAP_MIIM_NO is 0/1.

Luton25 port number:
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,xx,25
  -------------------------------------------------------------------------- */
#define PHY_MAP_PHY_NO  { \
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11, 0, 0}

#define PHY_MAP_MIIM_NO { \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 7, 0xb}

#define PHY_MAP_COMA_MODE_DISABLE { \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0}


/*
** Define GPIO port numbers of PHYs. For each port specify the
** PHY number of the PHY connected to the port.
 */
#define PHY_GPIO_PORT   { \
  0, 12}

#elif !defined(LUTON26_L10)

/*
** Define mapping between port numbers and PHYs. For each port specify the
** PHY number of the PHY connected to the port. And for each port specify
** the MIIM number of the management bus for the connected PHY.

** Codes used in PHY_MAP_MIIM_NO:
   0 -- MII 0 to internal PHY
   1 -- MII 1 to external PHY
   2 -- SGMII directly via MAC
   7 -- Unconnected
   8 -- 100FX directly via MAC
   9 -- SerDes directly	via MAC
** Settings at PHY_MAP_PHY_NO only care when PHY_MAP_MIIM_NO is 0/1.

Luton16 port number:
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,xx,xx,xx,xx,16,xx,xx,19,xx,xx,xx,xx,24,25
  -------------------------------------------------------------------------- */
#ifdef LUTON26_L16_QSGMII_EXT_PHY
#define PHY_MAP_PHY_NO  { \
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11, 0, 1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

#define PHY_MAP_MIIM_NO { \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7}

#define PHY_MAP_COMA_MODE_DISABLE { \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#else
#define PHY_MAP_PHY_NO  { \
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 2, 3}

#define PHY_MAP_MIIM_NO { \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 7, 7, 7, 1, 7, 7, 1, 7, 7, 7, 7, 1, 1}

#define PHY_MAP_COMA_MODE_DISABLE { \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#endif /* LUTON26_L16_QSGMII_EXT_PHY */


/*
** Define GPIO port numbers of PHYs. For each port specify the
** PHY number of the PHY connected to the port.
 */
#define PHY_GPIO_PORT   { \
  0}

#else

/*
** Define mapping between port numbers and PHYs. For each port specify the
** PHY number of the PHY connected to the port. And for each port specify
** the MIIM number of the management bus for the connected PHY.

** Codes used in PHY_MAP_MIIM_NO:
   0 -- MII 0 to internal PHY
   1 -- MII 1 to external PHY
   2 -- SGMII directly via MAC
   7 -- Unconnected
   8 -- 100FX directly via MAC
   9 -- SerDes directly	via MAC
** Settings at PHY_MAP_PHY_NO only care when PHY_MAP_MIIM_NO is 0/1.

Luton10 port number:
  0, 1, 2, 3, 4, 5, 6, 7, x, x,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,24,25
  -------------------------------------------------------------------------- */
#define PHY_MAP_PHY_NO  { \
  0, 1, 2, 3, 4, 5, 6, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

#define PHY_MAP_MIIM_NO { \
  0, 0, 0, 0, 0, 0, 0, 0, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 0xb, 0xb}

#define PHY_MAP_COMA_MODE_DISABLE { \
  0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

/*
** Define GPIO port numbers of PHYs. For each port specify the
** PHY number of the PHY connected to the port.
 */
#define PHY_GPIO_PORT   { \
  0}

#endif


/*****************************************************************************
 *
 *
 * Enabling/disabling of echo mode in 10 Mbps half-duplex
 *
 *
 *
 ****************************************************************************/


/*
** Define macros for disabling/enabling echo mode.
** On some PHYs echo mode must be disabled at 10 Mbps in half-duplex
** mode to work together with the MAC.
*/

#define PHY_DISABLE_ECHO_MODE(port_no) \
    phy_write_masked(port_no, 0x16, (ushort) 1 << 13, (ushort) 1 << 13)

#define PHY_ENABLE_ECHO_MODE(port_no) \
    phy_write_masked(port_no, 0x16, (ushort) 0 << 13, (ushort) 1 << 13)


/*****************************************************************************
 *
 *
 * Enabling/disabling PHY id check
 *
 *
 *
 ****************************************************************************/

/*
** Define whether PHY ids should be checked as run-time check.
** Set PHY_ID_CHECK to 1 to enable check, or to 0 to disable check.
*/
#define PHY_ID_CHECK 0

/*
** Define PHY id to be checked if PHY_ID_CHECK is set to 1
** Set PHY_OUI_MSB to the expected value of register 2.
*/
#define PHY_OUI_MSB 0x000F




/*****************************************************************************
 *
 *
 * SFP module configuration
 *
 *
 ****************************************************************************/

/*
** Set SFP_NUM to number of SFP modules. If no SFPs, set it to 0, and the
** remaining SFP configuration is don't care.
*/

#define SFP_RATE_SEL_GPIO       4
#define SFP_MODULE_DETECT_GPIO  3
#define SFP_TX_DISABLE_GPIO     2

#define SFP_MODULE_GPIO_PORT    12

/*****************************************************************************
 *
 *
 * Option for controlling flow control mode when auto-negotiation failed and
 * the fell back to half-duplex mode.
 *
 * This is applicable with the following criterion:
 * - In Unmanaged software.
 * - Auto-negotiation failed.
 *
 *
 ****************************************************************************/
#define PHY_AN_FAIL_FLOW_CTRL_MODE  (0)


/*****************************************************************************
 *
 *
 * Options for controlling flow control mode when the auto-negotiation result
 * is half-duplex.
 *
 * This is applicable with the following criterion:
 * - In Unmanaged software.
 * - Auto-negotiation failed or auto-negotiation succeeded with half duplex
 *   agreement.
 *
 *
 ****************************************************************************/
#define PHY_HDX_FLOW_CTRL_MODE      (0)

#endif /* __PHYCONF_H__ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
