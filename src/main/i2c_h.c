//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT


//==========================================================================
//
//      I2C driver for Microchip Switch Chip
//
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):
// Contributors:  James Lin
// Date:          2010-08-04
// Description:   I2C driver for Microchip Switch chip
//####DESCRIPTIONEND####
//==========================================================================

#include "common.h"     /* Always include common.h at the first place of user-defined herder files */
#if USE_HW_TWI
#include "vtss_luton26_regs.h"
#include "vtss_luton26_regs_twi.h"
#include "h2io.h"
#include "hwport.h"
#include "main.h"
#include "timer.h"
#include "print.h"
#include "i2c_h.h"

#define I2C_DEBUG 0

// Since the tar register only must be changed when the controller is disabled
// we have a special function for setting the tar register.
void i2c_set_tar_register(const ushort i2c_addr)
{   //James
    ulong value, value1;

    // Check if the TAR register needs to be changed.
    H2_READ(VTSS_TWI_TWI_TAR, value);
#if I2C_DEBUG
    print_str("VTSS_TWI_TWI_TAR");
    print_dec(value);
    print_cr_lf();
#endif

	if (i2c_addr != (value & 0x3FF))
    {
#if I2C_DEBUG
        println_str("i2c_addr != ");
#endif
        // Check that the hardware fifos are empty ( They are flushed when the controller is disabled )
        H2_READ(VTSS_TWI_TWI_RXFLR, value);
        H2_READ(VTSS_TWI_TWI_TXFLR, value1);

        if (value != 0 || value1 != 0) {
            println_str("I2C - Hardware FIFOs will be flushed");
        }

        // Disable the I2C controller because TAR register cannot be changed when the controller is enabled.
        H2_WRITE(VTSS_TWI_TWI_CTRL, 0x0);

        // Set Target address
        H2_WRITE(VTSS_TWI_TWI_TAR, i2c_addr);

        // Enable the I2C controller
        H2_WRITE(VTSS_TWI_TWI_CTRL, VTSS_F_TWI_TWI_CTRL_ENABLE);
    }
}

// Function for waiting until the TX buffer is empty . This is needed for making sure that
// multiple I2C transactions aren't concatenated into one transaction at the I2C bus.
//
// If the buffer isn't emptied within 500 ms the function returns 0 else 1
static int wait_for_tx_buffer_empty(void)
{ //James

   ulong value;

   // Check if TX fifo is empty. If not wait until it is.
   start_timer(MSEC_500);

    do {
        H2_READ(VTSS_TWI_TWI_TXFLR, value);
        if (timeout()) {
            // Fifo was not emptied within 500 ms.
            println_str("TX buffer empty Timeout\n");
            return 0;
        }
    } while (value != 0);

    return 1;
}

// Function for transmitting the data - returns 0 upon timeout failure.
static int tx_i2c_data(const ulong tx_data)
{ //James
    ulong value;

    start_timer(MSEC_50);

    // Check if TX fifo is full. If so, wait until there is room for one more byte
    do {
        H2_READ(VTSS_TWI_TWI_TXFLR, value);
        if (timeout()) {
            // Fifo was not emptied within 50 ms.
            println_str("I2C Tx Timeout");
            return 0;
        }
    } while (value > 6);


    H2_WRITE(VTSS_TWI_TWI_DATA_CMD, tx_data);
    return 1;
}

// ----------------------------------------------------------------------------
// The functions needed for all I2C devices.

