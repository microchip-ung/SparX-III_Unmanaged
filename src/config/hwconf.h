//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT


/****************************************************************************
 *                                                                          *
 *  Hardware configuration options                                          *
 *                                                                          *
 ****************************************************************************/

#ifndef __HWCONF_H__
#define __HWCONF_H__

#ifndef __INCLUDE_CONFIGS__
#error "hwconf.h is for common.h only"
#endif

#include <REG52.H>

/*****************************************************************************
 *
 *
 * Internal 8051 registers
 *
 *
 *
 ****************************************************************************/

sfr GPR       = 0x80;
sfr PAGE_SEL  = 0xB0;
sfr EPAGE_SEL = 0xC0;
sfr MMAP      = 0xF2;
sfr RA_AD0_RD = 0xF6;
sfr RA_AD0_WR = 0xF7;
sfr RA_AD1    = 0xF9;
sfr RA_AD2    = 0xFA;
sfr RA_AD3    = 0xFB;
sfr RA_DA0    = 0xFC;
sfr RA_DA1    = 0xFD;
sfr RA_DA2    = 0xFE;
sfr RA_DA3    = 0xFF;

sbit GPR0     = 0x80;
sbit GPR1     = 0x81;
sbit GPR2     = 0x82;
sbit GPR3     = 0x83;
sbit GPR4     = 0x84;
sbit GPR5     = 0x85;
sbit GPR6     = 0x86;
sbit GPR7     = 0x87;

/*****************************************************************************
 *
 *
 * Select chip interface
 *
 *
 ****************************************************************************/

/*
 * Define which chip interface to use, either 8-bit parallel interface,
 * SI interface or SFR interface.
 * Set USE_PI to 1 to use 8-bit interface, otherwise set it to 0.
 * Set USE_SI to 1 to use SI interface, otherwise set it to 0.
 * Set USE_SFR to 1 to use SFR interface, otherwise set it to 0.
 * Please, keep assembler file hwconf.inc updated accordingly.
 */

#define USE_PI  0
#define USE_SI  0
#define USE_SFR 1

/*****************************************************************************
 *
 *
 * MCU clock frequency
 *
 *
 ****************************************************************************/

#define CLOCK_FREQ          250000000

/*****************************************************************************
 *
 *
 * Serial port baud rate
 *
 *
 ****************************************************************************/

/*
 * Baud rate at RS232 interface.
 * Choose value among 115200, 57600, 38400, 19200 and 9600.
 */

#define BAUD_RATE 9600


/*****************************************************************************
 *
 *
 * GPIOs
 *
 *
 *
 ****************************************************************************/

#define USE_HW_TWI 0
#define USE_SW_TWI 1

/*
 * I2C control signals
 */

#define GPIO_5_BIT          5UL
#define GPIO_6_BIT          6UL

#define GPIO_OE_5_BIT       5UL
#define GPIO_OE_6_BIT       6UL

#define SDA_BIT             GPIO_6_BIT
#define CLK_BIT             GPIO_5_BIT

#define SDA_BIT_OE          GPIO_OE_6_BIT
#define CLK_BIT_OE          GPIO_OE_5_BIT


/*****************************************************************************
 *
 *
 * Watchdog
 *
 *
 ****************************************************************************/

/*
 * Define whether watchdog is present and whether it should be enabled.
 * Set WATCHDOG_PRESENT to 1, if a watchdog is present and you want to use it,
 *  otherwise set it to 0
 * Set WATCHDOG_ENABLE to 1, if watchdog should be running all the time, otherwise
 * set it to 0 (in which case it will only be used in case of software reboot).
 */
#define WATCHDOG_PRESENT    0
#define WATCHDOG_ENABLE     0

/*
 * Define macros to enable and kick watchdog, see hwport.c.
 */


/*****************************************************************************
 *
 *
 * Control bits for paging
 *
 *
 ****************************************************************************/



/*****************************************************************************
 *
 *
 * Select presence of PHY loopback test
 *
 *
 ****************************************************************************/

/*
 * Define whether PHY loopback test should be supported
 * Set LOOPBACK_TEST to 1, if loopback test should be supported, otherwise set
 * it to 0
 */
#define LOOPBACK_TEST   0

/*****************************************************************************
 *
 *
 * Enable/disable switch chip id check
 *
 *
 ****************************************************************************/

/**
 * Define whether switch chip id should be checked as run-time check.
 * Set H2_ID_CHECK to 1 to enable check, or to 0 to disable check.
 */
#define H2_ID_CHECK 0

/*****************************************************************************
 *
 *
 * Configure any alive/debug LED
 *
 *
 *
 ****************************************************************************/

/*
** Define macro for turning on debug/alive LED. If no LED, define
** an empty macro.
*/
#define ALIVE_LED_ON {}

