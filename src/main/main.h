//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#ifndef __MAIN_H__
#define __MAIN_H__

#if UNMANAGED_PORT_STATISTICS_IF
extern bit      start_display_statis_bit;
extern uchar    port_cnt;
#endif

/*
 * Power up error codes.
 */
#define POWER_UP_ERROR_FLASH_CHECKSUM 4
#define POWER_UP_ERROR_PHY            5
#define POWER_UP_ERROR_H2             6

/*
 * Run-time error codes.
 */
#define ERROR_RX_BUF_OVERRUN          1
#define ERROR_UART_FRAMING            3
#define ERROR_FATAL                   7

/*
 * Error ids
 */
#define MIIM_FAILURE          2
#define ARBITER_EMPTY_FAILURE 3
#define MACTAB_FAILURE        4
#define H2_GENERAL_FAILURE    5
#define PHY_GENERAL_FAILURE   6
#define SYSTEM_INIT_FAILURE   7
#define VLANTAB_FAILURE       8

#endif

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
