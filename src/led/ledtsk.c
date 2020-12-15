//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include "common.h"     /* Always include common.h at the first place of user-defined herder files */
#include "vtss_luton26_regs.h"
#include "h2gpios.h"
#include "h2io.h"
#include "hwport.h"
#include "phytsk.h"
#include "main.h"
#include "phydrv.h"
#include "h2stats.h"
#include "ledtsk.h"
#include "string.h"

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

#if FRONT_LED_PRESENT

uchar port_led_mode;
uchar led_mode_timer;
static uchar is_refresh_led = 0;
static ulong col_cnt[NO_OF_PORTS];
static char led_state[NO_OF_PORTS][VTSS_LED_EVENT_END];
static char led_final_state[NO_OF_PORTS];


static void led_set (uchar port_sio, vtss_sgpio_bit_t output_bit, vtss_sgpio_mode_t mode)
{
    h2_sgpio_write(port_sio, output_bit, mode);
    h2_sgpio_write(port_sio, output_bit == VTSS_SGPIO_BIT_0? 1:0, 0x0);
}

#if TRANSIT_VERIPHY
uchar sgpio_output (uchar port_sio, vtss_sgpio_bit_t output_bit, vtss_sgpio_state_t state)
{
    if (state == VTSS_SGPIO_STATE_ON)
        /* LED_MODE_ON, LED_MODE_OFF defined in hwconf.h */
        h2_sgpio_write(port_sio, output_bit, LED_MODE_ON);
    else
        h2_sgpio_write(port_sio, output_bit, LED_MODE_OFF);

    return 	 0;
}
#endif

