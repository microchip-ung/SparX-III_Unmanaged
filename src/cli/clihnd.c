//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT


#include <ctype.h>
#include <string.h>

#include "common.h"     /* Always include common.h at the first place of user-defined herder files */
#include "sysutil.h"
#include "hwport.h"
#if TRANSIT_EEE
#include "eee_api.h"
#include "eee_base_api.h"
#endif

#include "txt.h"
#include "h2io.h"
#include "uartdrv.h"
#include "main.h"
#include "print.h"
#include "h2.h"
#include "timer.h"
#include "phydrv.h"
#include "phymap.h"
#include "txrxtst.h"
#include "misc1.h"
#include "clihnd.h"
#include "version.h"
#include "spiflash.h"
#include "vtss_luton26_regs_devcpu_gcb.h"
#include "vtss_luton26_regs_dev.h"
#include "vtss_luton26_reg.h"
#include "i2c_h.h"
#include "ledtsk.h"
#include "i2c.h"
#include "misc2.h"
#include "vtss_common_os.h"
#if TRANSIT_EEE
#include "eee_api.h"
#include "eee_base_api.h"
#endif
#if TRANSIT_FAN_CONTROL
#include "fan_api.h"
#endif
#if TRANSIT_LLDP
#include "lldp_remote.h"
#include "lldp_tlv.h"
#include "h2txrx.h"
#endif
#if TRANSIT_VERIPHY
#include "veriphy.h"
#endif
#include "h2stats.h"

#ifndef NO_DEBUG_IF

/*****************************************************************************
 *
 *
 * Defines
 *
 *
 *
 ****************************************************************************/

#define MAX_NO_OF_PARMS 4

#define MAX_CMD_LEN 30

#define FORMAT_OK    0
#define FORMAT_ERROR 1

#if TRANSIT_EEE
#define EEE_WAKEUP_TIME_MAX 1000
#define EEE_WAKEUP_TIME_MIN 0
#endif

/*****************************************************************************
 *
 *
 * Typedefs and enums
 *
 *
 *
 ****************************************************************************/

typedef struct {
    uchar *str;
    uchar len;
} str_parm_t;

/*****************************************************************************
 *
 *
 * Prototypes for local functions
 *
 *
 *
 ****************************************************************************/

static uchar handle_command (void);
static void  handle_v_command (void);
static uchar retrieve_parms (void);
#if LUTON_UNMANAGED_CONF_IF
static uchar retrieve_str_parms (void);
#endif
static void  skip_spaces (void);
static uchar cmd_cmp (char *s1, char *s2) small;
#if LUTON_UNMANAGED_CONF_IF
static uchar handle_sys_config (void);
#endif
#ifndef VTSS_COMMON_NDEBUG
static void update_debug_lvl(uchar lvl);
#endif
#if TRANSIT_FAN_CONTROL
#if UNMANAGED_FAN_DEBUG_IF
static void handle_fan_control (void);
#endif
#endif
#if TRANSIT_LLDP
#if UNMANAGED_LLDP_DEBUG_IF
static void cmd_print_lldp_remoteinfo (void);
#endif
#endif
#if TRANSIT_VERIPHY
static void cmd_run_veriphy(uchar port_no);
#endif

/*****************************************************************************
 *
 *
 * Local data
 *
 *
 *
 ****************************************************************************/

static ulong idata parms [MAX_NO_OF_PARMS];
static str_parm_t str_parms [MAX_NO_OF_PARMS];
static uchar parms_no;

static uchar xdata cmd_buf [MAX_CMD_LEN];
static uchar xdata *cmd_ptr;
static uchar cmd_len = 0;

/* ************************************************************************ */
void cli_tsk (void)
/* ------------------------------------------------------------------------ --
 * Purpose     : Handle command line interface.
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uchar error_status;

    if (cmd_ready()) {

        cmd_ptr = &cmd_buf[0];
        skip_spaces();
#if LUTON_UNMANAGED_SWUP
        if (cmd_cmp(cmd_ptr, "ERASE") == 0) {
#ifndef VTSS_COMMON_NDEBUG
            vtss_os_trace_level = 0;
#endif

            cmd_ptr += (sizeof("ERASE") - 1);
            if (retrieve_parms() != FORMAT_OK) {
                error_status = FORMAT_ERROR;
            } else {
#if FRONT_LED_PRESENT
                led_status(VTSS_LED_MODE_BLINK_GREEN);
#endif
                print_cr_lf();
                if (flash_erase_code((ulong) parms[0])) {
                    println_str("Fail");
                } else {
                    println_str("Done");
                }
                error_status = 0;
#if FRONT_LED_PRESENT
                led_status(VTSS_LED_MODE_ON_GREEN);
#endif
            }
        } else if (cmd_cmp(cmd_ptr, "PROGRAM") == 0) {

            cmd_ptr += (sizeof("PROGRAM") - 1);
            if (retrieve_parms() != FORMAT_OK) {
                error_status = FORMAT_ERROR;
            }
            else if (parms_no != 3) {
                error_status = FORMAT_ERROR;
            } else {
#ifndef VTSS_COMMON_NDEBUG
                vtss_os_trace_level = 0;
#endif
#if FRONT_LED_PRESENT
                led_status(VTSS_LED_MODE_BLINK_GREEN);
#endif
                print_cr_lf();
                if (flash_download_image((ulong) parms[0], (ulong) parms[1], (uchar) parms[2])) {
                    println_str("Fail");
#if FRONT_LED_PRESENT
                    led_status(VTSS_LED_MODE_ON_GREEN);
#endif
                } else {
                    println_str("Done");
                    h2_reset();
                }
            }
        } else
#endif
#if LUTON_UNMANAGED_CONF_IF
            /*
                config mac
                config mac xx-xx-xx-xx-xx-xx
                config dlevel error, warning, debug, noise
            */
            if(cmp_cmd_txt(CMD_TXT_NO_CONFIG, cmd_ptr)) {
                cmd_ptr += cmd_txt_tab[CMD_TXT_NO_CONFIG].min_match;
                error_status = handle_sys_config();
            } else
