//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



/*****************************************************************************
 *                                                                          *
 *  Mapping of PHY MIIM, port no, and so on.                                *
 *                                                                          *
 ****************************************************************************/

#ifndef __PHYMAP_H__
#define __PHYMAP_H__

uchar phy_map_coma_mode_disable (vtss_port_no_t port_no) small;
uchar phy_map_miim_no           (vtss_port_no_t port_no) small;
uchar phy_map_phy_no            (vtss_port_no_t port_no) small;
bool  phy_map                   (vtss_port_no_t port_no) small;
uchar phy_map_serdes            (vtss_port_no_t port_no) small;
void  phy_map_serdes_if_update  (vtss_port_no_t port_no, uchar mac_if) small;

#endif

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