void led_mode (uchar mode)
/* ------------------------------------------------------------------------ --
 * Purpose     : Setup mode LED mode
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    switch (mode) {
#if defined(LUTON26_L25)
        /*
         *  LED tower
         *  (top)       o  mode A (link/speed)      sgpio port 26
         *              o  mode B (link/duplex)     sgpio port 27
         *              o  mode C (link/status)     sgpio port 28
         *  (button)    o  PWR save                 sgpio port 29
         */
    case VTSS_LED_MODE_LINK_SPEED:
        led_set(28, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(29, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(27, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(26, VTSS_SGPIO_BIT_1, LED_MODE_ON);  /* Green */
        break;
    case VTSS_LED_MODE_DUPLEX:
        led_set(27, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(29, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(26, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(27, VTSS_SGPIO_BIT_1, LED_MODE_ON);  /* Green */
        break;
    case VTSS_LED_MODE_POWER_SAVE:
        led_set(27, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(28, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(26, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(29, VTSS_SGPIO_BIT_1, LED_MODE_ON);  /* Green */
        break;
    case VTSS_LED_MODE_LINK_STATUS:
        /* Mode C not supported in the unmanaged solution */
#if 0
        led_set(27, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(28, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(26, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(28, VTSS_SGPIO_BIT_1, LED_MODE_ON);  /* Green */
        break;
#endif
#elif defined(LUTON26_L10)
        /*
         *  LED tower
         *  (top)       o  mode A (link/speed)      sgpio port 20
         *              o  mode B (link/duplex)     sgpio port 21
         *              o  mode C (link/status)     sgpio port 22
         *  (button)    o  PWR save                 sgpio port 23
         */
    case VTSS_LED_MODE_LINK_SPEED:
        led_set(21, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(22, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(23, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(20, VTSS_SGPIO_BIT_0, LED_MODE_ON);  /* Green */

        break;
    case VTSS_LED_MODE_DUPLEX:
        led_set(20, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(22, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(23, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(21, VTSS_SGPIO_BIT_0, LED_MODE_ON);  /* Green */
        break;
    case VTSS_LED_MODE_POWER_SAVE:
        led_set(21, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(22, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(20, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(23, VTSS_SGPIO_BIT_0, LED_MODE_ON);  /* Green */
        break;
    case VTSS_LED_MODE_LINK_STATUS:
        /* Mode C not supported in the unmanaged solution */
#else
        /*
         *  LED tower
         *  (top)       o  mode A (link/speed)      sgpio port 26
         *              o  mode B (link/duplex)     sgpio port 27
         *              o  mode C (link/status)     sgpio port 28
         *  (button)    o  PWR save                 sgpio port 29
         */
    case VTSS_LED_MODE_LINK_SPEED:
        led_set(28, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(29, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(27, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(26, VTSS_SGPIO_BIT_1, LED_MODE_ON);  /* Green */
        break;
    case VTSS_LED_MODE_DUPLEX:
        led_set(27, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(29, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(26, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(27, VTSS_SGPIO_BIT_1, LED_MODE_ON);  /* Green */
        break;
    case VTSS_LED_MODE_POWER_SAVE:
        led_set(27, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(28, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(26, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(29, VTSS_SGPIO_BIT_1, LED_MODE_ON);  /* Green */
        break;
    case VTSS_LED_MODE_LINK_STATUS:
        /* Mode C not supported in the unmanaged solution */
#if 0
        led_set(27, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(28, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(26, VTSS_SGPIO_BIT_0, LED_MODE_OFF); /* Off */
        led_set(28, VTSS_SGPIO_BIT_1, LED_MODE_ON);  /* Green */
        break;
#endif
#endif
    default:
        break;
    }
}

/* ************************************************************************ */
void led_tsk (void)
/* ------------------------------------------------------------------------ --
 * Purpose     : Detects button and setup port led accordingly
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    /* GPIO bit 16 is for push button */
    if (h2_gpio_read(16)) {
        port_led_mode = (++port_led_mode) % VTSS_LED_MODE_END;

        /* Mode C not supported in the unmanaged solution */
        if (port_led_mode == VTSS_LED_MODE_LINK_STATUS) {
            port_led_mode++;
        }
        led_mode_timer = LED_MODE_DEFAULT_TIME;
    } else if(led_mode_timer == 0) {
        port_led_mode = VTSS_LED_MODE_POWER_SAVE;

    }

    led_mode(port_led_mode);
    led_port(port_led_mode);

#if 0
    if (is_refresh_led && port_led_mode != VTSS_LED_MODE_POWER_SAVE) {
        /* Update front LEDs */
        led_port(port_led_mode);
    }

    is_refresh_led = 0;
#endif
    /*
    **  Fixme:
    **  1) Setup other led/sgpio outputs, such as LED tower
    **  and sgpio port 26-31 for general purpose in the function
    **  or a new function
    **  2) To implement a callback function to update port led
    **  output for link change (up/down) James : done
    **  3) To implement polling for switch counters to detect
    **  collisions at HDX for VTSS_LED_MODE_DUPLEX mode and detect
    **  errors for VTSS_LED_MODE_LINK_STATUS mode.
    **  4) Possibily create API for sgpio read. (Note: to read data
    **  in you need to issue two bursts, one for LD and one for
    **  clocking in)
    */
}

/* ************************************************************************ */
static BOOL port_collision (uchar port_no)
/* ------------------------------------------------------------------------ --
 * Purpose     : Check if collision occured
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    ulong collision_cnt;
    uchar i_port_no;

    i_port_no = port2int(port_no + 1);
    collision_cnt = h2_stats_counter_get(i_port_no, CNT_TX_COLLISIONS);

    if (col_cnt[port_no] != collision_cnt) {
        col_cnt[port_no] = collision_cnt;
        return TRUE;
    }

    return FALSE;
}

/* ************************************************************************ */
void led_port (uchar mode)
/* ------------------------------------------------------------------------ --
 * Purpose     : Setup port LED mode
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uchar port_no, i_port_no;
    uchar link_mode;

    for (port_no = 0; port_no < NO_OF_PORTS; port_no++) {
        if (led_final_state[port_no] != VTSS_LED_MODE_NORMAL) {
            /* at least one of error events occurs, just show the error status */
            continue;
        }
        i_port_no = port2int(port_no + 1);
        link_mode = phy_get_link_mode_raw(i_port_no);
        switch (mode) {
        case VTSS_LED_MODE_POWER_SAVE:
            /* Force off no matter link is up or not */
            led_set(i_port_no, VTSS_SGPIO_BIT_0, LED_MODE_OFF);
            break;

        case VTSS_LED_MODE_LINK_SPEED:
            /* Link/activity; Green for 1G and Yellow for 10/100 */
            if (link_mode == LINK_MODE_DOWN) {
                /* Link down */
                led_set(i_port_no, VTSS_SGPIO_BIT_0, LED_MODE_OFF);
            } else if ((link_mode & LINK_MODE_SPEED_MASK) == LINK_MODE_SPEED_1000) {
                /* Green: 1G link/activity */
                led_set(i_port_no, VTSS_SGPIO_BIT_1, LED_LINK_ACTIVE_MODE);
            } else {
                /* Yellow: 100/10 link/activity */
                led_set(i_port_no, VTSS_SGPIO_BIT_0, LED_LINK_ACTIVE_MODE);
            }
            break;

        case VTSS_LED_MODE_DUPLEX:
            /* Duplex mode; Green for FDX and Yellow for HDX */
            if (link_mode == LINK_MODE_DOWN) {
                /* Link down */
                led_set(i_port_no, VTSS_SGPIO_BIT_0, LED_MODE_OFF);
            } else if (link_mode & LINK_MODE_FDX_MASK) {
                /* Green: FDX */
                led_set(i_port_no, VTSS_SGPIO_BIT_1, LED_MODE_ON);
            } else {
                if (port_collision(port_no)) {
                    /* collision, blinking LED - Yellow/Blink: HDX */
                    led_set(i_port_no, VTSS_SGPIO_BIT_0, VTSS_SGPIO_MODE_BL_0);
                } else {
                    /* no collision, turn on LED - Yellow/On: HDX */
                    led_set(i_port_no, VTSS_SGPIO_BIT_0, LED_MODE_ON);
                }
            }
            break;

        case VTSS_LED_MODE_LINK_STATUS: /* This mode is not supported */
            /* Green for link/activity; Yellow: Port disabled */
            if (link_mode == LINK_MODE_DOWN) {
                /* Link down */
                led_set(i_port_no, VTSS_SGPIO_BIT_0, LED_MODE_OFF);
            } else if (link_mode & LINK_MODE_SPEED_MASK) {
                /* Green: Link/activity */
                led_set(i_port_no, VTSS_SGPIO_BIT_0, LED_LINK_ACTIVE_MODE);
            } else {
                /* Yellow: Port disabled */
                led_set(i_port_no, VTSS_SGPIO_BIT_1, LED_MODE_ON);
            }
            break;

        default:
            break;
        }
    }
}

void led_refresh (void)
{
    is_refresh_led = 1;
}


/* ************************************************************************ */
void led_init (void)
/* ------------------------------------------------------------------------ --
 * Purpose     : SGPIO controller setup based on board
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    phy_page_std(15);
    phy_write_masked(15, 29, 0x00f0, 0x00f0);

    /*  Reset Collision counters */
    memset(led_state, 0, sizeof(led_state));

    led_mode_timer = LED_MODE_DEFAULT_TIME;

    /* Light status LED green */
    led_status(VTSS_LED_MODE_ON_GREEN);

    /* Light tower LED */
    led_mode(VTSS_LED_MODE_LINK_SPEED);

    /* Light port LED */
    led_port(port_led_mode);
}

/* ************************************************************************ */
void led_state_set (uchar port_no, vtss_led_event_type_t event, vtss_led_mode_type_t state)
/* ------------------------------------------------------------------------ --
 * Purpose     : Set the LED state for error event
 * Remarks     : The status set here overcomes the normal LED display
 *               event
 *                  VTSS_LED_EVENT_LOOP,
 *                  VTSS_LED_EVENT_OVERHEAT,
 *                  VTSS_LED_EVENT_CABLE
 *               state
 *                  VTSS_LED_MODE_NORMAL,    (priority low)
 *                  VTSS_LED_MODE_OFF,
 *                  VTSS_LED_MODE_ON_GREEN,
 *                  VTSS_LED_MODE_ON_YELLOW,
 *                  VTSS_LED_MODE_BLINK_GREEN,   at 10Hz
 *                  VTSS_LED_MODE_BLINK_YELLOW   at 10Hz (priority high)
 *
 *              The highest priority state will show when any conflict
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uchar   led_event;
    uchar   i_port_no;
    uchar   tmp_led_state;


    led_state[port_no-1][event] = state;

    /* Select the highest priority event to show */
    tmp_led_state = led_state[port_no-1][0];
    for (led_event = 1; led_event < VTSS_LED_EVENT_END; led_event++) {
        if (tmp_led_state < led_state[port_no-1][led_event])
            tmp_led_state =  led_state[port_no-1][led_event];
    }

    led_final_state[port_no-1] =  tmp_led_state;
    i_port_no = port2int(port_no);

    switch (led_final_state[port_no-1]) {
    case VTSS_LED_MODE_NORMAL:
        /* error event gone and back to normal LED display */
        led_set(i_port_no, VTSS_SGPIO_BIT_1, LED_MODE_OFF);
        led_refresh(); /* trigger to update normal LED at the next second */
        break;
    case VTSS_LED_MODE_OFF:
        led_set(i_port_no, VTSS_SGPIO_BIT_1, LED_MODE_OFF);
        break;
    case VTSS_LED_MODE_ON_GREEN:
        led_set(i_port_no, VTSS_SGPIO_BIT_1 /*ssss*/, LED_MODE_ON);
        break;
    case VTSS_LED_MODE_ON_YELLOW:
        led_set(i_port_no, VTSS_SGPIO_BIT_0, LED_MODE_ON);
        break;
    case VTSS_LED_MODE_BLINK_GREEN:
        led_set(i_port_no, VTSS_SGPIO_BIT_1, VTSS_SGPIO_MODE_BL_1);
        break;
    case VTSS_LED_MODE_BLINK_YELLOW:
        led_set(i_port_no, VTSS_SGPIO_BIT_0, VTSS_SGPIO_MODE_BL_1);
        break;
    default:
        break;
    }
}
#if 0
/* ************************************************************************ */
void sgpio_auto_burst (uchar enable)
/* ------------------------------------------------------------------------ --
 * Purpose     : Enable/disable repeated bursts
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    H2_WRITE_MASKED(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG, enable ? 0x20 : 0x0,
                    VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_AUTO_REPEAT);
}
#endif

uchar led_status (vtss_led_mode_type_t mode)
{
    switch(mode) {
#if defined(LUTON26_L25)
    case VTSS_LED_MODE_ON_GREEN:
        led_set(30, VTSS_SGPIO_BIT_1, LED_MODE_ON);
        break;
    case VTSS_LED_MODE_ON_RED:
        led_set(30, VTSS_SGPIO_BIT_0, LED_MODE_ON);
        break;
    case VTSS_LED_MODE_BLINK_GREEN:
        led_set(30, VTSS_SGPIO_BIT_1, VTSS_SGPIO_MODE_BL_1);
        break;
    case VTSS_LED_MODE_BLINK_RED:
        led_set(30, VTSS_SGPIO_BIT_0, VTSS_SGPIO_MODE_BL_0);
        break;
#else
    case VTSS_LED_MODE_ON_GREEN:
        led_set(26, VTSS_SGPIO_BIT_0, LED_MODE_ON);
        break;
    case VTSS_LED_MODE_ON_RED:
        led_set(26, VTSS_SGPIO_BIT_1, LED_MODE_ON);
        break;
    case VTSS_LED_MODE_BLINK_GREEN:
        led_set(26, VTSS_SGPIO_BIT_0, VTSS_SGPIO_MODE_BL_1);
        break;
    case VTSS_LED_MODE_BLINK_RED:
        led_set(26, VTSS_SGPIO_BIT_1, VTSS_SGPIO_MODE_BL_0);
        break;
#endif
    default :
        return 1;
    }

    return 0;
}

void led_1s_timer (void)
{
    if (led_mode_timer != 0)
        led_mode_timer--;
}

#endif

