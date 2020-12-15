//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#ifndef __HWPORT_H__
#define __HWPORT_H__

#include "common.h"     /* Always include common.h at the first place of user-defined herder files */

/**
 * Convert internal port number (physical GMII number) to external (logic)
 * port number.
 */
vtss_fport_no_t port2ext (vtss_sport_no_t port_no);

/**
 * Convert external (logic) port number to internal port number (physical GMII
 * number).
 */
vtss_sport_no_t port2int (vtss_fport_no_t port_no);

/**
 * Read hardware version from hardware.
 *
 * @TODO    This is a dummy. Customers should customize this if needed.
 */
void  get_hw_version (void);

/**
 * Read configured MAC address and adjust it according to specified port.
 */
void  get_mac_addr (uchar port_no, uchar *mac_addr);

/**
 * Remove hardware reset of the PHY chips.
 */
void phy_hw_init (void);

/**
 * Configure the GPIO pins mode and SGPIO pins mode.
 */
void gpio_init (void);

#endif /* __HWPORT_H__ */

/*****************************************************************************
 *                                                                           *
 *  End of file.                                                             *
 *                                                                           *
 *****************************************************************************/

