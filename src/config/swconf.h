//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT

/****************************************************************************
 *                                                                          *
 *  Software configuration options                                          *
 *                                                                          *
 ****************************************************************************/

#ifndef __SWCONF_H__
#define __SWCONF_H__

#ifndef __INCLUDE_CONFIGS__
#error "swconf.h is for common.h only"
#endif

/*
 * Pick your own protocols and features.
 */

/****************************************************************************
 * SNMP
 ****************************************************************************/
#define TRANSIT_SNMP                        0


/****************************************************************************
 * LLDP - IEEE802.1AB
 ****************************************************************************/
#define TRANSIT_LLDP                        1
#define TRANSIT_LLDP_REDUCED                1


/****************************************************************************
 * EEE
 ****************************************************************************/
#define TRANSIT_EEE                         1
#define TRANSIT_EEE_LLDP                    1 /* Define this if EEE shall
                                                 support changes by LLDP */
#if TRANSIT_EEE

#if !TRANSIT_LLDP
/* EEE depends on LLDP */
#error "TRANSIT_EEE depends on TRANSIT_LLDP, please set in swconf.h"
#endif

#endif /* TRANSIT_EEE */


/****************************************************************************
 * VeriPHY
 ****************************************************************************/
#define TRANSIT_VERIPHY                     1

/****************************************************************************
 * ActiPHY
 ****************************************************************************/
#define TRANSIT_ACTIPHY                     0

/****************************************************************************
 * Perfect Reach
 ****************************************************************************/
/* Set PERFECT_REACH mode at compile time */
#define PERFECT_REACH_LNK_UP                1
#define PERFECT_REACH_LNK_DN                1

#if PERFECT_REACH_LNK_UP && PERFECT_REACH_LNK_DN
/*
 * User can configure Perfect Reach at runtime
 */
#define PERFECT_REACH_MANAGEMENT            1
#else
#define PERFECT_REACH_MANAGEMENT            0
#endif /* PERFECT_REACH_LNK_UP && PERFECT_REACH_LNK_DN */


/****************************************************************************
 * Enable/Disable Loop Detection / Protection
 ****************************************************************************/
#define TRANSIT_LOOPDETECT                  1


/****************************************************************************
 * Allow/Disable BPDU pass through the switch.
 ****************************************************************************/
#define TRANSIT_BPDU_PASS_THROUGH           0


/****************************************************************************
 *
 *
 * Debug Features
 *
 *
 ****************************************************************************/


/****************************************************************************
 * FTIME
 ****************************************************************************/
#ifndef VTSS_FEATURE_FTIME
/**
 * Set VTSS_FEATURE_FTIME to 1 to enable system uptime tracking and provide
 * an API ftime() to get the uptime of the system.
 *
 * @note ftime is a 4.2BSD, POSIX.1-2001 API.
 */
#define VTSS_FEATURE_FTIME                  0
#endif


/****************************************************************************
 * Debug - Disable asserts and trace: use #undef to enable asserts and trace.
 ****************************************************************************/
#define NDEBUG                              1


/****************************************************************************
 * Disable Debug Interfaces - UART, Web upgrade
 ****************************************************************************/
#if 0
#define NO_DEBUG_IF
#endif

#ifdef NO_DEBUG_IF
#undef  UNMANAGED_REDUCED_DEBUG_IF
#define UNMANAGED_LLDP_DEBUG_IF             0
#define UNMANAGED_EEE_DEBUG_IF              0
#define UNMANAGED_FAN_DEBUG_IF              0
#define UNMANAGED_TRANSIT_VERIPHY_DEBUG_IF  0
#define UNMANAGED_PORT_STATISTICS_IF        0
#else
/* In case debug is supported, users are free to define:
 * UNMANAGED_REDUCED_DEBUG_IF -- define if only limited debug i/f is expected
 * LUTON_UNMANAGED_SWUP -- set if firmware upgrade is expected
 */
//#define UNMANAGED_REDUCED_DEBUG_IF
#define UNMANAGED_LLDP_DEBUG_IF             0
#define UNMANAGED_EEE_DEBUG_IF              0
#define UNMANAGED_FAN_DEBUG_IF              0
#define UNMANAGED_TRANSIT_VERIPHY_DEBUG_IF  0
#define UNMANAGED_PORT_STATISTICS_IF        0
#endif


#define LUTON_UNMANAGED_SWUP                1
#define LUTON_UNMANAGED_CONF_IF             1
/****************************************************************************
 *
 *
 * Obsolete Features
 *
 *
 ****************************************************************************/


/****************************************************************************
 * FAN S/W Control
 ****************************************************************************/
#define  TRANSIT_FAN_CONTROL                0 /* FAN specifications is defined
                                                 in fan_custom_api.h */


/****************************************************************************
 * Thermal Control - This is de-speced from VSC7420, VSC7421, VSC7422
 ****************************************************************************/
#define  TRANSIT_THERMAL                    0


/****************************************************************************
 * PoE - Not supported right now.
 ****************************************************************************/
#define TRANSIT_POE                         0
#define TRANSIT_POE_LLDP                    0

#if TRANSIT_POE_LLDP
#if (TRANSIT_LLDP == 0)
#error "Please set TRANSIT_LLDP 1 in swconf.h"
#endif
#endif /* TRANSIT_POE_LLDP */


#endif /* __SWCONF_H__ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