void i2c_init(void)
{ //James
    ulong register_value;

    //
    // The bus frequency is set by the platform.
    //
    // Set clock speed for standard speed
    ulong clk_freq = 250000000; /* 250 Mhz */
#if I2C_DEBUG
    print_str("clk_freq = ");
    print_dec(clk_freq);
    print_cr_lf();
#endif

    // Setting high clock flank to 5 us (for standard clk (100 kHz))
    register_value = (5 * (clk_freq / 1000000)) - 8; // Datasheet Section 6.13.9.5
    H2_WRITE(VTSS_TWI_TWI_SS_SCL_HCNT, register_value);
#if I2C_DEBUG
    print_str("VTSS_TWI_TWI_SS_SCL_HCNT = ");
    print_dec(register_value);
    print_cr_lf();
#endif

    // Setting low clock flank to 5 us (for standard clk (100 kHz))
    register_value = (5 * (clk_freq / 1000000)) - 1; // Datasheet Section 6.13.9.6
    H2_WRITE(VTSS_TWI_TWI_SS_SCL_LCNT,register_value);
#if I2C_DEBUG
    print_str("VTSS_TWI_TWI_SS_SCL_LCNT = ");
    print_dec(register_value);
    print_cr_lf();
#endif
    // Setting TWI_DELAY to 300ns
    register_value = VTSS_F_ICPU_CFG_TWI_DELAY_TWI_CONFIG_TWI_CNT_RELOAD((unsigned int)(0.3 * (clk_freq / 1000000)) - 1) |
                     VTSS_F_ICPU_CFG_TWI_DELAY_TWI_CONFIG_TWI_DELAY_ENABLE; // Datasheet Section 6.8.3
    H2_WRITE(VTSS_ICPU_CFG_TWI_DELAY_TWI_CONFIG, register_value);
#if I2C_DEBUG
    print_str("VTSS_ICPU_CFG_TWI_DELAY_TWI_CONFIG = 0x");
    print_hex_dw(register_value);
    print_cr_lf();
#endif

    // Setting high clock flak to 1.1 us (for fast clock (400 kHz)) (Asym. because VTSS_TWI_FS_SCL_LCNT mustn't be below 1.3 us).
    register_value = (1.1 * clk_freq / 1000000) - 8; // Datasheet Section 6.13.9.7
    H2_WRITE(VTSS_TWI_TWI_FS_SCL_HCNT, register_value);
#if I2C_DEBUG
    print_str("VTSS_TWI_TWI_FS_SCL_HCNT = ");
    print_dec(register_value);
    print_cr_lf();
#endif

    // Setting low clock flak to 1.4 us ( for fast clock (400 kHz)) ( Asym. because VTSS_TWI_FS_SCL_LCNT mustn't be below 1.3 us).
    register_value = (1.4 * clk_freq / 1000000) - 1 ;// Datasheet Section 6.13.9.8
    H2_WRITE(VTSS_TWI_TWI_FS_SCL_LCNT,register_value);
#if I2C_DEBUG
    print_str("VTSS_TWI_TWI_FS_SCL_LCNT = ");
    print_dec(register_value);
    print_cr_lf();
#endif

    // The I2C is an overlaid function on the GPIOx and GPIOy. Enable overlaying function 1
    H2_WRITE_MASKED(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(0), (VTSS_BIT(6) | VTSS_BIT(5)), (VTSS_BIT(6) | VTSS_BIT(5)));
    H2_WRITE_MASKED(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(1), 0, (VTSS_BIT(6) | VTSS_BIT(5)));


    // Set I2C clock frequency to 100 kbps.
    register_value = 0;
    register_value |=  VTSS_BIT(0); // Master enable
    register_value |=  VTSS_BIT(1); // Set bit 1
    register_value &= ~VTSS_BIT(2); // Clear bit 2
    register_value &= ~VTSS_BIT(3); // 7 bit mode
    register_value &= ~VTSS_BIT(4); // 7 bit mode
    register_value |=  VTSS_BIT(5); // Restart enable
    register_value |=  VTSS_BIT(6); // Slave disable

    H2_WRITE(VTSS_TWI_TWI_CFG, register_value);
#if I2C_DEBUG
    print_str("VTSS_TWI_TWI_CFG = 0x");
    print_hex_dw(register_value);
    print_cr_lf();
#endif

    // Set clock speed for to normal speed
    register_value = (0.25 * (clk_freq / 1000000)); // Datasheet section 6.13.9.30
    H2_WRITE(VTSS_TWI_TWI_SDA_SETUP, register_value);
#if I2C_DEBUG
    print_str("VTSS_TWI_TWI_SDA_SETUP = ");
    print_dec(register_value);
    print_cr_lf();
#endif

#if 0
    H2_WRITE(VTSS_TWI_TWI_TAR, 0);
    H2_WRITE(VTSS_TWI_TWI_SAR, 0);
    print_str("VTSS_TWI_TWI_SAR, 0");
#endif

    // Enable I2C controller
    H2_WRITE(VTSS_TWI_TWI_CTRL, VTSS_F_TWI_TWI_CTRL_ENABLE);

    //James : no int in unmanaged Enable interrupt for when the hardware rx fifo contains data
    //James H2_WRITE(VTSS_TWI_TWI_INTR_MASK, 0x04);
    //James H2_WRITE(VTSS_TWI_TWI_RX_TL, 0x00); // 0x00 means one byte of data in the fifo
}