#endif
                error_status = handle_command();


        if (error_status) {
            /* empty queue */
            while (uart_byte_ready()) {
                (void) uart_get_byte();
            }
            print_str(txt_01); /* error message */
        }

        cmd_len = 0;
    }
}

/* ************************************************************************ */
bool cmd_ready (void)
/* ------------------------------------------------------------------------ --
 * Purpose     : Collect bytes received by uart driver and check if a command
 *               is ready (i.e. <CR> received).
 * Remarks     : Returns TRUE, if command is ready, otherwise FALSE.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uchar ch;

    if (uart_byte_ready()) {
        ch = uart_get_byte();
        if (ch != 0x0a) { /* discard LF chars */

            if (ch == 0x08) { /* handle backspace char */
                if (cmd_len > 0) {
                    cmd_len--;
                }
            } else {
                if (cmd_len < MAX_CMD_LEN) {
                    cmd_buf[cmd_len++] = ch;
                    /* echo */
                    uart_put_byte(ch);

                }
                if (ch == 0x0d) {
                    /* error handling: ensure that CR is present in buffer in case of buffer overflow */
                    if (cmd_len == MAX_CMD_LEN) {
                        cmd_buf[MAX_CMD_LEN - 1] = 0x0d;
                    }

                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

#if 0 /* For debug only */
void list_mii_reg(uchar iport)
{
    uchar  cnt;

    println_str("std");

    for (cnt = 0; cnt <= 31; cnt++) {
        print_hex_w(cnt);
        print_str(": ");
        print_hex_w(phy_read(iport, cnt));
        print_cr_lf();
    }

    println_str("ext1");

    phy_page_ext(iport);
    for (cnt = 0x12; cnt <= 0x1e; cnt++) {
        print_hex_w(cnt);
        print_str(": ");
        print_hex_w(phy_read(iport, cnt));
        print_cr_lf();
    }

    println_str("ext2");

    phy_page_ext2(iport);
    for (cnt = 0x10; cnt <= 0x11; cnt++) {
        print_hex_w(cnt);
        print_str(": ");
        print_hex_w(phy_read(iport, cnt));
        print_cr_lf();
    }

    println_str("gp");

    phy_page_gp(iport);
    for (cnt = 0x0e; cnt <= 0x1d; cnt++) {
        print_hex_w(cnt);
        print_str(": ");
        print_hex_w(phy_read(iport, cnt));
        print_cr_lf();
    }

    phy_page_std(iport);

}
#endif


/* ************************************************************************ */
static uchar handle_command (void)
/* ------------------------------------------------------------------------ --
 * Purpose     : Interpret and handle command (apart from config commands).
 * Remarks     : Module variable cmd_ptr must have been set to point to first
 *               non-space char in command string, when this function is called.
 *               Returns 0, if successful, otherwise <> 0.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uchar cmd;
#ifndef UNMANAGED_REDUCED_DEBUG_IF
    ulong dat;
#if USE_HW_TWI
    ulong count;
    uchar i2c_addr,i2c_data;
    ulong curr_addr_l;
    uchar curr_addr;
#endif
#endif

    cmd = *cmd_ptr;
    cmd_ptr++;

    if (retrieve_parms() != FORMAT_OK) {
        return FORMAT_ERROR;
    }

    switch (conv_to_upper_case(cmd)) {

    case 'V': /* Get version info */
        handle_v_command();
        break;

    case 'R': /* Read from chip register */
        print_cr_lf();
        print_hex_prefix();
        /* Target and offset as parameter */
        H2_READ((parms[0] + parms[1] + parms[2]), parms[3] /*dat*/);
        print_hex_dw(parms[3]/*dat*/);
        print_cr_lf();
        break;

    case 'W': /* Write to chip register */

        if (parms_no >= 4) {
            H2_WRITE((parms[0] + parms[1] + parms[2]), parms[3]);
        }
        break;

    case 'I': /* Read (input) from PHY register */
        print_cr_lf();
        /* Front port number and PHY address */
        print_hex_prefix();
        if (parms_no > 2) {
            // Change page
            phy_write(port2int((uchar) parms[0]), 31, parms[2]);
            print_hex_w(phy_read(port2int((uchar) parms[0]), parms[1]));
            phy_write(port2int((uchar) parms[0]), 31, 0); // Back to std page
#if 0
        } else if (parms_no == 1) {
            /* list all mii register */
            list_mii_reg(port2int((uchar) parms[0]));
#endif
        } else {
            print_hex_w(phy_read(port2int((uchar) parms[0]), parms[1]));
        }


        print_cr_lf();
        break;

    case 'O': /* write (output) to PHY register */
        /* Front port number and PHY address */
        if (parms_no >= 3) {
            /* Change page */
            phy_write(port2int((uchar) parms[0]), 31, parms[3]);
            phy_write(port2int((uchar) parms[0]), parms[1], parms[2]);
            phy_write(port2int((uchar) parms[0]), 31, 0);
        } else if (parms_no == 2) {
            phy_write(port2int((uchar) parms[0]), parms[1], parms[2]); // direct write
        }
        break;

#ifndef VTSS_COMMON_NDEBUG
    case 'L': /* test LED */
        led_state(parms[0], parms[1], parms[2]);
        break;
#endif

    case '?':
        println_str("V : Show version");
        println_str("R <target> <offset> <addr>: Read from chip register");
        println_str("W <target> <offset> <addr> <value>: Write to chip register");
        println_str("I <port> <addr> [page]: Read (input) from PHY register");
        println_str("O <port> <addr> <value> [page]: Write (output) to PHY register");
        println_str("? : Show commands");
#ifndef UNMANAGED_REDUCED_DEBUG_IF
#if LOOPBACK_TEST
        println_str("T : Loopback test");
#endif
#if TRANSIT_LLDP
#if TRANSIT_EEE_LLDP
        println_str("G [0|1|2|3] : 0 = Disable, 1 = Enable LLDP, 2 = Remove EEE TLV, 3 = Add EEE TLV");
#else
        println_str("G [0|1] : 0 = Disable, 1 = Enable LLDP");
#endif
#endif
        println_str("Z [0|1] : 0/1 D/Enable E-Col-Drop(link-up ports only)");
#if UNMANAGED_FAN_DEBUG_IF
        println_str("F temp_max temp_on: Update temp. max and on");
#endif
#ifndef VTSS_COMMON_NDEBUG
        println_str("B level: Set debug level 1 to 4");
#endif
#if UNMANAGED_TRANSIT_VERIPHY_DEBUG_IF
        println_str("C [port_no|0xff] : run veriphy");
#endif
#if TRANSIT_EEE
        println_str("A [0|1|2]: 1=EEE mode rev_b (max savings), 2=EEE mode rev_a (less power savings), 0=EEE Disabled");
#if TRANSIT_EEE_LLDP
        println_str("LLDP with EEE tlv enabled/disabled automatically ");
#endif
#endif
#if UNMANAGED_PORT_STATISTICS_IF
        println_str("H port_no : show port statistics");
#endif
#if USE_HW_TWI
        println_str("C : read I2C data <I2C_addr> <starting addr> <count>");
#endif
        println_str("S : Suspend/Resume applications");
#if LUTON_UNMANAGED_SWUP
        println_str("D : Dump bytes from SPI flash");
#endif
#endif /* UNMANAGED_REDUCED_DEBUG_IF */
#if LUTON_UNMANAGED_CONF_IF
        println_str("CONFIG                       : Show all configurations");
        println_str("CONFIG MAC xx:xx:xx:xx:xx:xx : Update MAC addresses in RAM");
        println_str("CONFIG SAVE                  : Program configurations at RAM to flash");
#endif
        break;

#ifndef UNMANAGED_REDUCED_DEBUG_IF
    case 'T': /* Tests */
        switch ((uchar) parms[0]) {
#if LOOPBACK_TEST
        case 1:
            perform_tx_rx_test(parms[1], parms[2], 0);
            break;
        case 2:
            perform_tx_rx_test(parms[1], parms[2], 1);
            break;
#endif /* LOOPBACK_TEST */

        default:
            return FORMAT_ERROR;
            //break;
        }
        break;

    case 'Z': /* enable/disalbe Excessive Col drop */
        if (parms_no == 1) {
#if 0
            uchar port_no;
            uchar port_ext;
            ulong exc_col_ena = 0;

            if (parms[0] == 0)
                exc_col_ena = VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_RETRY_AFTER_EXC_COL_ENA;

            for (port_ext = MIN_PORT; port_ext < MAX_PORT; port_ext++) {
                port_no = port2int(port_ext);

                /* Set MAC HDX Late collision and load PCS seed. */
                H2_WRITE(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_HDX_CFG(VTSS_TO_DEV(port_no)),
                         VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_LATE_COL_POS((port_no < 12)?64:67) |
                         VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_SEED(port_no) |
                         exc_col_ena);
            }
#endif
            parms[1] = 0;
            if(!parms[0]) /* 0: don't drop packet */
                parms[1] = VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_RETRY_AFTER_EXC_COL_ENA;

            for(parms[2] = MIN_PORT; parms[2] < MAX_PORT; parms[2]++) {
                parms[3] = port2int((uchar) parms[2]);
                /* Set MAC HDX Late collision and load PCS seed. */
                H2_WRITE(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_HDX_CFG(VTSS_TO_DEV(parms[3])),
                         VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_LATE_COL_POS((parms[3] < 12)?64:67) |
                         VTSS_F_DEV_MAC_CFG_STATUS_MAC_HDX_CFG_SEED(parms[3]) |
                         parms[1]);
            }
        }
        break;
#if USE_HW_TWI
    case 'C': /* Read from chip register */
        print_cr_lf();
        i2c_addr = parms[0];
        if (parms[2] == 0) { /* test i2c_eeprom_read */
            curr_addr_l = parms[1];
            if (i2c_eeprom_read(i2c_addr, &curr_addr_l, &i2c_data) == TRUE) {
                print_hex_b(i2c_data);
                print_spaces(2);
                if (i2c_data > 32 && i2c_data< 127)
                    print_ch(i2c_data);
                print_cr_lf();
            } else {
                print_str("access fail");
                print_cr_lf();
            }

        } else { /* sample code of i2c_tx and i2c_rx*/
            curr_addr = parms[1];
            for (count = 0; count < parms[2]; count++) {
                i2c_tx(i2c_addr, &curr_addr, 1 );
                dat = i2c_rx(i2c_addr,&i2c_data, 1);
                if (dat != 0 ) {
                    print_dec(count);
                    print_str(": ");
                    print_hex_b(i2c_data);
                    print_spaces(2);
                    if (i2c_data > 32 && i2c_data< 127)
                        print_ch(i2c_data);
                    print_cr_lf();
                } else
                    break;
                //print_hex_dw(dat);
                //print_cr_lf();
                curr_addr++;
            }
        }

        break;
#endif
#if UNMANAGED_PORT_STATISTICS_IF
    case 'H':
        if(parms_no == 0) {
            start_display_statis_bit = 1;
            port_cnt = MIN_PORT;
        } else if(parms_no == 1 &&
                  (uchar) parms[0] >= MIN_PORT && (uchar) parms[0] < MAX_PORT) {
            print_port_statistics (port2int((uchar) parms[0]));
        } else {
            return FORMAT_ERROR;
        }
        break;
    case 0x18:
        start_display_statis_bit = 0;
        break;
#endif

#if TRANSIT_FAN_CONTROL
#if UNMANAGED_FAN_DEBUG_IF
    case 'F':
        /*
        ** F t_max t_on
        ** F
        */
        if (parms_no >= 1) {
            write_fan_conf_t_max((uchar) parms[0]);
            write_fan_conf_t_on((uchar) parms[1]);
        } else {
            handle_fan_control();
        }
        break;
#endif
#endif

#if TRANSIT_EEE
        //
        // Enable / disable EEE for all ports.
        // cmd:"A 0"  = EEE Disable, disable LLDP
        // cmd:"A 1"  = EEE Enable VGA down, enable LLDP
        // cmd:"A 2"  = EEE Enable VGA up, enable LLDP

    case 'A':
        // We set all ports to the same.
        for (parms[1] = 0; parms[1] < NO_OF_PORTS; parms[1]++)
        {
            vtss_port_no_t  iport = parms[1];
            vtss_port_no_t  fport = iport + 1;
            vtss_port_no_t  sport = port2int(fport);

            switch ((uchar) parms[0]) {
            case 0:
                /* Disable EEE */
                write_eee_conf_mode(iport, FALSE);
                eee_port_mode_setup(sport);
#if TRANSIT_EEE_LLDP
                lldp_os_set_admin_status(fport, LLDP_DISABLED);
#endif
                break;
            case 1:
                /* Enable EEE*/
                write_eee_conf_mode(iport, TRUE);
                eee_port_mode_setup(sport);
                /* Enable VGA Down*/
#if VTSS_ATOM12_A
                vga_adc_debug (sport, ATOM12_EN_NONE);
#endif
#if TRANSIT_EEE_LLDP
                lldp_os_set_admin_status(fport, LLDP_ENABLED_RX_TX);
#endif
                break;
            case 2:
                /* Enable EEE*/
                write_eee_conf_mode(iport, TRUE);
                eee_port_mode_setup(sport);
#if VTSS_ATOM12_A
                /* Enable VGA Up*/
                vga_adc_debug (sport, ATOM12_EN_BOTH);
#endif
#if TRANSIT_EEE_LLDP
                lldp_os_set_admin_status(fport, LLDP_ENABLED_RX_TX);
#endif
                break;

            case 3:
                dat = phy_mmd_rd(sport, 7, 61);
                print_str ("7.61 =0x");
                print_hex_w(dat);

                dat = phy_mmd_rd(sport, 7, 60);
                print_str (" 7.60 =0x");
                print_hex_w(dat);

                dat = phy_mmd_rd(sport, 3, 1);
                print_str (" 3.1 =0x");
                print_hex_w(dat);

                print_str (" \r\n");
                break;

            default:
                // print out current state
                print_str("Port ");
                print_dec(fport);

                if (read_eee_conf_mode(fport)) {
                    print_str(": Enabled");
                } else {
                    print_str(": Disabled");
                }
#if TRANSIT_LLDP
                if (lldp_os_get_admin_status(fport)) {
                    print_str(" w LLDP");
#if TRANSIT_EEE_LLDP
                    if(lldp_os_get_optional_tlv_enabled(LLDP_TLV_ORG_EEE_TLV)) {
                        print_str(" EEE tlv");
                    } else {
                        print_str(" No EEE tlv");
                    }
#endif /* TRANSIT_EEE_LLDP */
                } else {
                    print_str(" w/o LLDP");
                }
#endif /* TRANSIT_LLDP */
                print_cr_lf();
                break;
            }
        }
        print_cr_lf();
        break;

#if UNMANAGED_EEE_DEBUG_IF
    case 'X': // Make system reboot
        h2_reset();
        break;
#endif
#endif /* TRANSIT_EEE */

#if TRANSIT_LLDP
    case 'G':
        if (parms_no == 1) {
            switch((uchar) parms[0]) {
            case 0: /* set disable */
                for(parms[1] = MIN_PORT; parms[1] < MAX_PORT; parms[1]++) {
                    lldp_os_set_admin_status((uchar)parms[1], LLDP_DISABLED);
                }
#if TRANSIT_BPDU_PASS_THROUGH
                h2_bpdu_t_registration(0x0E, FALSE);
#endif
                break;
            case 1: /* set enable */
                for(parms[1] = MIN_PORT; parms[1] < MAX_PORT; parms[1]++) {
                    lldp_os_set_admin_status((uchar)parms[1], LLDP_ENABLED_RX_TX);
                }
#if TRANSIT_BPDU_PASS_THROUGH
                h2_bpdu_t_registration(0x0E, TRUE);
#endif
                break;
#if TRANSIT_EEE_LLDP
            case 2: /* Remove LLDP EEE tlv */
                lldp_os_set_optional_tlv(LLDP_TLV_ORG_EEE_TLV, 0);
                break;
            case 3: /* Add LLDP EEE tlv */
                lldp_os_set_optional_tlv(LLDP_TLV_ORG_EEE_TLV, 1);
                break;
#endif
            default:
                return FORMAT_ERROR;
            }
            break;
        }
#if UNMANAGED_LLDP_DEBUG_IF
        cmd_print_lldp_remoteinfo();
#endif
        break;
#endif

#ifndef VTSS_COMMON_NDEBUG
    case 'B': /* debug level */
        if(((uchar) parms[0] <= VTSS_COMMON_TRLVL_RACKET) &&
                ((uchar) parms[0] >= VTSS_COMMON_TRLVL_ERROR)) {
            update_debug_lvl((uchar) parms[0]);
        }
        break;
#endif

#if UNMANAGED_TRANSIT_VERIPHY_DEBUG_IF
    case 'C':
        cmd_run_veriphy((uchar) parms[0]);
        break;
#endif
    case 'S': /* Suspend/Resume applications */
        switch ((uchar) parms[0]) {
        case 0:
            /* Resume */
            sysutil_set_suspend(FALSE);
            break;
        case 1:
            /* Suspend */
            sysutil_set_suspend(TRUE);
            break;

        default:
            return FORMAT_ERROR;
        }
        break;
#if LUTON_UNMANAGED_SWUP
    case 'D': /* Dump bytes from SPI flash */
        flash_read_bytes(parms[0], parms[1]);
        break;
#endif

#endif /* UNMANAGED_REDUCED_DEBUG_IF */

    default:
        return FORMAT_ERROR;
    }
    return FORMAT_OK;
}

