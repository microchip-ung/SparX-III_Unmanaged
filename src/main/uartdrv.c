//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include "common.h"     /* Always include common.h at the first place of user-defined herder files */
#include "vtss_luton26_regs.h"
#include "h2io.h"
#include "uartdrv.h"
#include "timer.h"
#include "misc2.h"

#pragma NOAREGS

#ifndef NO_DEBUG_IF

/*****************************************************************************
 *
 *
 * Public data
 *
 *
 *
 ****************************************************************************/

bit uart_rx_buf_overrun = FALSE;

/*****************************************************************************
 *
 *
 * Defines
 *
 *
 *
 ****************************************************************************/
#define BAUD_RATE_DIVISOR     \
    (ushort) ((float)(CLOCK_FREQ / (32 * (float) BAUD_RATE)) + 0.5)


/* size of receive buffer. Must be a power of 2, i.e. 2, 4, 8 ... 256 */
#define RX_BUF_SIZE 32


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

static uchar idata rx_buf [RX_BUF_SIZE];

#if RX_BUF_SIZE > 256
static ushort data rx_head  = 0;
static ushort data rx_tail  = 0;
#else
static uchar data rx_head  = 0;
static uchar data rx_tail  = 0;
#endif /* RX_BUF_SIZE > 256 */


static data uchar xdata *redir_ptr = 0;

/* ************************************************************************ */
void uart_init (void) small
/* ------------------------------------------------------------------------ --
 * Purpose     : Enable TX and RX on UART.
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    /* Configure GPIO for UART */
    h2_write_masked(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(0), VTSS_GPIO_UART_MASK, VTSS_GPIO_UART_MASK);

    /* Disable interrupts of UART */

    /* Wait for transmit FIFO empty before reseting */

    /* Setup line control for 8-n-1 and enable divisor access */
    h2_write_masked(VTSS_UART_UART_LCR, VTSS_F_UART_UART_LCR_DLAB | VTSS_F_UART_UART_LCR_DLS(3), \
                    VTSS_F_UART_UART_LCR_DLAB | VTSS_F_UART_UART_LCR_DLS(3));

    /* Set divisor */
    h2_write(VTSS_UART_UART_RBR_THR, LOW_BYTE(BAUD_RATE_DIVISOR));
    h2_write(VTSS_UART_UART_IER, HIGH_BYTE(BAUD_RATE_DIVISOR));
    h2_write_masked(VTSS_UART_UART_LCR, 0, VTSS_F_UART_UART_LCR_DLAB);

    /* Enable FIFO */
    h2_write_masked(VTSS_UART_UART_IIR_FCR, VTSS_F_UART_UART_IIR_FCR_XFIFOR | \
                    VTSS_F_UART_UART_IIR_FCR_RFIFOR | VTSS_F_UART_UART_IIR_FCR_FIFOE, \
                    VTSS_F_UART_UART_IIR_FCR_XFIFOR | \
                    VTSS_F_UART_UART_IIR_FCR_RFIFOR | VTSS_F_UART_UART_IIR_FCR_FIFOE);

    /* Enable appropriate interrupts of UART */
    h2_write_masked(VTSS_UART_UART_IER, VTSS_F_UART_UART_IER_ERBFI, VTSS_F_UART_UART_IER_ERBFI);

    /* Setup interrupt controller to enable UART interrupting and the propagate destination */
    h2_write_masked(VTSS_ICPU_CFG_INTR_UART_INTR_CFG,
                    VTSS_F_ICPU_CFG_INTR_UART_INTR_CFG_UART_INTR_SEL(1),
                    VTSS_M_ICPU_CFG_INTR_UART_INTR_CFG_UART_INTR_SEL);

    h2_write_masked(VTSS_ICPU_CFG_INTR_INTR_ENA, VTSS_F_ICPU_CFG_INTR_INTR_ENA_UART_INTR_ENA, \
                    VTSS_F_ICPU_CFG_INTR_INTR_ENA_UART_INTR_ENA);
}