ulong i2c_tx(const uchar i2c_addr,
            const uchar *tx_data,
            ulong count)
{ //James
    ulong bytes_transmitted = 0;
    int tx_byte;

    // Setup Target address register
    i2c_set_tar_register(i2c_addr);

    //  Do the write
#if I2C_DEBUG
    print_str("tx_data = ");
#endif
    if (wait_for_tx_buffer_empty()) {
        for (tx_byte = count; tx_byte > 0; tx_byte--) {
#if I2C_DEBUG
            print_str("0x");
            print_hex_b(*tx_data);
#endif
            if (tx_i2c_data(*tx_data)  == 0) {
                // tx buffer remained full - timeout
                break;
            }

            tx_data++; // Point to next data
            bytes_transmitted++; // Ok - one more byte transmitted.
            H2_WRITE_MASKED(VTSS_DEVCPU_GCB_GPIO_GPIO_OUT_CLR, VTSS_BIT(9), VTSS_BIT(9));

        }
    }
#if I2C_DEBUG
    print_cr_lf();
    print_str("TX UNLOCK: ");
    print_dec(i2c_addr);
    print_cr_lf();
#endif
    return bytes_transmitted;
}

#define HW_BUF_LEN 8

#if 0 /* for debug */
#define P_STR(a)            print_str(a)
#define P_HEX_W(a)          print_hex_w(a)
#define P_CR_LF()           print_cr_lf()
#else
#define P_STR(a)
#define P_HEX_W(a)
#define P_CR_LF()
#endif

