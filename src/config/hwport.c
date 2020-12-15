//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include "common.h"     /* Always include common.h at the first place of user-defined herder files */

#include "hwport.h"
#include "phydrv.h"
#include "main.h"
#include "vtss_luton26_reg.h"
#include "spiflash.h"
#include "h2gpios.h"
#include "h2io.h"
#include "timer.h"
#include "version.h"

#ifndef PHY_DEBUG
#define PHY_DEBUG (0)
#endif

#if PHY_DEBUG
#include "print.h"
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
 * Public data
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
 ****************************************************************************/


static code uchar port2ext_tab [MAX_PORT] = { PORT_MAP_2_EXT};

static code uchar port2int_tab [MAX_PORT + 1] = {
    0,  /* dummy entry for index 0 */
    PORT_MAP_2_INT
};

/*****************************************************************************
 *
 *
 * Public Functions
 *
 *
 ****************************************************************************/


vtss_fport_no_t port2ext (vtss_sport_no_t port_no)
{
    return port2ext_tab[port_no];
}


vtss_sport_no_t port2int (vtss_fport_no_t port_no)
{
    return port2int_tab[port_no];
}


void get_hw_version (void)
{
}


void get_mac_addr (uchar port_no, uchar *mac_addr)
{
    /*
    ** Only one MAC address will be supported in Luton26 so
    ** return this MAC address anyway
    */
    if (port_no == SYSTEM_MAC_ADDR) {
        flash_read_mac_addr(mac_addr);
        return;
    } else {
#if 1  //update per port smac address according to the stored(changed) system mac address
        flash_read_mac_addr(mac_addr);
#else  //use system default mac address for per port's smac
        mac_addr[0] = spiflash_mac_addr[0];
        mac_addr[1] = spiflash_mac_addr[1];
        mac_addr[2] = spiflash_mac_addr[2];
        mac_addr[3] = spiflash_mac_addr[3];
        mac_addr[4] = spiflash_mac_addr[4];
        mac_addr[5] = spiflash_mac_addr[5];
#endif
    }

    port_no = port_no + 1 - MIN_PORT;

    if ((mac_addr[5] += port_no) < port_no) {
        if (++mac_addr[4] == 0) {
            mac_addr[3]++;
        }
    }
}


void phy_hw_init (void)
{
    ulong cmd;

    /* Release reset of built-in PHYs */
    H2_WRITE_MASKED(VTSS_DEVCPU_GCB_DEVCPU_RST_REGS_SOFT_CHIP_RST, 0x00, \
                    VTSS_F_DEVCPU_GCB_DEVCPU_RST_REGS_SOFT_CHIP_RST_SOFT_PHY_RST);
    do {
        H2_READ(VTSS_DEVCPU_GCB_MISC_MISC_STAT, cmd);
    } while ((cmd & VTSS_F_DEVCPU_GCB_MISC_MISC_STAT_PHY_READY) == 0);

#ifdef LUTON26_L16
#ifdef LUTON26_L16_QSGMII_EXT_PHY
    h2_gpio_mode_set(15, VTSS_GPIO_OUT);
    h2_gpio_write(15, 0);
    delay(MSEC_20);

#if PHY_DEBUG
    println_str("step 4. release reset");
#endif

    h2_gpio_write(15, 1);

#if PHY_DEBUG
    println_str("step 5. wait 120 ms");
#endif

    // Datasheet says 120 ms
    delay(MSEC_100);
    delay(MSEC_20);
#endif
#endif

}


void gpio_init (void)
{
    h2_sgpio_enable();
}

/*****************************************************************************
 *                                                                           *
 *  End of file.                                                             *
 *                                                                           *
 *****************************************************************************/

