//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#ifndef __SYSUTIL_H__
#define __SYSUTIL_H__

#include "common.h"

void    sysutil_reboot                          (void);
void    sysutil_hang                            (void);

void    sysutil_set_fatal_error                 (uchar error_id);
BOOL    sysutil_get_fatal_error                 (void);

/**
 * Set suspend flag.
 *
 * @see sysutil_get_suspend().
 */
void    sysutil_set_suspend                     (BOOL enable);

/**
 * Return suspend flag.
 *
 * @see sysutil_set_suspend().
 */
BOOL    sysutil_get_suspend                     (void);

#endif /* __SYSUTIL_H__ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