/*
** Define macro for toggling debug/alive LED. If no LED, define
** an empty macro.
*/
#define ALIVE_LED_TOGGLE {}

/*****************************************************************************
 *
 *
 * Configure any front panel LED
 *
 *
 *
 ****************************************************************************/

/*
** Define whether a red/green front LED is present.
** Set FRONT_LED_PRESENT to 1, if present, otherwise set it to 0
*/
#define FRONT_LED_PRESENT  1

/*
** Define LED mode.
*/
#define LED_MODE_OFF 0
#define LED_MODE_ON  1
#define LED_LINK_ACTIVE_MODE 6


/*****************************************************************************
 *
 *
 * Enable/disable jumbo frame support
 *
 *
 *
 ****************************************************************************/

/*
** Define whether to enable jumbo frames.
** Set JUMBO to 1 to enable jumbo frames, 0 to disable jumbo frames.
*/
#define JUMBO 1

/*
** Define size of jumbo frames, provided JUMBO is set to 1.
*/
#define JUMBO_SIZE 9600



/****************************************************************************
 *
 *
 * VSC7421 (Luton26 L16) MUX Mode
 *
 *
 ****************************************************************************/
#if defined(LUTON26_L16)

#define LUTON26_L16_MUX_MODE    (1) /* Available options: 0 or 1 */

#if LUTON26_L16_MUX_MODE == 0
/**
 * VSC7421 QSGMII mode with external 4 port PHY.
 *
 * eg. In current code base, VSC8514 (ELISE) is supported.
 *     Turne on ELISE in phyconf.h and this option, then
 *     L16 of Luton26 can support 16 port copper.
 */
#define LUTON26_L16_QSGMII_EXT_PHY
#endif

#endif /* defined(LUTON26_L16) */


/****************************************************************************
 *
 *
 * SFP Ports Support Functionality Configuration.
 *
 *
 ****************************************************************************/

/* Define if on some ports, MAC to SerDes directly */
#if !defined(LUTON26_L10) && !defined(LUTON26_L16)
#define MAC_TO_MEDIA                        1
#define TRANSIT_SFP_DETECT                  1
#elif !defined(LUTON26_L10)
#define MAC_TO_MEDIA                        0
#define TRANSIT_SFP_DETECT                  0
#else
#define MAC_TO_MEDIA                        1
#define TRANSIT_SFP_DETECT                  0
#endif


/*****************************************************************************
 *
 *
 * Port mapping
 *
 *
 ****************************************************************************/

#if !defined(LUTON26_L10) && !defined(LUTON26_L16)
/*
** Define mapping between Luton25 port numbers and logical port numbers.
** For each Luton25 port specify the logical port number and for each
** logical port specify the front port number.

Luton25 port number:
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25
  -------------------------------------------------------------------------- */
#define PORT_MAP_2_EXT  \
  1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24, 0,25

/* Logic port number:
  1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25
  ---------------------------------------------------------------------- */
#define PORT_MAP_2_INT \
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,25, 0

#elif !defined(LUTON26_L10)
/*
** Define mapping between Luton16 port numbers and logical port numbers.
** For each Luton16 port specify the logical port number and for each
** logical port specify the front port number.
** Option 0
Luton16 port number:
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,xx,xx,xx,xx,16,xx,xx,19,xx,xx,xx,xx,24,25
  -------------------------------------------------------------------------- */
#ifdef LUTON26_L16_QSGMII_EXT_PHY
#define PORT_MAP_2_EXT  \
  1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#else
#define PORT_MAP_2_EXT  \
  1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12, 0, 0, 0, 0,13, 0, 0,14, 0, 0, 0, 0,15,16
#endif /* LUTON26_L16_QSGMII_EXT_PHY */

/* Logic port number:
  1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16
  ---------------------------------------------- */
#ifdef LUTON26_L16_QSGMII_EXT_PHY
#define PORT_MAP_2_INT \
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15
#else
#define PORT_MAP_2_INT \
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,16,19,24,25
#endif /* LUTON26_L16_QSGMII_EXT_PHY */

#else
/*
** Define mapping between Luton10 port numbers and logical port numbers.
** For each Luton10 port specify the logical port number and for each
** logical port specify the front port number.

Luton10 port number:
  0, 1, 2, 3, 4, 5, 6, 7, x, x,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,24,25
  -------------------------------------------------------------------------- */
#define PORT_MAP_2_EXT  \
  1, 2, 3, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9,10

/* Logic port number:
  1, 2, 3, 4, 5, 6, 7, 8, 9,10
  ---------------------------------------------- */
#define PORT_MAP_2_INT \
  0, 1, 2, 3, 4, 5, 6, 7,24,25
#endif


#endif /* __HWCONF_H__ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
