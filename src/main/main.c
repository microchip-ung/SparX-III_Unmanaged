//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include "common.h"     /* Always include common.h at the first place of user-defined herder files */
#include "sysutil.h"
#include "event.h"

#include "interrupt.h"
#include "hwport.h"
#include "spiflash.h"
#include "timer.h"
#include "uartdrv.h"
#include "phytsk.h"
#include "print.h"
#include "clihnd.h"
#include "phydrv.h"
#include "phymap.h"
#include "phy_family.h"
#include "phy_base.h"
#include "h2.h"
#include "h2txrx.h"

#include "misc2.h"
#include "i2c_h.h"
#if UNMANAGED_PORT_STATISTICS_IF
#include "h2stats.h"
#endif

#include "taskdef.h"

#if TRANSIT_FAN_CONTROL
#include "fan_api.h"
#endif

#if TRANSIT_SNMP
#include "snmp.h"
#include "traps.h"
#endif /* TRANSIT_SNMP */

#if TRANSIT_LLDP
#include "lldp.h"
#endif /* TRANSIT_LLDP */

#if TRANSIT_EEE
#include "eee_api.h"
#include "eee_base_api.h"
#endif

#if TRANSIT_LOOPDETECT
#include "loopdet.h"
#endif

#if TRANSIT_POE
#include "poetsk.h"
#endif /* TRANSIT_POE */

#if TRANSIT_POE_LLDP
#include "poe_os.h"
#endif

#if FRONT_LED_PRESENT
#include "ledtsk.h"
#endif

#include "main.h"

/*****************************************************************************
 *
 *
 * Public data
 *
 *
 ****************************************************************************/


#if UNMANAGED_PORT_STATISTICS_IF
bit     start_display_statis_bit;
uchar   port_cnt;
#endif

/*****************************************************************************
 *
 *
 * Defines
 *
 *
 ****************************************************************************/


/*****************************************************************************
 *
 *
 * Typedefs and enums
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

/*****************************************************************************
 *
 *
 * Local Functions
 *
 *
 ****************************************************************************/


/**
 *
 */
static void error_check (void)
{
    if (h2_check() != 0) {
        sysutil_set_fatal_error(H2_GENERAL_FAILURE);
    }

    if (phy_check_all() != 0) {
        sysutil_set_fatal_error(PHY_GENERAL_FAILURE);
    }

}


/*****************************************************************************
 *
 *
 * Public Functions
 *
 *
 ****************************************************************************/


/**
 * Control initialization sequence and control round-robin loop.
 */