/* ************************************************************************ */
static uchar retrieve_parms (void)
/* ------------------------------------------------------------------------ --
 * Purpose     : Retrieve parameters from command string.
 * Remarks     : Module variable cmd_ptr must have been set to point to first
 *               char after the command in the command string, when this function
 *               is called.
 *               The module variables parms_no and parms are updated with
 *               actual number of parameters and the actual parameter values.
 *               Returns 0, if successful, otherwise <> 0.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uchar ch;
    uchar base;
    uchar j;
    uchar no_of_digits;
    uchar digits [10];
    ulong parm_bin;

    ch = *cmd_ptr;
    if ((ch != ' ') && (ch != 0x0d)) {
        return FORMAT_ERROR;
    }


    parms_no = 0;
    /* Preset parms to ff's, which may be used as default indication */
    memset(parms, 0xff, sizeof(parms));

    /*
    ** Retrieve parameters one by one.
    */
    for (;;) {

        skip_spaces();
        base = 10; /* default parameter is specified in decimal */
        no_of_digits = 0;

        /*
        ** Check if any hex prefix
        */
        if (*cmd_ptr == '0' && (conv_to_upper_case(*(cmd_ptr + 1)) == 'X')) {
            base = 16; /* parameter is specified in hex */
            cmd_ptr += 2;
            if (*cmd_ptr == ' ') {
                return FORMAT_ERROR;
            }
        }

        /*
        ** Retrieve digits until delimiter (space or CR) and then convert
        ** parameter to binary
        */
        for (;;) {

            ch = *cmd_ptr;

            if ((ch == ' ') || (ch == 0x0d)) {

                if (no_of_digits > 0) {
                    parm_bin = 0;
                    for (j = 0; j < no_of_digits; j++) {
                        parm_bin = (parm_bin * base) + digits[j];
                    }
                    if (parms_no < MAX_NO_OF_PARMS) {
                        parms[parms_no++] = parm_bin;
                    }
                }
                /* End processing at end of command string */
                if (ch == 0x0d) {
                    return FORMAT_OK;
                }
                break; /* go get new parameter */
            } else {

                ch = ascii_to_hex_nib(ch);
                if (ch != 0xff) {
                    if (no_of_digits < 10) {
                        digits[no_of_digits++] = ch;
                        if (ch > 9) {
                            base = 16; /* parameter is specified in hex */
                        }
                    }
                } else {
                    return FORMAT_ERROR;
                }
            }
            cmd_ptr++;
        }
    }
}

