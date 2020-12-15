//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#ifndef __H2_H__
#define __H2_H__

#include "common.h"

/****************************************************************************
 *
 *
 * Defines
 *
 *
 ****************************************************************************/


/*
 * This clause is dirty, but is a must when debugging hard-to-find
 * vtss_update_masks() issues.
 */

#define VTSS_UPDATE_MASKS_DEBUG()


/****************************************************************************
 *
 *
 * Functions
 *
 *
 ****************************************************************************/

void                h2_reset                (void) small;
void                h2_post_reset           (void);
void                h2_init_ports           (void);
void                h2_setup_mac            (uchar port_no, link_mode);
void                h2_setup_port           (uchar port_no, uchar link_mode);

uchar               h2_check                (void) small;
void                h2_enable_exc_col_drop  (uchar port_no, uchar drop_enable);
/**
 * Update source, destination and aggregation masks
 */
void                vtss_update_masks       (void);

#endif /* __H2_H__ */

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
