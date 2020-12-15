//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT


#ifndef __H2PCS1G_H__
#define __H2PCS1G_H__

#if  MAC_TO_MEDIA

typedef enum {
    MAC_IF_INTERNAL = 0,
    MAC_IF_EXTERNAL = 1,
    MAC_IF_SGMII  = 2,
    MAC_IF_100FX  = 8,
    MAC_IF_SERDES = 9,
    MAC_IF_SFP    = 0xa,
    MAC_IF_NONE = 0xff
} mac_if_type_t;

void  h2_pcs1g_clause_37_control_set(const uchar port_no);
uchar h2_pcs1g_clause_37_status_get(const uchar port_no);
uchar h2_pcs1g_100fx_status_get(const uchar port_no);
void  h2_pcs1g_clock_stop (uchar port_no);
void  h2_pcs1g_setup (uchar port_no, uchar mode);
#endif
#endif