#if LUTON_UNMANAGED_CONF_IF
/* ************************************************************************ */
static uchar retrieve_str_parms (void)
/* ------------------------------------------------------------------------ --
 * Purpose     : Retrieve parameters from command string.
 * Remarks     : Module variable cmd_ptr must have been set to point to first
 *               char after the command in the command string, when this function
 *               is called.
 *               The module variables parms_no and parms are updated with
 *               actual number of parameters and the actual parameter values.
 *               Returns 0, if successful, otherwise <> 0.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uchar ch;
    uchar j;

    ch = *cmd_ptr;
    if ((ch != ' ') && (ch != 0x0d)) {
        return FORMAT_ERROR;
    }

    parms_no = 0;
    for(parms_no = 0; parms_no < MAX_NO_OF_PARMS; parms_no++) {
        skip_spaces();
        str_parms[parms_no].str = cmd_ptr;
        str_parms[parms_no].len = 0;
        while(1) {
            if(*cmd_ptr == 0x0d) { /* Enter Key */
                *cmd_ptr = '\0';
                for(j = parms_no + 1; j < MAX_NO_OF_PARMS; j++) {
                    str_parms[j].str = cmd_ptr;
                    str_parms[j].len = 0;
                }
                return FORMAT_OK;
            }
            if(*cmd_ptr == ' ') { /* Space Key */
                *cmd_ptr = '\0'; // string null end sign
                cmd_ptr++;
                break;
            }
            str_parms[parms_no].len++;
            cmd_ptr++;
        }
    }
    return FORMAT_OK;
}
#endif