void main (void)
{
#if  VTSS_COBRA
    vtss_port_no_t port_no;
    phy_id_t phy_id;
#endif

    /* Determine hardware version */
    get_hw_version();

    /* Set up timer 0 for system tick */
    timer_1_init();

    /* Initialize drivers */
#ifndef NO_DEBUG_IF
    uart_init();
#endif

    /* Setup interrupts */
    ext_interrupt_init();

    /* Enable global interrupt */
    EA = 1;

    /* Wait 20 msec before accessing chip and PHYs */
    delay_1(20);

    h2_post_reset();

    gpio_init();

    /* Read configuration data into RAM */
#if LUTON_UNMANAGED_SWUP
    flash_init();
#endif
    flash_load_config();

    /*
     * Do some health check of chip
     */

    if ((h2_check() != 0) || sysutil_get_fatal_error()) {
        sysutil_hang();
    }

    /*
     * Initialize chip and software
     */
    phy_hw_init();

    h2_init_ports();

    /*
     * Initialize and check PHYs, hang the system if chek not passed.
     *
     * @note    Phy Init must come after the h2_init_port, because the port
     *          clocks must be enabled.
     */
    print_cr_lf();
    println_str("Initializing the PHYs...");
    println_str("System might hang if MDC/MDIO interface access the PHY registers unsuccessfully!");
    if (phy_tsk_init() != FALSE) {
        sysutil_hang();
    }

#if  VTSS_COBRA 
    /* Disable Power Savings */
    for (port_no = MIN_PORT; port_no < MAX_PORT; port_no++) {
        phy_read_id(port_no, &phy_id);
        if (phy_map(port_no) && (phy_id.family == VTSS_PHY_FAMILY_COBRA)) {
            phy_page_tp(port_no);
            phy_write_masked(port_no, 12, 0x0000, 0xfc00);
            phy_write_masked(port_no, 24, 0x2000, 0x2000);
            phy_page_std(port_no);
            phy_write_masked(port_no, 28, 0x0000, 0x0040);
            phy_write_masked(port_no, 0, 0x8000, 0x8000);
            delay(MSEC_20);

        }
    }
#endif


    /* Turn on green front LED when power up done */
#if FRONT_LED_PRESENT
    led_init();
#endif

#if TRANSIT_LLDP || LOOPBACK_TEST
    h2_rx_init();
#endif
#if TRANSIT_LLDP
    lldp_init();
#endif /* TRANSIT_LLDP */

#if (WATCHDOG_PRESENT && WATCHDOG_ENABLE)
    enable_watchdog();
#endif

#if TRANSIT_VERIPHY
    phy_tsk(); // Activate state machine. We have seen that dual media doesn't pass VeriPhy if state machine is not activated.
    phy_veriphy_all();
#endif
#if TRANSIT_ACTIPHY
    phy_actiphy_all();
#endif

#if TRANSIT_EEE
    eee_mgmt_int();
#endif

#if TRANSIT_FAN_CONTROL
    fan_init();
#endif

#if USE_HW_TWI
    /* Initial I2C */
    i2c_init();
#endif
    print_str("Enter ? to get the CLI help command");
    print_cr_lf();

    /************************************************************************
     *
     *
     * Loop forever
     *
     *
     *
     ************************************************************************/

    while (TRUE) {
        /* For profiling/debug purposes */
        MAIN_LOOP_ENTER();

        /* If fatal error happens, reboot */
        if (sysutil_get_fatal_error())
            sysutil_reboot();

        /* Handle any commands received on RS232 interface */
#ifndef NO_DEBUG_IF
        TASK(TASK_ID_CLI, cli_tsk());
#endif

        /* Suspended. Skip all tasks besides CLI */
        if (sysutil_get_suspend())
            continue;

#if TRANSIT_LLDP || LOOPBACK_TEST
        /* Handle any packets received */
        TASK(TASK_ID_RX_PACKET, rx_packet_tsk());
#endif

        /*
         * Do 10 msec jobs
         */
        if (ms_10_timeout_flag) {
            ms_10_timeout_flag = 0;

            TASK(TASK_ID_PHY_TIMER, phy_timer_10());
            TASK(TASK_ID_PHY, phy_tsk());

#if (WATCHDOG_PRESENT && WATCHDOG_ENABLE)
            TASK(TASK_ID_WATCHDOG, kick_watchdog());
#endif

        }

        /*
         * Do 100 msec jobs
         */

        if (ms_100_timeout_flag) {
            ms_100_timeout_flag = 0;
#if FRONT_LED_PRESENT
            led_tsk();
#endif
#if TRANSIT_LOOPDETECT
            TASK(TASK_ID_LOOPBACK_CHECK, ldettsk());
            ldet_aging_100ms();
#endif
#if TRANSIT_EEE
            TASK(TASK_ID_EEE, eee_mgmt());
#endif
        }

        /*
         * Do 1 sec jobs
         */

        if (sec_1_timeout_flag) {
            sec_1_timeout_flag = 0;

#if FRONT_LED_PRESENT
            led_1s_timer();
#endif

#if TRANSIT_THERMAL
            phy_handle_temperature_protect();
            phy_temperature_timer_1sec ();
#endif

#if TRANSIT_LLDP
            TASK(TASK_ID_LLDP_TIMER, lldp_1sec_timer_tick());
            TASK(TASK_ID_TIMER_SINCE_BOOT, time_since_boot_update());
#endif /* TRANSIT_LLDP */

            /*
             * Check H2 and PHYs
             */
            TASK(TASK_ID_ERROR_CHECK, error_check());

#if TRANSIT_EEE
            callback_delayed_eee_lpi();
#endif /* TRANSIT_EEE */

#if TRANSIT_FAN_CONTROL
            TASK(TASK_ID_FAN_CONTROL, fan_control());
#endif
            /* toggle any alive LED */
            ALIVE_LED_TOGGLE;

        }

        /* For profiling/debug purposes */
        MAIN_LOOP_EXIT();

        /*
         * Sleep until next interrupt
         * Make sure to keep it as the last command
         */
        PCON = 0x1;
    }
}

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
