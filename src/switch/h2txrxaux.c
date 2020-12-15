//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include "common.h"     /* Always include common.h at the first place of user-defined herder files */
#include "vtss_luton26_regs.h"
#include "h2io.h"
#include "timer.h"
#include "h2packet.h"
#include "h2txrxaux.h"
#include "misc2.h"
#include "misc3.h"
#include "hwport.h"
#include <string.h>
#include "spiflash.h"
#if TRANSIT_LOOPDETECT
#include "loopdet.h"
#endif

#define XTR_EOF_0     0x80000000UL
#define XTR_EOF_1     0x80000001UL
#define XTR_EOF_2     0x80000002UL
#define XTR_EOF_3     0x80000003UL
#define XTR_PRUNED    0x80000004UL
#define XTR_ABORT     0x80000005UL
#define XTR_ESCAPE    0x80000006UL
#define XTR_NOT_READY 0x80000007UL



static ulong rx_word (uchar qno);
static void  tx_word (uchar qno, ulong value);
static bool  fifo_status(uchar qno);

static void h2_rx_frame_discard(const uchar qno)
{
    uchar done = FALSE;

    while(!done) {
        ulong val;
        val = rx_word(qno);
        switch(val) {
        case XTR_ABORT:
        case XTR_PRUNED:
        case XTR_EOF_3:
        case XTR_EOF_2:
        case XTR_EOF_1:
        case XTR_EOF_0:
            val = rx_word(qno); /* Last data */
            done = TRUE;        /* Last 1-4 bytes */
            break;
        case XTR_ESCAPE:
            val = rx_word(qno); /* Escaped data */
            break;
        case XTR_NOT_READY:
        default:
            ;
        }
    }
}

void h2_rx_frame_get (const uchar qno, vtss_rx_frame_t xdata * rx_frame_ptr)
/* ------------------------------------------------------------------------ --
 * Purpose     : Receive frame.
 * Remarks     : The structure pointed to by rx_frame_ptr is updated with the
 *               data received, see h2packet.h for a description of the structure.
 * Restrictions: Only to be called if h2_frame_received has returned TRUE.
 * See also    :
 * Example     :
 ****************************************************************************/
{
    ulong  qstat;
    ulong  ifh0, ifh1;
    ulong  xdata *packet;
    uchar  eof_flag, escape_flag, abort_flag, pruned_flag;

#define MAX_LENGTH (RECV_BUFSIZE+2)

    vtss_eth_hdr xdata * eth_hdr = rx_frame_ptr->rx_packet;
    mac_addr_t self_mac_addr;

    get_mac_addr(SYSTEM_MAC_ADDR, self_mac_addr);

    h2_discard_frame(rx_frame_ptr);

    H2_READ(VTSS_DEVCPU_QS_XTR_XTR_DATA_PRESENT, qstat);

    if(test_bit_32(qno, &qstat)) {
        ifh0 = rx_word(qno);
        ifh1 = rx_word(qno);

        memset(&rx_frame_ptr->header, 0, sizeof(vtss_packet_rx_header_t));
        rx_frame_ptr->header.port = IFH_GET(ifh0, ifh1, PORT);
        rx_frame_ptr->header.vid  = IFH_GET(ifh0, ifh1, VID);

        packet = (ulong *) rx_frame_ptr->rx_packet;

        eof_flag = 0;
        escape_flag = 0;
        abort_flag = 0;
        pruned_flag = 0;

        while(!eof_flag) {
            *packet = rx_word(qno);
            switch(*packet) {
            case XTR_NOT_READY:
                break;              /* Try again... */
            case XTR_ABORT:
                *packet = rx_word(qno); /* Unused */
                abort_flag = 1;
                eof_flag = TRUE;
                break;
            case XTR_EOF_3:
                *packet = rx_word(qno);
                rx_frame_ptr->total_bytes += 1;
                eof_flag = TRUE;
                break;
            case XTR_EOF_2:
                *packet = rx_word(qno);
                rx_frame_ptr->total_bytes += 2;
                eof_flag = TRUE;
                break;
            case XTR_EOF_1:
                *packet = rx_word(qno);
                rx_frame_ptr->total_bytes += 3;
                eof_flag = TRUE;
                break;
            case XTR_PRUNED:
                pruned_flag = 1; /* But get the last 4 bytes as well */
                /* FALLTHROUGH */
            case XTR_EOF_0:
                eof_flag = TRUE;
                /* FALLTHROUGH */
            case XTR_ESCAPE:
                *packet = rx_word(qno);
                /* FALLTHROUGH */
            default:
                if(rx_frame_ptr->total_bytes == 12) {

                    if((eth_hdr->dest.addr[0] == 0x01) &&
                            (eth_hdr->dest.addr[1] == 0x80) &&
                            (eth_hdr->dest.addr[2] == 0xc2) &&
                            (eth_hdr->dest.addr[3] == 0x00) &&
                            (eth_hdr->dest.addr[4] == 0x00)) {
#if TRANSIT_LLDP
                        if(eth_hdr->dest.addr[5] != 0x0e) { // LLDP frame
                            h2_rx_frame_discard(qno);
                            abort_flag = 1;
                            goto discard_packet;
                        }
#endif
                    }

                    if(mac_cmp(eth_hdr->src.addr, self_mac_addr) == 0) {
#if TRANSIT_LOOPDETECT
#if LOOPBACK_TEST
                        if (eth_hdr->type != 0x8809)
#endif
                            ldet_add_cpu_found(rx_frame_ptr->header.port);  //Local loop was found on port
#endif
                    }


                }
                rx_frame_ptr->total_bytes += 4;
                packet++;
            }
        }

        if(eof_flag) {
            rx_frame_ptr->discard = 0;
        }

        if(pruned_flag) {
            rx_frame_ptr->pruned = 1;
        }

discard_packet:
        if(abort_flag || !eof_flag) {
            h2_discard_frame(rx_frame_ptr);
            return;
        }
    }
}