#ifndef VTSS_COMMON_NDEBUG
static void update_debug_lvl(uchar lvl)
{
    vtss_os_trace_level = lvl;
}
#endif

#if LUTON_UNMANAGED_CONF_IF
static uchar cmd_retrieve_mac_addr (uchar *mac_addr_str, uchar * mac_addr)
/* ------------------------------------------------------------------------ --
 * Purpose     : Retrieve MAC value from command string.
 * Remarks     : Module variable cmd_ptr must have been set to point to first
 *               non-space char after the "config mac" command in the command string,
 *               when this function is called.
 *               The retrieved MAC address is returned in module variable cmd_mac_addr
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uchar j;
    uchar k;
    uchar ch;
    uchar *ptr = mac_addr_str;

    for (j = 0; j < 6; j++) {
        for (k = 0; k < 2; k++) {
            ch = *ptr;
            ptr++;

            ch = ascii_to_hex_nib(ch);
            if (ch == 0xff) {
                return FORMAT_ERROR;
            }

            mac_addr[j] = (mac_addr[j] << 4) | ch;
        }

        if (j < 5) {
            ch = *ptr;
            if (ascii_to_hex_nib(ch) == 0xff) {
                ptr++;
                if ((ch != '-') && (ch != ':') && (ch != '.')) {
                    return FORMAT_ERROR;
                }
            }
        }
    }
    if(*ptr != '\0') {
        return FORMAT_ERROR;
    }

    return FORMAT_OK;
}
#endif

/* ************************************************************************ */
static void skip_spaces (void)
/* ------------------------------------------------------------------------ --
 * Purpose     : Adjust cmd_ptr to point to next char different from space.
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    while (*cmd_ptr == ' ') {
        cmd_ptr++;
    }
}

#if UNMANAGED_TRANSIT_VERIPHY_DEBUG_IF
/* ************************************************************************ */
static void print_veriphy_status (uchar status)
/* ------------------------------------------------------------------------ --
 * Purpose     :
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    code std_txt_t status_txt_tab [16] = {
        TXT_NO_VERIPHY_OK,       /*  0, Correctly terminated pair */
        TXT_NO_VERIPHY_OPEN,     /*  1, Open pair */
        TXT_NO_VERIPHY_SHORT,    /*  2, Shorted pair */
        TXT_NO_VERIPHY_FAULT,    /*  3, not used */
        TXT_NO_VERIPHY_ABNORMAL, /*  4, Abnormal termination */
        TXT_NO_VERIPHY_FAULT,    /*  5, not used */
        TXT_NO_VERIPHY_FAULT,    /*  6, not used */
        TXT_NO_VERIPHY_FAULT,    /*  7, not used */
        TXT_NO_VERIPHY_XA,       /*  8, Cross-pair short to pair A */
        TXT_NO_VERIPHY_XB,       /*  9, Cross-pair short to pair B */
        TXT_NO_VERIPHY_XC,       /* 10, Cross-pair short to pair C */
        TXT_NO_VERIPHY_XD,       /* 11, Cross-pair short to pair D */
        TXT_NO_VERIPHY_XCPLA,    /* 12, Abnormal cross-pair coupling with pair A */
        TXT_NO_VERIPHY_XCPLB,    /* 13, Abnormal cross-pair coupling with pair B */
        TXT_NO_VERIPHY_XCPLC,    /* 14, Abnormal cross-pair coupling with pair C */
        TXT_NO_VERIPHY_XCPLD,    /* 15, Abnormal cross-pair coupling with pair D */
    };
    print_txt(status_txt_tab[status]);

}

