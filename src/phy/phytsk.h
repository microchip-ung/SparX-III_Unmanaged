//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#ifndef __PHYTSK_H__
#define __PHYTSK_H__

#include "common.h"     /* Always include common.h at the first place of user-defined herder files */

/**
 * Tick PHY timers. To be called every 10 msec (approximately).
 *
 * @note    Don't call this from timer interrupt function.
 */
void   phy_timer_10                 (void);

/**
 * Do run-time check on all PHYs.
 */
uchar  phy_check_all                (void);


#if LOOPBACK_TEST
void   phy_restart                  (vtss_port_no_t port_no);
#endif /* LOOPBACK_TEST */


/**
 * Init PHY and data used by phy task (actually 'port' task).
 */
uchar  phy_tsk_init                 (void);

/**
 * Activate state machine for each port/PHY. This is the 'port task' which is
 * performing the port status detection and handles link up / down events.
 */
void   phy_tsk                      (void);

/**
 * Get current link mode with bit mapping as defined in main.h
 */
uchar  phy_get_link_mode_raw        (vtss_port_no_t port_no);


/**
 * Get (software) link state (up/down) for specified port.
 *
 * @return TRUE     If link is up.
 * @return FALSE    Otherwise.
 */
uchar  phy_get_link_state           (vtss_port_no_t port_no);


/**
 * Get link state (up/down) for all ports.
 *
 * @retval  Return bit mask where each bit represents a port.
 */
port_bit_mask_t phy_get_link_mask   (void);


#if TRANSIT_VERIPHY


/**
 * Run veriphy on all ports.
 *
 * @todo Run only on PHY ports.
 */
void   phy_veriphy_all              (void);


#endif /* TRANSIT_VERIPHY */

#if TRANSIT_ACTIPHY
/**
 * Run ActiPHY on all PHY ports.
 *
 */
void phy_actiphy_all (void);
#endif /* TRANSIT_ACTIPHY */


#if TRANSIT_FAN_CONTROL || TRANSIT_THERMAL
ushort phy_get_sys_temp             (void);
#endif /* TRANSIT_FAN_CONTROL || TRANSIT_THERMAL */


#if TRANSIT_THERMAL
/**
 * State machine for temperature monitor. Monitor and set up switch port.
 */
void phy_handle_temperature_protect (void);


void phy_temperature_timer_1sec     (void);


#endif /* TRANSIT_THERMAL */


#endif /* __PHYTSK_H__ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
