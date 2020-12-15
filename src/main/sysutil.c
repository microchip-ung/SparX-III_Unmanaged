//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include "common.h"     /* Always include common.h at the first place of user-defined herder files */
#include "h2io.h"
#include "vtss_common_os.h"
#include "vtss_luton26_reg.h"
#include "h2mactab.h"
#include "sysutil.h"

/*****************************************************************************
 *
 *
 * Macros
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

static BOOL     g_suspend_enable    = FALSE;

static BOOL     g_h2_fatal_error    = FALSE;

/*****************************************************************************
 *
 *
 * Local Functions
 *
 *
 ****************************************************************************/


/*****************************************************************************
 *
 *
 * Public Functions
 *
 *
 ****************************************************************************/


/**
 * Reboot by forcing watchdog reset or utilizing any other reset feature.
 */
void sysutil_reboot (void)
{
    MMAP = 0xAF;
    H2_WRITE(VTSS_DEVCPU_GCB_DEVCPU_RST_REGS_SOFT_CHIP_RST,
             VTSS_F_DEVCPU_GCB_DEVCPU_RST_REGS_SOFT_CHIP_RST_SOFT_CHIP_RST);
    while (1) ;
}

/**
 * Hang forever with front LED blinking red.
 */
void sysutil_hang (void)
{
    uchar status;

    status = 1;
    while (TRUE) {
        /* luton26 hang function */
    }
}

/**
 * Indicate error and initiate error recovery.
 */
void sysutil_set_fatal_error (uchar error_id)
{
    error_id = error_id; /* keep compiler happy */

    /* luton26 error code handler */
    g_h2_fatal_error = TRUE;
}

BOOL sysutil_get_fatal_error (void)
{
    return (g_h2_fatal_error) ? 1 : 0;
}

void sysutil_set_suspend(BOOL enable)
{
    g_suspend_enable = enable;
}

BOOL sysutil_get_suspend(void)
{
    return g_suspend_enable;
}

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
