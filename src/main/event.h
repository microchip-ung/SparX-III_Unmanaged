//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#ifndef __EVENT_H__
#define __EVENT_H__

#include "common.h"

void  callback_delayed_eee_lpi             (void);
void  callback_link_up                     (vtss_port_no_t port_no);
void  callback_link_down                   (vtss_port_no_t port_no);

#endif /* __EVENT_H__ */

