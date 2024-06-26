//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT


#ifndef _VTSS_LUTON26_REGS_DEV_CMN_H_
#define _VTSS_LUTON26_REGS_DEV_CMN_H_

#include "vtss_luton26_regs_dev_gmii.h"
#include "vtss_luton26_regs_dev.h"

/*
 * Abstraction macros for functionally identical registers 
 * in the DEV and DEV_GMII targets.
 *
 * Caution: These macros may not work a lvalues, depending
 * on compiler and platform.
 *
 */

#define VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_MODE_CFG(t) ( (t) >= VTSS_TO_DEV_10 ? VTSS_DEV_MAC_CFG_STATUS_MAC_MODE_CFG(t) : VTSS_DEV_GMII_MAC_CFG_STATUS_MAC_MODE_CFG(t) )

#define VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_IFG_CFG(t) ( (t) >= VTSS_TO_DEV_10 ? VTSS_DEV_MAC_CFG_STATUS_MAC_IFG_CFG(t) : VTSS_DEV_GMII_MAC_CFG_STATUS_MAC_IFG_CFG(t) )

#define VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_ADV_CHK_CFG(t) ( (t) >= VTSS_TO_DEV_10 ? VTSS_DEV_MAC_CFG_STATUS_MAC_ADV_CHK_CFG(t) : VTSS_DEV_GMII_MAC_CFG_STATUS_MAC_ADV_CHK_CFG(t) )

#define VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_TAGS_CFG(t) ( (t) >= VTSS_TO_DEV_10 ? VTSS_DEV_MAC_CFG_STATUS_MAC_TAGS_CFG(t) : VTSS_DEV_GMII_MAC_CFG_STATUS_MAC_TAGS_CFG(t) )

#define VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_HDX_CFG(t) ( (t) >= VTSS_TO_DEV_10 ? VTSS_DEV_MAC_CFG_STATUS_MAC_HDX_CFG(t) : VTSS_DEV_GMII_MAC_CFG_STATUS_MAC_HDX_CFG(t) )

#define VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_ENA_CFG(t) ( (t) >= VTSS_TO_DEV_10 ? VTSS_DEV_MAC_CFG_STATUS_MAC_ENA_CFG(t) : VTSS_DEV_GMII_MAC_CFG_STATUS_MAC_ENA_CFG(t) )

#define VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_FC_CFG(t) ( (t) >= VTSS_TO_DEV_10 ? VTSS_DEV_MAC_CFG_STATUS_MAC_FC_CFG(t) : VTSS_DEV_GMII_MAC_CFG_STATUS_MAC_FC_CFG(t) )

#define VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_FC_MAC_HIGH_CFG(t) ( (t) >= VTSS_TO_DEV_10 ? VTSS_DEV_MAC_CFG_STATUS_MAC_FC_MAC_HIGH_CFG(t) : VTSS_DEV_GMII_MAC_CFG_STATUS_MAC_FC_MAC_HIGH_CFG(t) )

#define VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_MAXLEN_CFG(t) ( (t) >= VTSS_TO_DEV_10 ? VTSS_DEV_MAC_CFG_STATUS_MAC_MAXLEN_CFG(t) : VTSS_DEV_GMII_MAC_CFG_STATUS_MAC_MAXLEN_CFG(t) )

#define VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_STICKY(t) ( (t) >= VTSS_TO_DEV_10 ? VTSS_DEV_MAC_CFG_STATUS_MAC_STICKY(t) : VTSS_DEV_GMII_MAC_CFG_STATUS_MAC_STICKY(t) )

#define VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_FC_MAC_LOW_CFG(t) ( (t) >= VTSS_TO_DEV_10 ? VTSS_DEV_MAC_CFG_STATUS_MAC_FC_MAC_LOW_CFG(t) : VTSS_DEV_GMII_MAC_CFG_STATUS_MAC_FC_MAC_LOW_CFG(t) )


#endif /* _VTSS_LUTON26_REGS_DEV_CMN_H_ */