static void cmd_run_veriphy(uchar uport_no)
{
    uchar port_no;
    uchar j;
    uchar port_no_ext;
    port_bit_mask_t port_mask;
    BOOL    done = FALSE;   
    /* veriphy_parms_t xdata veriphy_parms [MAX_PORT]; */
    veriphy_parms_t xdata *veriphy_parms = (veriphy_parms_t *) rx_packet;

    port_mask = 0;
    if(uport_no == 0xff) {
        port_mask = ALL_PORTS;
    } else {
        WRITE_PORT_BIT_MASK(port2int(uport_no), 1, &port_mask);
    }

 
    for (port_no = MIN_PORT; port_no < MAX_PORT; port_no++) {
        if (TEST_PORT_BIT_MASK(port_no, &port_mask) && phy_map(port_no)) {
            veriphy_start(port_no); // Starting veriphy for selected port(s)
        }
    }

    // Pulling Verphy until Veriphy is done
    for (port_no = MIN_PORT; port_no < MAX_PORT; port_no++) {
        phy_page_std(port_no);//read phy status register
        if (TEST_PORT_BIT_MASK(port_no, &port_mask) && phy_map(port_no)) {
            done = FALSE;
            while (!done) {			
                veriphy_run(port_no, (veriphy_parms + port_no), &done);
            }
        }
        else {//For Non-Test port, set link status as "OPEN"
            veriphy_parms[port_no].loc[0]=0;
            veriphy_parms[port_no].loc[1]=0;
            veriphy_parms[port_no].loc[2]=0;
            veriphy_parms[port_no].loc[3]=0;
            veriphy_parms[port_no].stat[0]=1;
            veriphy_parms[port_no].stat[1]=1;
            veriphy_parms[port_no].stat[2]=1;
            veriphy_parms[port_no].stat[3]=1;
            veriphy_parms[port_no].flags=0;
        }
    }

    /* Print header */
    print_txt(TXT_NO_VERIPHY_STAT_HDR);
    for (port_no_ext = 1; port_no_ext < MAX_PORT; port_no_ext++) {
        port_no = port2int(port_no_ext);
        if (TEST_PORT_BIT_MASK(port_no, &port_mask) && phy_map(port_no) ) {
            print_dec_16_right(port_no_ext, 2);
            print_ch(':');
            print_spaces(3);
            /* Valid */
            if ((veriphy_parms+port_no)->flags) {
                print_str("yes");
            } else {
                print_str("no ");
            }
            print_spaces(2);

            /* Length or distance to fault for each pair */
            for (j = 0; j < 4; j++) {
                print_spaces(3);

			 if ((veriphy_parms + port_no)->loc[j] != 0xff) {
				 print_dec_16_right(veriphy_parms[port_no].loc[j], 3);

                } else {
                    print_str("  -");
                }
            }

            /* Status for each pair */

            for (j = 0; j < 4; j++) {
                print_spaces(2);
                print_veriphy_status((veriphy_parms + port_no)->stat[j]);
#if FRONT_LED_PRESENT
                if((veriphy_parms+port_no)->stat[j]) {
                    /* Set Error Leds */
                    led_state_set(port_no_ext, VTSS_LED_EVENT_CABLE, VTSS_LED_MODE_BLINK_YELLOW);
                }
#endif
            }
            print_cr_lf();
        }
    }

#if FRONT_LED_PRESENT
    delay(MSEC_100);
    for (port_no_ext = 1; port_no_ext < MAX_PORT; port_no_ext++) {
        port_no = port2int(port_no_ext);

        if (TEST_PORT_BIT_MASK(port_no, &port_mask) && phy_map(port_no)) {
            led_state_set(port_no_ext, VTSS_LED_EVENT_CABLE, VTSS_LED_MODE_NORMAL);
        }
    }
#endif
}
#endif