/* ************************************************************************ */
void uart_interrupt (void) small
/* ------------------------------------------------------------------------ --
 * Purpose     : Interrupt function. Save received char in buffer.
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uchar ch;
    ulong temp;

    //const ulong code reg_uart_iir_fcr = VTSS_UART_UART_IIR_FCR;
    //const ulong code reg_uart_rbr_thr = VTSS_UART_UART_RBR_THR;
    //const ulong code reg_uart_lsr     = VTSS_UART_UART_LSR;

    //const ulong code lsr_dr_mask      = VTSS_F_UART_UART_LSR_DR;
    //const ulong code iir_int_id_mask  = IIR_INTERRUPT_ID_MASK;

    EA = 0;
    //temp = h2_read(reg_uart_iir_fcr);
    temp = h2_read(VTSS_UART_UART_IIR_FCR);
    temp &= IIR_INTERRUPT_ID_MASK; //iir_int_id_mask;

    if ((temp == IIR_RX_AVAIL) || (temp == IIR_RX_CHAR_TIMEOUT)) {
        do {
            //temp = h2_read(reg_uart_rbr_thr);
            temp = h2_read(VTSS_UART_UART_RBR_THR);
            ch = (uchar) (temp & 0xFF);
            /* save received char if not buffer overrun */
            if ((rx_head + 1) == rx_tail) {
                uart_rx_buf_overrun = TRUE;
            } else {
                rx_buf[rx_head] = ch;
                rx_head = (rx_head + 1) & (RX_BUF_SIZE - 1);
            }
            //temp = h2_read(reg_uart_lsr);
            temp = h2_read(VTSS_UART_UART_LSR);
            //} while (temp & lsr_dr_mask);
        } while (test_bit_32(0, &temp));
    }
    EA = 1;

}

/* ************************************************************************ */
bool uart_byte_ready (void) small
/* ------------------------------------------------------------------------ --
 * Purpose     : Check if any char received.
 * Remarks     : Returns TRUE, if char ready, otherwise FALSE.
 * Restrictions:
 * See also    : uart_get_byte
 * Example     :
 ****************************************************************************/
{
    return (rx_head != rx_tail);
}

/* ************************************************************************ */
uchar uart_get_byte (void) small
/* ------------------------------------------------------------------------ --
 * Purpose     : Get received char.
 * Remarks     :
 * Restrictions: May only be called if uart_byte_ready has returned TRUE.
 * See also    : uart_byte_ready
 * Example     :
 ****************************************************************************/
{
    uchar tmp;

    tmp = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) & (RX_BUF_SIZE - 1);
    return tmp;
}

void uart_put_byte (uchar ch) small

{
    ulong lsr;
    //const ulong code lsr_temp_mask    = VTSS_F_UART_UART_LSR_TEMT;
    //const ulong code reg_uart_lsr     = VTSS_UART_UART_LSR;
    //const ulong code reg_uart_rbr_thr = VTSS_UART_UART_RBR_THR;

    if(redir_ptr != 0) {
        *redir_ptr++ = ch;
        return;
    }

    start_timer(MSEC_20);
    do {
        /* Wait TX FIFO be empty */
        //H2_READ(reg_uart_lsr, lsr);
        H2_READ(VTSS_UART_UART_LSR, lsr);
    } while(!test_bit_32(6, &lsr) && !timeout());

    //H2_WRITE(reg_uart_rbr_thr, ch);
    H2_WRITE(VTSS_UART_UART_RBR_THR, ch);

}

/* ************************************************************************ */
#if UNMANAGED_LLDP_DEBUG_IF || !TRANSIT_LLDP_REDUCED
void uart_redirect (uchar xdata *ptr) small
/* ------------------------------------------------------------------------ --
 * Purpose     : Request uart driver to redirect output to the specified
 *               pointer. Used for the WEB interface.
 * Remarks     : If ptr = 0, then stop redirect output. When different from
 *               0, the caller must ensure that the buffer is large enough.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    if (ptr == 0) {
        *redir_ptr = '\0';
    }
    redir_ptr = ptr;
}
#endif

#endif /* NO_DEBUG_IF */
