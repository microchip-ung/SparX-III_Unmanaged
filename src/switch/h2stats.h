//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#if !defined(__H2STATS_H__)
#define __H2STATS_H__

/****************************************************************************
 * Defines
 *
 *
 ************************************************************************** */

/****************************************************************************
 * Typedefs and enums
 *
 *
 ************************************************************************** */
typedef enum _port_statistics_t {
    /* TABLE Lookup counters */

    CNT_RX_OCTETS           =  0x0,
    CNT_RX_PKTS             =  0x1,
    CNT_RX_MCAST_PKTS       =  0x2,
    CNT_RX_BCAST_PKTS       =  0x3,
    CNT_RX_UNDERSIZE_PKTS   =  0x4,
    CNT_RX_FRAGMENTS        =  0x5,
    CNT_RX_JABBERS          =  0x6,
    CNT_RX_CRC_ALIGN_ERRS   =  0x7,
    CNT_RX_64               =  0x8,
    CNT_RX_65_TO_127        =  0x9,
    CNT_RX_128_TO_255       =  0xa,
    CNT_RX_256_TO_511       =  0xb,
    CNT_RX_512_TO_1023      =  0xc,
    CNT_RX_1024_TO_1526     =  0xd,
    CNT_RX_OVERSIZE_PKTS    =  0xe,
    CNT_RX_PAUSE            =  0xf,
    CNT_RX_CTRL             = 0x10,
    CNT_RX_LONG             = 0x11,
    CNT_RX_CAT_DROP         = 0x12,

    CNT_TX_OCTETS           = 0x800,
    CNT_TX_PKTS             = 0x801,
    CNT_TX_MCAST_PKTS       = 0x802,
    CNT_TX_BCAST_PKTS       = 0x803,
    CNT_TX_COLLISIONS       = 0x804,
    CNT_TX_DROP             = 0x805,
    CNT_TX_PAUSE            = 0x806,
    CNT_TX_64               = 0x807,
    CNT_TX_65_TO_127        = 0x808,
    CNT_TX_128_TO_255       = 0x809,
    CNT_TX_256_TO_511       = 0x80a,
    CNT_TX_512_TO_1023      = 0x80b,
    CNT_TX_1024_TO_1526     = 0x80c,
    CNT_TX_OVERSIZE_PKTS    = 0x80d,
    CNT_TX_AGED             = 0x81e,

    CNT_DROP_LOCAL          = 0xc00,
    CNT_DROP_TAIL           = 0xc01,
    
    /* Combined Counters */
    CNT_RX_UCAST_PKTS       = 0x7000,
    CNT_RX_NON_UCAST_PKTS   = 0x7001,
    CNT_RX_DISCARD_PKTS     = 0x7002,
    CNT_RX_ERR_PKTS         = 0x7003,
    CNT_RX_UNKNOWN_PROTOS   = 0x7004,
    CNT_RX_DROPEVENTS       = 0x7005,
    
    CNT_TX_UCAST_PKTS       = 0x7100,
    CNT_TX_NON_UCAST_PKTS   = 0x7101,
    CNT_TX_DISCARDS         = 0x7102,
    CNT_TX_ERR_PKTS         = 0x7103,
    

    COUNTER_NONE            = 0xffff,
} port_statistics_t;
uchar h2_stats_counter_exists (port_statistics_t counter_id);
ulong h2_stats_counter_get (uchar port_no, port_statistics_t counter_id);
void  print_port_statistics (uchar port_no);
#endif