#if TRANSIT_LLDP
#if UNMANAGED_LLDP_DEBUG_IF
static void report_remote_entry_val (std_txt_t txt_no, lldp_tlv_t lldp_field, lldp_remote_entry_t xdata * entry)
{

    print_txt_left(txt_no, 15);
    if(txt_len(txt_no) >= 15) {
        print_spaces(1);
    }

    lldp_remote_tlv_to_string(entry, lldp_field, rx_packet);
    println_str(rx_packet);
    rx_packet[0] = 0;
}

static void cmd_print_lldp_remoteinfo (void)
{
    uchar i, max;
    lldp_remote_entry_t xdata * entry;
    uchar ext_port;
    uchar found = FALSE;


    for(ext_port = MIN_PORT; ext_port < MAX_PORT; ext_port++) {
        max = lldp_remote_get_max_entries();
        for(i = 0; i < max; i++) {
            entry = lldp_get_remote_entry(i);
            if((entry->in_use) && (entry->receive_port == ext_port)) {
                found = TRUE;
                print_txt(TXT_NO_LLDP_LOCAL_PORT);
                print_ch(':');
                print_spaces(4);
                print_dec(ext_port);
                print_cr_lf();

                print_txt_left(TXT_NO_LLDP_CHASSIS_TYPE, 15);
                lldp_chassis_type_to_string(entry, rx_packet);
                println_str(rx_packet);

                report_remote_entry_val(TXT_NO_LLDP_CHASSIS_ID, LLDP_TLV_BASIC_MGMT_CHASSIS_ID, entry);

                print_txt_left(TXT_NO_LLDP_PORT_TYPE, 15);
                lldp_port_type_to_string(entry, rx_packet);
                println_str(rx_packet);

                report_remote_entry_val(TXT_NO_LLDP_PORT_ID, LLDP_TLV_BASIC_MGMT_PORT_ID, entry);
                report_remote_entry_val(TXT_NO_LLDP_SYSTEM_NAME, LLDP_TLV_BASIC_MGMT_SYSTEM_NAME, entry);
                report_remote_entry_val(TXT_NO_LLDP_SYSTEM_DESCR, LLDP_TLV_BASIC_MGMT_SYSTEM_DESCR, entry);
                report_remote_entry_val(TXT_NO_LLDP_PORT_DESCR, LLDP_TLV_BASIC_MGMT_PORT_DESCR, entry);
                report_remote_entry_val(TXT_NO_LLDP_SYSTEM_CAPA, LLDP_TLV_BASIC_MGMT_SYSTEM_CAPA, entry);
                report_remote_entry_val(TXT_NO_LLDP_MGMT_ADDR, LLDP_TLV_BASIC_MGMT_MGMT_ADDR, entry);
#if UNMANAGED_EEE_DEBUG_IF
                report_remote_entry_val(TXT_NO_EEE_STAT, LLDP_TLV_ORG_EEE_TLV, entry);
#endif
#ifndef NDEBUG
                print_str("TTL: ");
                print_dec(entry->rx_info_ttl);
                print_cr_lf();
#endif

                print_cr_lf();
            }
        }
    }

    if(!found) {
        print_txt(TXT_NO_LLDP_NO_ENTRIES);
        print_cr_lf();
    }
}
#endif
#endif

