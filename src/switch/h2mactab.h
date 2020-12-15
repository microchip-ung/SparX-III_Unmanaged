//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT


#ifndef __H2MACTAB_H__
#define __H2MACTAB_H__

void   h2_mactab_agetime_set (void);
void   h2_mactab_flush_port (uchar port_no);
void   h2_mactab_age (uchar pgid_age, uchar pgid, uchar vid_age, ushort vid);
void   h2_mactab_clear (void);

#endif







