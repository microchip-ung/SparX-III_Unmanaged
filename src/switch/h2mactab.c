//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include "common.h"     /* Always include common.h at the first place of user-defined herder files */

#include "vtss_luton26_reg.h"
#include "h2mactab.h"
#include "h2io.h"
#include "timer.h"
#include "main.h"

/*****************************************************************************
 *
 *
 * Defines
 *
 *
 *
 ****************************************************************************/


/* MAC table commands */
#define MAC_TAB_IDLE   0
#define MAC_TAB_LEARN  1
#define MAC_TAB_FORGET 2
#define MAC_TAB_AGE    3
#define MAC_TAB_GET_NX 4
#define MAC_TAB_CLEAR  5
#define MAC_TAB_READ   6
#define MAC_TAB_WRITE  7


/* Define period for performing ageing, granularity is 1 second */
#define AGEING_TIMEOUT 300


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

static void do_mactab_cmd (ulong mac_access_reg_val);

/*****************************************************************************
 *
 *
 * Local data
 *
 *
 *
 ****************************************************************************/

/* ************************************************************************ */
void h2_mactab_agetime_set (void)
/* ------------------------------------------------------------------------ --
 * Purpose     : Normal auto aging in seconds
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
#define age_time AGEING_TIMEOUT

    H2_WRITE(VTSS_ANA_ANA_AUTOAGE,
             VTSS_F_ANA_ANA_AUTOAGE_AGE_PERIOD((ulong)(age_time/2)));
}

/* ************************************************************************ */
void h2_mactab_age (uchar pgid_age, uchar pgid, uchar vid_age, ushort vid)
/* ------------------------------------------------------------------------ --
 * Purpose     : Perform ageing operation on MAC address table.
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    /* Selective aging */
    H2_WRITE(VTSS_ANA_ANA_ANAGEFIL,
           (pgid_age ? VTSS_F_ANA_ANA_ANAGEFIL_PID_EN : 0) |
           (vid_age ? VTSS_F_ANA_ANA_ANAGEFIL_VID_EN  : 0) |
           VTSS_F_ANA_ANA_ANAGEFIL_PID_VAL(pgid) |
           VTSS_F_ANA_ANA_ANAGEFIL_VID_VAL(vid));

    /* Do the aging */
    do_mactab_cmd(VTSS_F_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(MAC_CMD_TABLE_AGE));

    /* Clear age filter again to avoid affecting automatic ageing */
    H2_WRITE(VTSS_ANA_ANA_ANAGEFIL, 0);
}

/* ************************************************************************ */
void h2_mactab_clear (void)
/* ------------------------------------------------------------------------ --
 * Purpose     : Perform clear operation on MAC address table.
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    do_mactab_cmd(MAC_CMD_TABLE_CLEAR);
}


void h2_mactab_flush_port (uchar port_no)
{
    // age twice means flush
    h2_mactab_age(1, port_no, 0, 0);
    h2_mactab_age(1, port_no, 0, 0);
}

/*****************************************************************************
 *
 *
 * Support functions
 *
 *
 *
 ****************************************************************************/
static void do_mactab_cmd (ulong mac_access_reg_val)
{
    ulong cmd;
    H2_WRITE(VTSS_ANA_ANA_TABLES_MACACCESS,  mac_access_reg_val);
    do {
        H2_READ(VTSS_ANA_ANA_TABLES_MACACCESS, cmd);
    } while (VTSS_X_ANA_ANA_TABLES_MACACCESS_MAC_TABLE_CMD(cmd) != MAC_CMD_IDLE);
}

