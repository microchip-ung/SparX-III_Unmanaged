//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include "common.h"     /* Always include common.h at the first place of user-defined herder files */

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

/*
** Mapping of PHYs on ports. Define for each port, the MIIM (0 or 1) to which
** the PHY is connected, and the PHY number.
*/
static code uchar port_to_phy              [LUTON26_PORTS] = PHY_MAP_PHY_NO;
static      uchar port_to_miim             [LUTON26_PORTS] = PHY_MAP_MIIM_NO;
static code uchar port_to_coma_mode_disable[LUTON26_PORTS] = PHY_MAP_COMA_MODE_DISABLE;


/* ************************************************************************ */
uchar phy_map_miim_no (vtss_port_no_t port_no) small
/* ------------------------------------------------------------------------ --
 * Purpose     : Get MIIM number of the PHY attached to specified port.
 * Remarks     : Returns MIIM number.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    return port_to_miim[port_no];
}

#if 0
uchar phy_map_coma_mode_disable (vtss_port_no_t port_no) small
/* ------------------------------------------------------------------------ --
 * Purpose     : Get if PHY coma mode shall be isabled. Coma mode shall be
 *               disabled when the last PHY port has been initialized.
 *               For this to work it is assumed that the ports are initialized
 *               begin from the lowest phy port to the highest phy port.
 * Remarks     : Returns if coma mode shall be disabled..
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    return port_to_coma_mode_disable[port_no];
}
#endif

/* ************************************************************************ */
uchar phy_map_phy_no (vtss_port_no_t port_no) small
/* ------------------------------------------------------------------------ --
 * Purpose     : Get PHY number of the PHY attached to specified port.
 * Remarks     : Returns PHY number.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    return port_to_phy[port_no];
}



/* ************************************************************************ */
bool phy_map (vtss_port_no_t port_no) small
/* ------------------------------------------------------------------------ --
 * Purpose     : Check if a PHY is attached to an MIIM for the specified port.
 * Remarks     : Returns TRUE if so, otherwise FALSE
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    return (port_to_miim[port_no] <= 1);
}


#if MAC_TO_MEDIA
/* ************************************************************************ */
uchar phy_map_serdes(vtss_port_no_t port_no)	small
/* ------------------------------------------------------------------------ --
 * Purpose     : Check if the PHY attached to specified port is a Serdes port.
 * Remarks     : Returns TRUE if Serdes port, otherwise FALSE
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    return (port_to_miim[port_no] == 9 ||
						port_to_miim[port_no] == 8 ||
						port_to_miim[port_no] == 2 ||
						port_to_miim[port_no] == 0xa);
}

#if TRANSIT_SFP_DETECT
void phy_map_serdes_if_update(vtss_port_no_t port_no, uchar mac_if) small
/* ------------------------------------------------------------------------ --
 * Purpose     : update if the SPF attached to specified port is a Serdes port.
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    port_to_miim[port_no] = mac_if;
}
#endif
#endif

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