ulong i2c_rx(const uchar i2c_addr,
             uchar* rx_data,
             ulong count)
{   //James
    ulong bytes_recieved = 0;
    ulong       value;
	ulong       rx_byte;
	int         byte_left ,i = 0;

#if I2C_DEBUG
    print_str("RX LOCK: ");
    print_dec(i2c_addr); print_cr_lf();
#endif
    // Initialize the read
    // Setup Target address register
    i2c_set_tar_register(i2c_addr);
#if I2C_DEBUG
    print_str("Rx data = ");
#endif
    byte_left = count;
    while (byte_left > 0)
    {
        if (byte_left > HW_BUF_LEN)
            rx_byte = HW_BUF_LEN;
        else
            rx_byte = byte_left;
        for (; rx_byte > 0; rx_byte--) {
            if (wait_for_tx_buffer_empty()) {
                if (tx_i2c_data(VTSS_F_TWI_TWI_DATA_CMD_CMD) == 0) {
                    println_str("I2C - Couldn't transmit read request");
                    return 0;
                }
            }
            else{
                println_str("wait_for_tx_buffer_empty");
            }

        }

        // Set timeout to 100 msec.
        //delay(MSEC_100);
        start_timer(MSEC_100);
        if (byte_left > HW_BUF_LEN)
        {
            rx_byte = HW_BUF_LEN;
            byte_left = byte_left  - HW_BUF_LEN;
        }
        else
        {
            rx_byte = byte_left;
            byte_left = 0;
        }

        for (; rx_byte > 0; rx_byte--) {

            do {
                H2_READ(VTSS_TWI_TWI_RXFLR, value);
                //print_str("VTSS_TWI_TWI_RXFLR : ");
                //print_dec(value);
                //print_cr_lf();
                if (timeout()) {
                    // Fifo was not emptied within 100 ms.
                    print_str("Read TimeOut(Addr) "); print_dec(i2c_addr);
                    print_cr_lf();

                    /* Recover I2C when errors happen, for example,
                       plug out SFP module when reading I2C data
                    */
                    H2_READ(VTSS_TWI_TWI_RAW_INTR_STAT, value);
                    P_STR("RAW_INTR_STAT(0xd): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_RX_TL, value);
                    P_STR("RX_TL(0xe): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_TX_TL, value);
                    P_STR("TX_TL(0xf): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_CLR_RX_UNDER, value);
                    P_STR("CLR_RX_UNDER(0x11): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_CLR_RX_OVER, value);
                    P_STR("CLR_RX_OVER(0x12): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_CLR_TX_OVER, value);
                    P_STR("CLR_TX_OVER(0x13): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_CLR_RD_REQ, value);
                    P_STR("CLR_RD_REQ(0x14): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_CLR_TX_ABRT, value);
                    P_STR("CLR_TX_ABRT(0x15): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_CLR_RX_DONE, value);
                    P_STR("CLR_RX_DONE(0x16): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_CLR_ACTIVITY, value);
                    P_STR("CLR_ACTIVITY(0x17): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_CLR_STOP_DET, value);
                    P_STR("CLR_STOP_DET(0x18): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_CLR_START_DET, value);
                    P_STR("CLR_START_DET(0x19): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_CLR_GEN_CALL, value);
                    P_STR("CLR_GEN_CALL(0x1a): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_CTRL, value);
                    P_STR("CTRL(0x1b): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_STAT, value);
                    P_STR("STAT(0x1c): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_TXFLR, value);
                    P_STR("TXFLR(0x1d): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_RXFLR, value);
                    P_STR("RXFLR(0x1e): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_TX_ABRT_SOURCE, value);
                    P_STR("TX_ABRT_SOURCE(0x20): 0x");  P_HEX_W(value); P_CR_LF();

                    H2_READ(VTSS_TWI_TWI_CLR_INTR, value);
                    P_STR("CLR_INTR(0x10): 0x");  P_HEX_W(value); P_CR_LF();

                    i2c_init();
                    return 0;
                }
            } while (value == 0);

            H2_READ(VTSS_TWI_TWI_DATA_CMD, value);
            //print_str("value "); print_hex_b(value); print_cr_lf();
            *rx_data = value & 0xff;
#if I2C_DEBUG
            print_dec(i); i++;
            print_str(" 0x");
            print_hex_b(*rx_data);
            print_cr_lf();
#endif
            rx_data++;
            bytes_recieved++;
        }
    }
#if 0
    print_str("bytes_recieved");
    print_dec(bytes_recieved);
    H2_READ(VTSS_TWI_TWI_RXFLR, value);
                print_str("  VTSS_TWI_TWI_RXFLR : ");
                print_dec(value);
    print_cr_lf();
#endif
    return bytes_recieved;
}


ulong i2c_eeprom_read(const uchar i2c_addr,
                      ulong *mem_addr,
                      uchar *i2c_data)
{
    uchar addr_8;

    if (*mem_addr > 65535) {
      print_str(" addr > 65535 not implemet");
    } else if (*mem_addr > 255) {
      addr_8  = (*mem_addr) >> 8;
      i2c_tx(i2c_addr, &addr_8, 1);
      addr_8  = (*mem_addr)& 0xff;
      i2c_tx(i2c_addr, &addr_8, 1);
    } else {
      addr_8  = *mem_addr;
      i2c_tx(i2c_addr, &addr_8, 1);
    }

    if (i2c_rx(i2c_addr,i2c_data, 1) != 0 ) {
        return TRUE; // suceess
    } else {
        return FALSE; // fail
    }

}

#endif

//---------------------------------------------------------------------------
// EOF i2c_h.c