bool h2_tx_frame_port(const uchar port_no,
                      const uchar *const frame,
                      const ushort length,
                      const vtss_vid_t vid)
/* ------------------------------------------------------------------------ --
 * Purpose     : Send a frame on the specified port.
 * Remarks     : port_no specifies the Heathrow transmit port.
 *               frame_ptr points to the frame data and frame_len specifies
 *               the length of frame data in number of bytes.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    ulong ifh0 = 0, ifh1 = 0;
    port_bit_mask_t  dest_port_mask = 0;
    vtss_vid_t tx_vid = vid;

    const ulong *bufptr = (ulong *) frame;
    ulong        buflen = length, count, w, last, val;

    uchar qno;

#if 0
    WRITE_PORT_BIT_MASK(port_no, 1, &dest_port_mask);

    /* Calculate and send IFH0 and IFH1 */
    /* In order for IFH-DEST-field, then set the BYPASS. */
    IFH_PUT(ifh0, ifh1, BYPASS, 1);
    IFH_PUT(ifh0, ifh1, DEST, dest_port_mask);
#endif

    ifh0 = VTSS_ENCODE_BITFIELD(1, 63 - 32, 1) | /* BYPASS */
           VTSS_ENCODE_BITFIELD(1, port_no, 1);  /* DEST */

    ifh1 = VTSS_ENCODE_BITFIELD(3, 28, 2); /* POP_CNT=3 disables rewriter */

    /* Select a tx queue */
    for(qno = VTSS_PACKET_TX_QUEUE_START; qno < VTSS_PACKET_TX_QUEUE_END; qno++) {
        if(fifo_status(qno))
            break;
    }
    if(qno == VTSS_PACKET_TX_QUEUE_END) return FALSE; // No tx queue available.

    H2_WRITE(VTSS_DEVCPU_QS_INJ_INJ_CTRL(qno), VTSS_F_DEVCPU_QS_INJ_INJ_CTRL_SOF);
    tx_word(qno, ifh0);
    tx_word(qno, ifh1);

    count = buflen / 4;
    last  = buflen % 4;

    w = 0;
    while(count) {
        if (w == 3 && vid != VTSS_VID_NULL) {
            /* Insert C-tag */
            val = ushorts2ulong(0x8100, vid);
            tx_word(qno, val);
            w++;
        }
        val = *bufptr;
        tx_word(qno, val);
        bufptr++;
        count--;
        w++;
    }

    if(last) {
        val = *bufptr;
        tx_word(qno, val);
        w++;
    }

    /* Add padding */
    while (w < 15 /*(60/4)*/ ) {
        tx_word(qno, 0);
        w++;
    }

    /* Indicate EOF and valid bytes in last word */
    H2_WRITE(VTSS_DEVCPU_QS_INJ_INJ_CTRL(qno),
             VTSS_F_DEVCPU_QS_INJ_INJ_CTRL_VLD_BYTES(length < 60 ? 0 : last) |
             VTSS_F_DEVCPU_QS_INJ_INJ_CTRL_EOF);

    /* Add dummy CRC */
    tx_word(qno, 0);

    return TRUE;
}

void h2_discard_frame( vtss_rx_frame_t xdata * rx_frame_ptr)
{
    rx_frame_ptr->total_bytes = 0;
    rx_frame_ptr->discard = 1;
    rx_frame_ptr->pruned = 0;
}

static void tx_word (uchar qno, ulong value)
/* ------------------------------------------------------------------------ --
 * Purpose     : Write a 32-bit chunk to transmit fifo.
 * Remarks     :
 * Restrictions: See h2_tx_init
 * See also    :
 * Example     :
 ****************************************************************************/
{
    H2_WRITE(VTSS_DEVCPU_QS_INJ_INJ_WR(qno), value);
}


static ulong rx_word (const uchar qno)
{
    ulong value;
    H2_READ(VTSS_DEVCPU_QS_XTR_XTR_RD(qno), value);
    return value;
}

static bool fifo_status(uchar qno)
{
    ulong qstat;
    bool  wmark;
    bool  ready;

    H2_READ(VTSS_DEVCPU_QS_INJ_INJ_STATUS, qstat);

    wmark = test_bit_32((qno+4), &qstat);
    ready = test_bit_32((qno+2), &qstat);

    return (!wmark & ready);
}
