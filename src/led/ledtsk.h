//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT


#ifndef __LEDTSK_H__
#define __LEDTSK_H__

#include "common.h"     /* Always include common.h at the first place of user-defined herder files */

#define VTSS_GPIO_SIO_MASK      0xF
#define LED_MODE_DEFAULT_TIME   30

typedef enum {
    VTSS_SGPIO_MODE_OFF,        /**< Forced '0' */
    VTSS_SGPIO_MODE_ON,         /**< Forced '1' */
    VTSS_SGPIO_MODE_BL_0,       /**< Blink mode 0 */
    VTSS_SGPIO_MODE_BL_1,   	/**< Blink mode 1 */
    VTSS_SGPIO_MODE_LACT_0,     /**< Link activity blink mode 0 */
    VTSS_SGPIO_MODE_LACT_1,   	/**< Link activity blink mode 1 */
    VTSS_SGPIO_MODE_LACT_0_REV, /**< Link activity blink mode 0 inversed polarity */
    VTSS_SGPIO_MODE_LACT_1_REV, /**< Link activity blink mode 1 inversed polarity */
    VTSS_SGPIO_MODE_END
} vtss_sgpio_mode_t;

enum {
    VTSS_LED_MODE_LINK_SPEED, 	/**< Green: 1G link/activity; Orange: 10/100 link/activity */
    VTSS_LED_MODE_DUPLEX,       /**< Green: FDX; Orange: HDX + Collisions */
    VTSS_LED_MODE_LINK_STATUS,  /**< Green: Link/activity; Orange: port disabled/errors */
    VTSS_LED_MODE_POWER_SAVE, 	/**< Disabled to save power */
    VTSS_LED_MODE_END
};

typedef enum {
    VTSS_LED_MODE_NORMAL,           /* Normal mode */
    VTSS_LED_MODE_OFF,              /* OFF mode */
    VTSS_LED_MODE_ON_GREEN,         /* ON (green) mode */
    VTSS_LED_MODE_ON_YELLOW,        /* ON (yellow) mode */
    VTSS_LED_MODE_ON_RED,           /* ON (red) mode */
    VTSS_LED_MODE_BLINK_GREEN, 	    /* BLINK (green) mode */
    VTSS_LED_MODE_BLINK_YELLOW, 	/* BLINK (yellow) mode */
    VTSS_LED_MODE_BLINK_RED, 	    /* BLINK (red) mode */
    VTSS_LED_MODE_TYPE_END
} vtss_led_mode_type_t;

typedef enum {
    VTSS_LED_EVENT_LOOP = 0,
    VTSS_LED_EVENT_OVERHEAT,
    VTSS_LED_EVENT_CABLE,

    /* Add new events before here */
    VTSS_LED_EVENT_END
} vtss_led_event_type_t;

typedef enum {
    VTSS_SGPIO_BIT_0,
    VTSS_SGPIO_BIT_1,
    VTSS_SGPIO_BIT_2,
    VTSS_SGPIO_BIT_3,
    VTSS_SGPIO_BIT_END
} vtss_sgpio_bit_t;

typedef enum {
    VTSS_SGPIO_STATE_ON,
    VTSS_SGPIO_STATE_OFF,
    VTSS_SGPIO_STATE_END
} vtss_sgpio_state_t;

#if FRONT_LED_PRESENT
extern uchar port_led_mode;
extern uchar led_mode_timer;
#endif

void  led_init      (void);
void  led_tsk       (void);
void  led_port      (uchar mode);
uchar led_status    (uchar mode);
void  led_1s_timer  (void);
void  led_refresh   (void);
void  led_state_set (uchar port_no, vtss_led_event_type_t event, vtss_led_mode_type_t state);
uchar sgpio_output  (uchar port_sio, vtss_sgpio_bit_t output_bit, vtss_sgpio_state_t state);

#endif /* __LEDTSK_H__ */