#if TRANSIT_FAN_CONTROL
#if UNMANAGED_FAN_DEBUG_IF
static void handle_fan_control (void)
{
    // Get the chip temperature
    fan_local_status_t status;

    print_str("Temp Max.");
    print_dec_16_right(read_fan_conf_t_max(), 5);
    print_str("C, ");
    print_str("Temp On  ");
    print_dec_16_right(read_fan_conf_t_on (), 5);
    println_str("C.");

    fan_get_local_status(&status);
    print_str("Chip Temp.  ");
    print_dec_16_right(status.chip_temp, 5);
    print_str("C, ");
    print_str("Fan Speed set to ");
    print_dec_16_right(status.fan_speed_setting_pct, 5);
    print_str("%, ");
    print_str("Fan Speed  ");
    print_dec_16_right(status.fan_speed, 5);
    println_str("RPM.");

}
#endif
#endif
#if TRANSIT_THERMAL
static void print_chips_temp (void)
{
    print_txt(TXT_NO_TEMPERATURE);
    print_str(": ");
    parms[0] = phy_read_temp_reg(0);
    parms[1] = (71*parms[0]) / 100;
    if(parms[1] > 135) {
        print_str("Reg: ");
        print_hex_prefix();
        print_hex_w((ushort) parms[0]);
    } else {
        parms[0] = 135 - parms[1];
        print_dec(parms[0]);
        print_str(" C");
    }
#if !defined(LUTON26_L10) && !defined(LUTON26_L16)
    print_str(", ");
    parms[0] = phy_read_temp_reg(12);
    parms[1] = (71*parms[0]) / 100;
    if(parms[1] > 135) {
        print_str("Reg: ");
        print_hex_prefix();
        print_hex_w((ushort) parms[0]);
    } else {
        parms[0] = 135 - parms[1];
        print_dec(parms[0]);
        print_str(" C");
    }
#endif

    print_cr_lf();
}
#endif
#if LUTON_UNMANAGED_CONF_IF
static uchar handle_sys_config (void)
{
    uchar ret;
    mac_addr_t mac_addr;

    ret = retrieve_str_parms();
    if(ret != FORMAT_OK) {
        return ret;
    }

    if(!str_parms[0].len) {
        /* Dump all configurations */
        flash_read_mac_addr(&mac_addr);
        print_mac_addr(mac_addr);
    } else {
        if(cmp_cmd_txt(CMD_TXT_NO_MAC, str_parms[0].str))  {
            /* Update MAC addresses in RAM */
            ret = cmd_retrieve_mac_addr(str_parms[1].str, (uchar *)mac_addr);
            if(ret != FORMAT_OK) {
                return ret;
            }
            if(mac_cmp(mac_addr, mac_addr_0) == 0 ||
                    mac_addr[0] & 0x01)
                return FORMAT_ERROR;
            flash_write_mac_addr(&mac_addr);
#if TRANSIT_LLDP
            lldp_something_changed_local();
#endif
        } else if(cmp_cmd_txt(CMD_TXT_NO_SAVE, str_parms[0].str)) {
            /* Program configurations at RAM to flash */
            flash_read_mac_addr(&mac_addr);
            if(mac_cmp(mac_addr, mac_addr_0) == 0 || mac_cmp(mac_addr, spiflash_mac_addr) == 0 ||
                    mac_addr[0] & 0x01)
                return FORMAT_ERROR;
            flash_program_config();
        }
    }
    return FORMAT_OK;
}
#endif

#if LUTON_UNMANAGED_SWUP
/* ************************************************************************ */
static uchar cmd_cmp (char *s1, char *s2) small
/* ------------------------------------------------------------------------ --
 * Purpose     : Compare a string in RAM with a 0-terminated string in flash memory.
 * Remarks     : s1 points to string in RAM, s2 points to string in flash.
 *               Returns 0 if RAM string is equal to the flash string up till, but
 *               not including the 0-terminator. Otherwise 1 is returned.
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uchar ch1;
    uchar ch2;

    for (;;) {
        ch2 = *s2;
        if (ch2 == 0) {
            return 0;
        }
        s2++;

        ch1 = conv_to_upper_case(*s1++);
        if (ch1 != ch2) {
            return 1;
        }
    }
}
#endif

#endif /* NO_DEBUG_IF */

#ifndef NO_DEBUG_IF
/* ************************************************************************ */
static void handle_v_command (void)
/* ------------------------------------------------------------------------ --
 * Purpose     :
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
#ifndef UNMANAGED_REDUCED_DEBUG_IF
    uchar port_no;
    uchar port_ext;
    ulong value;
    ushort dat;
#endif

    /* software version */
    print_str(sw_version);
#if JUMBO
    print_str(" Jumbo");
#ifndef UNMANAGED_REDUCED_DEBUG_IF
    print_spaces(1);
    print_dec(JUMBO_SIZE);
#endif
#endif
    print_cr_lf();
#if TRANSIT_THERMAL
    /* chip temperature */
    print_chips_temp();
#endif
#ifndef UNMANAGED_REDUCED_DEBUG_IF
    /* chip id */
    print_str(txt_02);
    H2_READ(VTSS_DEVCPU_GCB_CHIP_REGS_CHIP_ID, value);
    print_hex_dw(value);
    print_cr_lf();

    print_txt(TXT_NO_COMPILE_DATE);
    println_str(compile_date);

    /* Info about ports */
    print_str(txt_03);
    for (port_ext = 1; port_ext <= NO_OF_PORTS; port_ext++) {
        port_no = port2int(port_ext);
        if(!phy_map(port_no)
#if MAC_TO_MEDIA
                && !phy_map_serdes(port_no)
#endif
          ) {
            continue;
        }
        print_dec_8_right_2(port_ext);
        print_spaces(3);
        /* mac address, to be modified */
        print_port_mac_addr(port_no);
        print_spaces(2);

        /* miim and phy number */
        print_spaces(1);
        print_dec(phy_map_miim_no(port_no));
        print_spaces(3);
        print_dec_8_right_2(phy_map_phy_no(port_no));

#if 1  /* James: for debug only */
        if (phy_map(port_no)) {
            print_spaces(1);
            dat = phy_read(port_no, 4);
            print_hex_w(dat);
            print_spaces(1);
            dat = phy_read(port_no, 0);
            print_hex_w(dat);
            if (dat == 0x2100) {
                print_str("(100Full)");
            } else if  (dat == 0x2000) {
                print_str("(100Half)");
            }
            print_spaces(3);
            dat = phy_read(port_no, 1);
            print_hex_w(dat);

            if (dat & 0x4) {
                print_str("(Link up)");
            }
        }
#if MAC_TO_MEDIA
        else if(phy_map_serdes(port_no)) {
            print_spaces(1);
            print_str("Link speed ");
            print_dec(vtss_os_get_linkspeed(port_ext));
        }
#endif /* MAC_TO_MEDIA */
#endif /* End James debug */
        print_cr_lf();
    }
#endif
}
#endif

/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
