//Copyright (c) 2004-2025 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include "common.h"     /* Always include common.h at the first place of user-defined herder files */
#include "event.h"
#include "vtss_luton26_reg.h"
#include "vtss_common_os.h"
#include "h2io.h"
#include "h2.h"
#include "phytsk.h"
#include "phymap.h"
#include "phydrv.h"
#include "timer.h"
#include "hwport.h"
#include "h2gpios.h"
#include "h2sdcfg.h"
#include "h2pcs1g.h"
#include "h2txrx.h"
#if FRONT_LED_PRESENT
#include "ledtsk.h"
#endif
#include "print.h"
#include "misc1.h"
#include "misc2.h"
#if TRANSIT_LLDP
#include "lldp.h"
#endif /* TRANSIT_LLDP */
#if TRANSIT_VERIPHY
#include "veriphy.h"
#endif
#if USE_HW_TWI
#include "i2c_h.h"
#endif
#if USE_SW_TWI
#include "i2c.h"
#endif

/*****************************************************************************
 *
 *
 * Defines
 *
 *
 *
 ****************************************************************************/

/* define states of state machine */
#define PORT_DISABLED                   0
#define SET_UP_SPEED_MODE_ON_PHY        1
#define WAITING_FOR_LINK                2
#define LINK_UP                         3
#define SERDES_SIG_SET_UP_MODE         99
#define SERDES_SET_UP_MODE            100
#define SERDES_WAITING_FOR_LINK       101
#define SERDES_LINK_UP                102

/* define periods in granularity of 10 msec */
#define POLL_PERIOD_FOR_LINK           10 /* 100 msec */

#define MAX_THERMAL_PROT_TIME 10 /* 10 sec */
#define MAX_JUNCTION_TEMP 122

#define SFP_TXDISABLE_PIN  15

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

/* Link up/down state */
static port_bit_mask_t phy_link_up_mask;

/* Current state of each PHY/port state machine */
static uchar xdata phy_state [MAX_PORT];

const uchar gpio_ports[] = PHY_GPIO_PORT;

/* Flag for activate polling of PHYs */
static bit poll_phy_flag;

/* Flag for activate polling of PHYs temperature monitor */
static uchar start_thermal_protect_timer = FALSE;
static ushort thermal_protect_cnt = 0;
static port_bit_mask_t led_err_stat = 0;

/* Let all PHYs initially be powered down/disabled */
static port_bit_mask_t phy_enabled = ALL_PORTS;

#if TRANSIT_THERMAL
static uchar max_protect_temp = MAX_JUNCTION_TEMP;
#endif

#if MAC_TO_MEDIA
uchar mac_if_changed[MAX_PORT];
#endif

#define POLARITY_DETECT_FOR_10HDX_MODE
/****************************************************************************
 *
 * Local functions
 *
 ****************************************************************************/

/**
 * Initialize all PHYs after a power up reset.
 */
static void phy_init (void)
{
    vtss_port_no_t port_no;

    phy_link_up_mask = 0;

    delay(MSEC_30);

    phy_pre_reset(0);
#if !defined(LUTON26_L10) && !defined(LUTON26_L16)
    phy_pre_reset(12);
#elif !defined(LUTON26_L10)
#ifdef LUTON26_L16_QSGMII_EXT_PHY
    phy_pre_reset(12);
#endif /* LUTON26_L16_QSGMII_EXT_PHY */
#else
    if (phy_map(24)) {//using MDC/MDIO
        phy_pre_reset(24); /* L10, port24 uses external single  PHY */
    }
    if (phy_map(25)) {//using MDC/MDIO
        phy_pre_reset(25); /* L10, port25 uses external single PHY*/
    }        
#endif

#if !defined(LUTON26_L10) && !defined(LUTON26_L16) 
    if (phy_map(25)) { //using MDC/MDIO
        phy_pre_reset(25); /*L25, port25 uses external single */
    }   
#endif

    delay(MSEC_2000); // delay_1(2000);

    phy_page_std(0);
#if !defined(LUTON26_L10) && !defined(LUTON26_L16)
    phy_page_std(12);
#endif

    for (port_no = MIN_PORT; port_no < MAX_PORT; port_no++) {
        if (phy_map(port_no)) {
            if(TEST_PORT_BIT_MASK(port_no, &phy_enabled)) {
                phy_setup(port_no);
                phy_post_reset(port_no);
            } else {
                phy_power_down(port_no);
            }

        }
        #if MAC_TO_MEDIA
            mac_if_changed[port_no] = 1;
        #endif
    }
#if TRANSIT_THERMAL //de-spec
    phy_init_temp_mode_regs(0);
#if !defined(LUTON26_L10) && !defined(LUTON26_L16)
    phy_init_temp_mode_regs(12);
#endif
#endif
}

/**
 * Initialize all SFP gpio controls after a power up reset.
 */
static void sfp_init (void)
{
#if defined(LUTON26_L25UN)
    h2_gpio_mode_set(11, VTSS_GPIO_OUT);
    h2_gpio_write(11, 1); //set GPIO11(I2C_MUX)=1 to select SFP1
    h2_sgpio_write(27, 1, 0); // tx disable
#elif  defined(LUTON26_L25)
    phy_page_std(SFP_MODULE_GPIO_PORT);
    /* Configure  PHY GPIO 2, 3 as input, and 4 as output */
    phy_page_gp(SFP_MODULE_GPIO_PORT);
    /* Step 1. setup GPIO 2, 3, 4 is controled by GPIO function */
    phy_write_masked(SFP_MODULE_GPIO_PORT, 13, 0x00fc, 0x00fc);
    /* Step 2, setup input and output pins */
    phy_write_masked(SFP_MODULE_GPIO_PORT, 17, 0x0010, 0x001c);
    phy_page_std(SFP_MODULE_GPIO_PORT);

    /* Use normal GPIO function */
    h2_gpio_mode_set(SFP_TXDISABLE_PIN, VTSS_GPIO_OUT);
    h2_gpio_write(SFP_TXDISABLE_PIN, 1); // reset GPIO
#elif defined(LUTON26_L10)
    h2_sgpio_write(30, 0, 0); // Set tx_enable
    h2_sgpio_write(31, 0, 0); // Set tx_enable
#endif
}

static uchar phy_init_state (vtss_port_no_t port_no)
{

    if (phy_map(port_no)) {
        if (TEST_PORT_BIT_MASK(port_no, &phy_enabled)) {
            phy_write_masked(port_no, 18, 0x0040, 0x0040);
            return SET_UP_SPEED_MODE_ON_PHY;
        } else {
            return PORT_DISABLED;
        }
    }
#if MAC_TO_MEDIA
    else if (phy_map_serdes(port_no)) {
        return SERDES_SIG_SET_UP_MODE;
    }
#endif
    else {
        return PORT_DISABLED;
    }
}

/*
** Return link mode
** bit 1:0 = 00:   No link
** bit 1:0 = 01: 1000 Mbit/s
** bit 1:0 = 10:  100 Mbit/s
** bit 1:0 = 11:   10 Mbit/s
**
** bit 4 = 0: half duplex
** bit 4 = 1: full duplex
**
** bit 5 = 0: link partner doesn't support pause frames
** bit 5 = 1: link partner does support pause frames

** bit 7:6 = 00: No EEE advertised
** bit 7:6 = 01: 100Base EEE advertised
** bit 7:6 = 10: 1000Base EEE advertised
** bit 7:6 = 11: Reserved
*/
/* Fixme: Tune the two functions to fit VSC8522/12 */
static uchar get_link_mode (vtss_port_no_t port_no)
{
    int eee_advertisement;
    uchar link_mode;

    link_mode = phy_get_speed_and_fdx(port_no);

    /* check if link partner supports pause frames */
    if (phy_read(port_no, 5) & 0x0400) {
        link_mode |= LINK_MODE_PAUSE_MASK;
    }

    // 802.3az says EEE is supported only under full-duplex mode
    if (!(link_mode & LINK_MODE_FDX_MASK))
        return link_mode;

    // Get EEE advertisement
    eee_advertisement = phy_mmd_rd(port_no, 7, 61);// 7.61 EEE Link Partner Advertisement.

    //Table 96, in data sheet
    if (eee_advertisement & 0x2) {
        link_mode |= LINK_MODE_POWER_MASK_100BASE;
    } else if (eee_advertisement & 0x4) {
        link_mode |= LINK_MODE_POWER_MASK_1000BASE;
    }

    return link_mode;
}

static void do_link_up (vtss_port_no_t port_no)
{
    VTSS_UPDATE_MASKS_DEBUG();
    vtss_update_masks();
    callback_link_up(port_no);
}

static void do_link_down (vtss_port_no_t port_no)
{
    WRITE_PORT_BIT_MASK(port_no, 0, &phy_link_up_mask);

    if (phy_map(port_no)) {
        /* Do any set-up of PHY due to link going down */
        phy_do_link_down_settings(port_no);
    }

    h2_setup_port(port_no, LINK_MODE_DOWN);

    callback_link_down(port_no);
}

/**
 * State machine for copper phy. Monitor PHY and set up H2 port.
 */
static void handle_phy (vtss_port_no_t port_no)
{
    uchar  link_mode;
#ifdef POLARITY_DETECT_FOR_10HDX_MODE
    ushort phy_data;
#endif
    switch (phy_state[port_no]) {

    case PORT_DISABLED:
        break;

    case SET_UP_SPEED_MODE_ON_PHY:

        /* Update register 4 with 10/100 advertising, plus pause capability */
        phy_write(port_no, 4, 0x05e1);
#if 0
        if (phy_read(port_no, 4) != 0x05e1) {
            print_str("pause disable");
            print_cr_lf();
        }
#endif
        /*  Update register 9 with 1000 Mbps advertising */
        phy_write(port_no, 9, PHY_REG_9_CONFIG);
        /* Restart auto-negotiation */
        phy_restart_aneg(port_no);

        /* shift state */
        phy_state[port_no] = WAITING_FOR_LINK;
        break;

    case WAITING_FOR_LINK:
        if (!poll_phy_flag) {
            return;
        }
        /* Check if link is up */
        if (phy_link_status(port_no)) {

            WRITE_PORT_BIT_MASK(port_no, 1, &phy_link_up_mask);


            /* Update switch chip according to link */
            link_mode = get_link_mode(port_no);


            h2_setup_port(port_no, link_mode);

            /* Do any set-up of PHY due to link going up */
            phy_do_link_up_settings(port_no, link_mode, 0);

            /* shift state */
            phy_state[port_no] = LINK_UP;
            do_link_up(port_no);
#ifdef POLARITY_DETECT_FOR_10HDX_MODE
            if ((phy_get_speed_and_fdx(port_no) & (LINK_MODE_SPEED_MASK|LINK_MODE_FDX_MASK)) == LINK_MODE_HDX_10) {//10HDX
                //println_str("Link is up: checking polarit...");
                phy_data = phy_read(port_no, 28);
                if (phy_data&0x0C00) { //if POL_INVERSE bits[11:10] is set : polarity swapped
                    phy_write(port_no, 31, 0x2a30);  // switch to test-register page
                    phy_write_masked(port_no, 5, 0x6, 0x6);    // write[2:1]={11}: force inversion 
                    phy_write(port_no, 31, 0x0);    // switch to std page
                } else { //bit[11:10] is clear
                    phy_write(port_no, 31, 0x2a30);  // switch to test-register page
                    phy_write_masked(port_no, 5, 0x4, 0x6);    // write[2:1]={10}: force normal polarity
                    phy_write(port_no, 31, 0x0);   // switch to std page
                }

                /* disable polarity detection */ 
                phy_write_masked(port_no, 0x12, 0x0010, 0x0010);
            }   
#endif //POLARITY_DETECT_FOR_10HDX_MODE
        }
        break;

    case LINK_UP:
        if (poll_phy_flag) {
            /* Check if link has been down */
            if (!phy_link_status(port_no)) {
                phy_state[port_no] = WAITING_FOR_LINK;
                do_link_down(port_no);
#ifdef POLARITY_DETECT_FOR_10HDX_MODE
                //print_str("Link is Down...");
                //print_hex_w(phy_get_speed_and_fdx(port_no));
                //print_cr_lf(); 
                phy_write(port_no, 31, 0x2a30);  // switch to test-register page
                phy_write_masked(port_no, 5, 0x0, 0x6);    // write[2:1]={00}:Unforce polarity
                phy_write(port_no, 31, 0x0); // switch to std page

                /* enable polarity detection */ 
                phy_write_masked(port_no, 0x12, 0x0000, 0x0010);
#endif // POLARITY_DETECT_FOR_10HDX_MODE
           }
        }
        break;


    default:
        break;
    }
}

#if  MAC_TO_MEDIA
#define SFP_TRAN_CODE      0x06

#if TRANSIT_SFP_DETECT
#if USE_SW_TWI
static uchar i2c_read(uchar addr, uchar reg)
{
    uchar ack;
    uchar temp;

    i2c_start();
    ack = i2c_send_byte(addr << 1);
    ack = i2c_send_byte(reg & 0xff);

    i2c_start();
    ack = i2c_send_byte((addr << 1) | 1);

    /* read LS byte */
    temp = i2c_get_byte(0);

    i2c_stop();

    return temp;
}
#endif /* USE_SW_TWI */

/* Enable I2C access and perform a read */
static uchar sfp_i2c_read(uchar dev, vtss_port_no_t port_no, uchar addr, uchar *const value, uchar cnt)
{
    uchar ret;
#if USE_SW_TWI
    uchar c;
#endif
    if(!phy_map_serdes(port_no))
        return 0;
#if USE_HW_TWI
    i2c_tx(dev, &addr, 1);
    ret = i2c_rx(dev, value, cnt);
#endif
#if USE_SW_TWI
    ret = 0;
    for(c = 0; c < cnt; c++) {
        *(value + c) = i2c_read(dev, (addr+c));
        if(*(value + c) != 0xff) ret |= 1;
    }
#if 0
    for(c = 0; c < cnt; c++) {
        print_hex_b(*(value + c));
        print_ch(' ');
    }
    print_str(", ret = ");
    print_dec(ret);
    print_cr_lf();
#endif
#endif
    return ret;
}

#define CuSFP_DET 1

static uchar sfp_detect(vtss_port_no_t port_no)
{
#if TRANSIT_LLDP || LOOPBACK_TEST
    uchar *buf = &rx_packet[0];
#else
    uchar buf[30];
#endif


#if CuSFP_DET
    ulong vendor;
    const char *CuSFPModel[2] = { "AXGT-R1T", "AXGT-R15" };

    /* I2C slave address 0x50 is the SFP EEPORM address and 0x56 is the SFP copper phy */
    if (sfp_i2c_read(0x50, port_no, 12, &buf[0], 1)) {//MSA EERPOM byte[12], rate in unit of 100Mbits/s
        //print_str("sfp rate:");
        //print_dec(buf[0]);   print_cr_lf();
        if (buf[0] >= 1 && buf[0] <= 2) {        /* 100Mb capabilities --> 100FX */
            return MAC_IF_100FX;
        } else if (buf[0] >= 25) { /* 2500Mb capabilities */
            //print_str(">2.5G SFP");
            //print_cr_lf();
            return MAC_IF_SERDES_2_5G;/* Currently default for 2.5G operation*/
        }    
        else if (buf[0] >= 10 && buf[0] <= 22) { /* 1Gb capabilities -->  Copper SFP or Serdes 1000Base-X */
            if (sfp_i2c_read(0x50, port_no, 40, &buf[0], 8)) {   /* Try to recognize the SFP module via EEPROM */
                buf[8] = '\0';
                if (xmemcmp((char *)CuSFPModel[0],(char*)buf, 8) == 0) {        /* Axcen SFP_CU_SGMII */
                    return MAC_IF_SGMII;
                } else if (xmemcmp((char *)CuSFPModel[1], (char*)buf, 8) == 0) { /* Axcen SFP_CU_SERDES */
                    return MAC_IF_SERDES;
                }
            }

            /* Vender is unknown, then read PHY registers */
            /* Try to determine the SFP host mode (serdes/sgmii) via the Phy settings */
            if (sfp_i2c_read(0x56, port_no, 0, &buf[0], 8)) {
                /* The Phy is there, find the vendor id and if the phy is in SGMII or Serdes mode */
                vendor = (((buf[4] << 8) | buf[5]) << 6) | ((((buf[6] << 8) | buf[7]) >> 10) & 0x3f);
                if (vendor == 0x3f1) { /* VTSS phy in SERDES mode */
                    return MAC_IF_SERDES;
                } else if (vendor == 0x5043) { /* Marvell phy */
                    if (sfp_i2c_read(0x56, port_no, 0, &buf[0], 22) &&
                            sfp_i2c_read(0x56, port_no, 22, &buf[0], 22)) {
                        if (buf[20] == 0x11) {    /* SGMII interface to host */
                            return MAC_IF_SGMII;
                        } else {
                            return MAC_IF_SERDES;
                        }
                    }
                }
                return MAC_IF_SERDES; /* PHY vendor is unknown */
            } else {
                return MAC_IF_SERDES; /* No phy --> SERDES SFP */
            }
        } else {
            if(sfp_i2c_read(0x50, port_no, SFP_TRAN_CODE, &buf[0], 1)) {
                if ((buf[0] & 0xf) == 0)
                    return MAC_IF_100FX;
            }
        }
    } else {
#if USE_HW_TWI
        i2c_tx(0, &buf[0], 1);
        i2c_rx(0, &buf[0], 1);
        delay(MSEC_20);
#endif
    }
    return MAC_IF_SERDES; //MAC_IF_NONE;
#endif
#if !CuSFP_DET
    if(sfp_i2c_read(0x50, port_no, SFP_TRAN_CODE, &buf[0], 1)) {

        if(&buf[0] == 0xff) {
#if USE_HW_TWI
            /* workaround of h2 twi function */

            uchar value=0;
            i2c_tx(0, &value, 1);
            i2c_rx(0, &value, 1);
            delay(MSEC_20);
#endif
            sfp_i2c_read(0x50, port_no, SFP_TRAN_CODE, &buf[0], 1);
        }
        if ((buf[0] & 0xf) == 0)
            return MAC_IF_100FX;
    }
    return MAC_IF_SERDES;
#endif
}
#endif /* TRANSIT_SFP_DETECT */

/* ************************************************************************ */
static uchar serdes_port_sfp_detect(vtss_port_no_t port_no)
/* ------------------------------------------------------------------------ --
 * Purpose     :
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
#if !defined(LUTON26_L10) && !defined(LUTON26_L16)
#if defined(LUTON26_L25)
    ushort reg15g;
#endif
#endif
    uchar  present_l;
    /* do signal detect */
    if(phy_map_serdes(port_no)) {
#if !defined(LUTON26_L10) && !defined(LUTON26_L16)
#if defined(LUTON26_L25UN)
        present_l = h2_sgpio_read(25, 1);  //sfp1;
#else
        phy_page_gp(SFP_MODULE_GPIO_PORT);
        reg15g = phy_read(SFP_MODULE_GPIO_PORT, 15);
        phy_page_std(SFP_MODULE_GPIO_PORT);
        present_l = test_bit_16(1, &reg15g);
#endif
#elif defined(LUTON26_L10)
        switch(port_no) {
        case 24:
            present_l = h2_sgpio_read(26, 1);  //sfp0;
            break;
        case 25:
            present_l = h2_sgpio_read(27, 1);  //sfp1;
            break;
        default:
            return 0;
        }
#endif
    }
    return present_l ? 0:1;
}

/* ************************************************************************ */
static void handle_serdes(vtss_port_no_t port_no)
/* ------------------------------------------------------------------------ --
 * Purpose     : State machine for Serdes port. Monitor and set up switch port.
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uchar  mac_if, lm, sfp_existed;

    switch (phy_state[port_no]) {

    case PORT_DISABLED:
        break;

    case SERDES_SIG_SET_UP_MODE:
        phy_state[port_no] = SERDES_SET_UP_MODE;
        break;
    case SERDES_SET_UP_MODE:
        sfp_existed = serdes_port_sfp_detect(port_no);
#if TRANSIT_SFP_DETECT
        if(sfp_existed) {
            mac_if = sfp_detect(port_no);
        } else {
            mac_if = phy_map_miim_no(port_no) ; //MAC_IF_SERDES;
        }

        if(mac_if != phy_map_miim_no(port_no)) {
            phy_map_serdes_if_update(port_no, mac_if);
            mac_if_changed[port_no] = 1;
#if 0
            print_str("mac if ");
            print_dec(mac_if);
            print_cr_lf();
#endif
        }
#endif

        mac_if = phy_map_miim_no(port_no);

        if(mac_if_changed[port_no]) {
            vtss_serdes_mode_t media_if;
            ulong              addr;
#if !defined(LUTON26_L10) && !defined(LUTON26_L16)
            addr = 0x1;
#elif defined(LUTON26_L10)
            switch(port_no) {
            case 24:
                addr = 0x2;
                break;
            case 25:
                addr = 0x1;
                break;
            }
#endif
            switch(mac_if) {
            case MAC_IF_SERDES_2_5G:
                media_if = VTSS_SERDES_MODE_2G5;
                break;
            case MAC_IF_SERDES:
                media_if = VTSS_SERDES_MODE_1000BaseX;
                break;
            case MAC_IF_SGMII:
                media_if = VTSS_SERDES_MODE_SGMII;
                break;
            case MAC_IF_100FX:
                media_if = VTSS_SERDES_MODE_100FX;
                break;
            default:
                media_if = VTSS_SERDES_MODE_1000BaseX;
            }

            h2_sd6g_cfg_change(media_if, addr);

            mac_if_changed[port_no] = 0;

            /* Luton26 supports 1G-F(AN mode) and 100Full SFP module(Force mode) */
            if (mac_if == MAC_IF_SERDES_2_5G || mac_if == MAC_IF_SERDES || mac_if == MAC_IF_SGMII || mac_if == MAC_IF_100FX) {
                h2_pcs1g_setup(port_no, mac_if);
            }

            if (mac_if == MAC_IF_SERDES || mac_if == MAC_IF_SGMII) {
                h2_pcs1g_clause_37_control_set(port_no);
            } else {
                /* 100 Full mode and Auto SFP mode,  do nothing */
            }
        }
#if !defined(LUTON26_L10) && !defined(LUTON26_L16)
#if defined(LUTON26_L25UN)
        h2_sgpio_write(27, 1, 1); // tx enable
#else
        h2_gpio_write(SFP_TXDISABLE_PIN, 0); // set GPIO
#endif
#endif

        phy_state[port_no] = SERDES_WAITING_FOR_LINK;

        break;

    case SERDES_WAITING_FOR_LINK:
        if (poll_phy_flag) {

            sfp_existed = serdes_port_sfp_detect(port_no);

            mac_if = phy_map_miim_no(port_no);

            if (mac_if == MAC_IF_SERDES_2_5G) {
                lm = h2_pcs1g_2_5g_link_status_get(port_no);
            } else if (mac_if == MAC_IF_SERDES || mac_if == MAC_IF_SGMII) {
                lm = h2_pcs1g_clause_37_status_get(port_no);
            } else {
                lm = h2_pcs1g_100fx_status_get(port_no);
            }

            if(lm != LINK_MODE_DOWN) {
                /* Link up */
#if 0
                print_str("port = ");
                print_dec(port2ext(port_no));
                print_str(", lm = ");
                print_hex_prefix();
                print_hex_b(lm);
                print_cr_lf();
#endif

                if(mac_if == MAC_IF_SGMII) {
                    ulong tgt = VTSS_TO_DEV(port_no);
                    if((lm & LINK_MODE_SPEED_AND_FDX_MASK) == LINK_MODE_FDX_1000) {
                        H2_WRITE_MASKED(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_MODE_CFG(tgt), 0x00000011, 0x00000011);
                    } else if((lm & LINK_MODE_SPEED_AND_FDX_MASK) == LINK_MODE_FDX_2500) {
                        H2_WRITE_MASKED(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_MODE_CFG(tgt), 0x00000011, 0x00000011);
                    } else {
                        if(lm & LINK_MODE_FDX_MASK) {
                            H2_WRITE_MASKED(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_MODE_CFG(tgt), 0x00000001, 0x00000011);
                        } else {
                            H2_WRITE_MASKED(VTSS_DEV_CMN_MAC_CFG_STATUS_MAC_MODE_CFG(tgt), 0x00000000, 0x00000011);
                        }
                    }
                }

                h2_setup_port(port_no, lm);
                WRITE_PORT_BIT_MASK(port_no, 1, &phy_link_up_mask);
                do_link_up(port_no);
                phy_state[port_no] = SERDES_LINK_UP;
            } else {
                if(!sfp_existed) {
                    if(mac_if != MAC_IF_SGMII) {
#if !defined(LUTON26_L10) && !defined(LUTON26_L16)
#if defined(LUTON26_L25UN)
                        h2_sgpio_write(27, 1, 0); //tx disable
#else
                        h2_gpio_write(SFP_TXDISABLE_PIN, 1); // reset GPIO
#endif
#endif
                    }
                    phy_state[port_no] = SERDES_SET_UP_MODE;
                }
            }

        }
        break;

    case SERDES_LINK_UP:
        if (poll_phy_flag) {
            mac_if = phy_map_miim_no(port_no);
#if TRANSIT_SFP_DETECT
            sfp_existed = serdes_port_sfp_detect(port_no);  // Module is unplug
            if(!sfp_existed) {
                phy_state[port_no] = SERDES_SET_UP_MODE;
#if !defined(LUTON26_L10) && !defined(LUTON26_L16)
#if defined(LUTON26_L25UN)
                h2_sgpio_write(27, 1, 0); // 1: tx_disable
#else
                h2_gpio_write(SFP_TXDISABLE_PIN, 1); // reset GPIO
#endif
#endif
                h2_pcs1g_clock_stop(port_no);
                do_link_down(port_no);
                break;
            }
#endif
            if (mac_if == MAC_IF_SERDES_2_5G) {
                lm = h2_pcs1g_2_5g_link_status_get(port_no);
	        } else if (mac_if == MAC_IF_SERDES || mac_if == MAC_IF_SGMII) {
                lm = h2_pcs1g_clause_37_status_get(port_no);
            } else {
                lm = h2_pcs1g_100fx_status_get(port_no);
            }
            if(lm == LINK_MODE_DOWN) {
                phy_state[port_no] = SERDES_SET_UP_MODE;
#if !defined(LUTON26_L10) && !defined(LUTON26_L16)
#if defined(LUTON26_L25UN)
                h2_sgpio_write(27, 1, 0); // 1: tx_disable
#else
                h2_gpio_write(SFP_TXDISABLE_PIN, 1); // reset GPIO
#endif
#endif
                h2_pcs1g_clock_stop(port_no);
                do_link_down(port_no);
            }
        }
        break;

    default:
        break;
    }
}
#endif /* MAC_TO_MEDIA */


/****************************************************************************
 *
 * Public functions
 *
 ****************************************************************************/


void phy_timer_10 (void)
{
    static uchar poll_phy_timer = 0;

    if (++poll_phy_timer >= 10) {
        poll_phy_timer = 0;
        poll_phy_flag = TRUE;
    }

}


uchar phy_check_all (void)
{
    vtss_port_no_t  port_no;
    uchar           error = FALSE;

    for (port_no = MIN_PORT; port_no < MAX_PORT; port_no++) {
        if (phy_map(port_no)) {
#if PHY_ID_CHECK
            if (phy_read(port_no, 2) != PHY_OUI_MSB) {
                error = TRUE;
            }
#endif
        }
    }

    return error;
}


uchar phy_tsk_init (void)
{
    vtss_port_no_t port_no;

    phy_init(); // Phy Init must come after the h2_init_port, because the port clocks must be enabled.
    sfp_init();

    if (phy_check_all() != FALSE) {
        return TRUE;
    }

    for (port_no = MIN_PORT; port_no < MAX_PORT; port_no++) {
        phy_state[port_no] = phy_init_state(port_no);
    }

    return FALSE;
}


void phy_tsk (void)
{
    uchar  port_no;

    for (port_no = MIN_PORT; port_no < MAX_PORT; port_no++) {
        if (phy_map(port_no)) {
            handle_phy(port_no);
        }
#if MAC_TO_MEDIA
        else if (phy_map_serdes(port_no)) {
            handle_serdes(port_no);
        }
#endif
    }

    poll_phy_flag = FALSE;
}


uchar phy_get_link_mode_raw (vtss_port_no_t port_no)
{

#if MAC_TO_MEDIA
    uchar mac_if;
#endif
    uchar lm = LINK_MODE_DOWN;

    if (TEST_PORT_BIT_MASK(port_no, &phy_link_up_mask)) {
        if(phy_map(port_no))
            lm = get_link_mode(port_no);
#if MAC_TO_MEDIA
        else if (phy_map_serdes(port_no)) {
            mac_if = phy_map_miim_no(port_no);
            if (mac_if == 0xb) {
                lm = h2_pcs1g_2_5g_link_status_get(port_no);
            } else if (mac_if == 9 || mac_if == 2) {
                lm = h2_pcs1g_clause_37_status_get(port_no);
            } else {
                lm = h2_pcs1g_100fx_status_get(port_no);
            }
        }
#endif
    }

    return lm;
}


uchar phy_get_link_state (vtss_port_no_t port_no)
{
    if (TEST_PORT_BIT_MASK(port_no, &phy_link_up_mask)) {
        return TRUE;
    }

    return FALSE;
}


port_bit_mask_t phy_get_link_mask (void)
{
    port_bit_mask_t link_mask;
    vtss_port_no_t port_no;
    uchar port_ext;

    link_mask = 0;
    for (port_ext = MIN_PORT; port_ext < MAX_PORT; port_ext++) {
        port_no = port2int(port_ext);
        if (phy_get_link_state(port_no)) {
            WRITE_PORT_BIT_MASK(port_no, 1, &link_mask);
        }
    }
    return link_mask;
}


#if TRANSIT_VERIPHY

/****************************************************************************
 *
 *
 * VeriPHY
 *
 *
 ****************************************************************************/

/**
 * Run veriphy on all ports.
 *
 * @todo Run only on PHY ports.
 */
void phy_veriphy_all (void)
{
    uchar                   port_no;
    uchar                   j;
    uchar                   errors;
    /* veriphy_parms_t xdata veriphy_parms [MAX_PORT]; */
    veriphy_parms_t xdata   *veriphy_parms = (veriphy_parms_t *) rx_packet;
    BOOL                    all_done = FALSE;
    BOOL                    done;
    ushort                  timeout = 1500;
    port_bit_mask_t         port_mask;


    // Start VeriPhy for all ports
    port_mask = ALL_PORTS;//The dual-media ports 20-23 might not pass the VeriPHY with RJ45 connected

    for (port_no = MIN_PORT; port_no < MAX_PORT; port_no++) {
        if (TEST_PORT_BIT_MASK(port_no, &port_mask) && phy_map(port_no)) {
            /* Read PHY id to determine action */
            veriphy_start(port_no); // Starting veriphy for selected port(s)
        }
    }

    // Polling Verphy until Veriphy is done
    j = 0;
    for (port_no = MIN_PORT; port_no < MAX_PORT; port_no++) {
        if (TEST_PORT_BIT_MASK(port_no, &port_mask) && phy_map(port_no)) {
            done = FALSE;
            while (!done) { 		
                veriphy_run(port_no, (veriphy_parms + port_no), &done);
                j++;
                if (j > 10) {
                    break;//timeout: 10*10*10ms
                }
                delay(10);
            }
        }
    }
    delay(10);

    errors = 0;
    for (port_no = MIN_PORT; port_no < MAX_PORT; port_no++) {
        if (TEST_PORT_BIT_MASK(port_no, &port_mask) && phy_map(port_no)) {
            /* Status is valid */
            if(!(veriphy_parms+port_no)->flags) {
                errors = 1;
            }
            /* Status for each pair */
            for (j = 0; j < 4; j++) {
                if((veriphy_parms+port_no)->stat[j]) {
                    /* Set Error Leds */
                    errors = 1;
                }
            }
            if(errors) break;
        }
    }
    delay(10);


    if (errors) {
        for (j = 0; j < 10; j++) {
            for (port_no = MIN_PORT; port_no < MAX_PORT; port_no++) {
                if (TEST_PORT_BIT_MASK(port_no, &port_mask) && phy_map(port_no)) {
#if defined(LUTON26_L25UN)
                    sgpio_output(port_no, VTSS_SGPIO_BIT_0, VTSS_SGPIO_STATE_OFF);
                    sgpio_output(port_no, VTSS_SGPIO_BIT_1, VTSS_SGPIO_STATE_ON);
#else
                    sgpio_output(port_no, VTSS_SGPIO_BIT_1, VTSS_SGPIO_STATE_OFF);
                    sgpio_output(port_no, VTSS_SGPIO_BIT_0, VTSS_SGPIO_STATE_ON);
#endif
                }
            }
            delay(MSEC_100);
            for (port_no = MIN_PORT; port_no < MAX_PORT; port_no++) {
                if (TEST_PORT_BIT_MASK(port_no, &port_mask) && phy_map(port_no)) {
                    sgpio_output(port_no, VTSS_SGPIO_BIT_0, VTSS_SGPIO_STATE_OFF);
                    sgpio_output(port_no, VTSS_SGPIO_BIT_1, VTSS_SGPIO_STATE_OFF);
                }
            }
            delay(MSEC_100);
        }
    }
    delay(10);


    for (port_no = MIN_PORT; port_no < MAX_PORT; port_no++) {
        if (TEST_PORT_BIT_MASK(port_no, &port_mask) && phy_map(port_no)) {
            sgpio_output(port_no, VTSS_SGPIO_BIT_0, VTSS_SGPIO_STATE_OFF);
            sgpio_output(port_no, VTSS_SGPIO_BIT_1, VTSS_SGPIO_STATE_OFF);
        }
    }
	
}
#endif /* TRANSIT_VERIPHY */

#if TRANSIT_ACTIPHY

/****************************************************************************
 *
 *
 * ActiPHY
 *
 *
 ****************************************************************************/

/**
 * Run ActiPHY on all PHY ports.
 *
 */
void phy_actiphy_all (void)
{
    vtss_port_no_t port_no;    

	/* ActiPHY init*/
    println_str("ActiPHY enable for all ports.");
    for (port_no = MIN_PORT; port_no < MAX_PORT; port_no++) {
        if(phy_map(port_no)) {
            phy_page_tp(port_no);
            phy_write_masked(port_no, 12, 0x0000, 0xfc00);
            phy_write_masked(port_no, 24, 0x2000, 0x2000);
            phy_page_std(port_no);
#if 1
            phy_write_masked(port_no, 28, 0x0040, 0x0040);//actiphy en
#else
            phy_write_masked(port_no, 28, 0x0000, 0x0040);//actiphy dis
#endif
            delay_1(1);
            }
    }   
}    
#endif /* TRANSIT_ACTIPHY */


#if TRANSIT_FAN_CONTROL || TRANSIT_THERMAL
ushort phy_get_sys_temp (void)
{
    ushort temp_0 = 0, temp_1 = 0;
    temp_0 = phy_read_temp_reg(0);
#if !defined(LUTON26_L10) && !defined(LUTON26_L16)
    temp_1 = phy_read_temp_reg(12);
#else
    temp_1 = 0;
#endif

    return MAX(temp_1, temp_0);
}
#endif


#if TRANSIT_THERMAL
void phy_handle_temperature_protect()
{
    ushort temperatue, temp;
    uchar  port_ext;
    uchar  port_no;
    uchar  temp_id;

    uchar enable;

    const uchar code protect_port[] = {3, 7, 13, 19, 23, 26};
    const uchar code protect_temp[] = {0, 1, 2,  3,  4,  5}; /* Threshhold is 122C ~ 128C*/

    enable = 1;

    temperatue = phy_get_sys_temp();

    temp_id = 0xff;
    if(temperatue >= max_protect_temp) {  /* Temp. over threshold */
        start_thermal_protect_timer = 1;  /* Start the protectioin timer */
        thermal_protect_cnt = MAX_THERMAL_PROT_TIME; /* Refresh the timer value to default value 10 sec */

        temp = temperatue - max_protect_temp; /* Find the power down ports */
        for (temp_id = 0; temp_id < ARRAY_LENGTH(protect_temp) - 1; temp_id++) {
            if(temp >= protect_temp[temp_id] && temp < protect_temp[temp_id + 1])
                break;
        }

        print_str("temp. is ");
        print_dec(temperatue);
        print_cr_lf();

#if 0
        print_str("cur temp. diff = ");
        print_dec(temp);
        print_ch('(');
        print_dec(protect_temp[temp_id]);
        print_str("), power down ports = ");
        print_dec(protect_port[temp_id]);
        print_cr_lf();
#endif
    }

    if(temp_id != 0xff && temp_id < ARRAY_LENGTH(protect_temp)) {
        for (port_ext = 1; port_ext <= NO_OF_PORTS; port_ext++) {
            uchar half = protect_port[temp_id] / 2;
            uchar half_port = (MAX_PORT + MIN_PORT) / 2;
            port_no = port2int(port_ext);
            if(phy_map(port_no)
#if MAC_TO_MEDIA
                    || phy_map_serdes(port_no)
#endif
              ) {
                if(port_ext < half_port) { /* power down ports */
                    if(port_ext <= half) {
                        if(!TEST_PORT_BIT_MASK(port_no, &led_err_stat)) {
                            if(enable) {
                                phy_set_enable(port_no, FALSE);
                            }
                            WRITE_PORT_BIT_MASK(port_no, 1, &led_err_stat);
#if FRONT_LED_PRESENT
                            led_state_set(port_ext, VTSS_LED_EVENT_OVERHEAT, VTSS_LED_MODE_BLINK_YELLOW);
#endif
                        }
                    }
                } else {
                    if(port_ext < (half_port + (protect_port[temp_id] - half))) {
                        if(!TEST_PORT_BIT_MASK(port_no, &led_err_stat)) {
                            if(enable) {
                                phy_set_enable(port_no, FALSE);
                            }
                            WRITE_PORT_BIT_MASK(port_no, 1, &led_err_stat);
#if FRONT_LED_PRESENT
                            led_state_set(port_ext, VTSS_LED_EVENT_OVERHEAT, VTSS_LED_MODE_BLINK_YELLOW);
#endif
                        }
                    }
                }
            }
        }
    }

    if(!start_thermal_protect_timer) { /* timer is stopped. */
        for (port_ext = 1; port_ext <= NO_OF_PORTS; port_ext++) {
            port_no = port2int(port_ext);
            if(phy_map(port_no)
#if MAC_TO_MEDIA
                    || phy_map_serdes(port_no)
#endif
              ) {
                if(TEST_PORT_BIT_MASK(port_no, &led_err_stat)) { /* power on the ports */
                    if(enable) {
                        phy_set_enable(port_no, TRUE);
                    }
                    WRITE_PORT_BIT_MASK(port_no, 0, &led_err_stat);
#if FRONT_LED_PRESENT
                    led_state_set(port_ext, VTSS_LED_EVENT_OVERHEAT, VTSS_LED_MODE_NORMAL);
#endif
                }
            }
        }
    }

#if 0
    if(led_err_stat) {
        print_str("over heat led stat ");
        print_hex_prefix();
        print_hex_dw(led_err_stat);
        print_cr_lf();
    }
#endif
}


void phy_temperature_timer_1sec (void)
{
    if (start_thermal_protect_timer)
    {
        thermal_protect_cnt--;

        if (thermal_protect_cnt == 0)
        {
            start_thermal_protect_timer = 0;
        }
    }
}


#endif /* TRANSIT_THERMAL */


#if TRANSIT_THERMAL
void phy_set_enable (vtss_port_no_t port_no, uchar status)
{
    if (TEST_PORT_BIT_MASK(port_no, &phy_enabled) != status) {
        WRITE_PORT_BIT_MASK(port_no, status, &phy_enabled);

        if (status) {
            phy_write_masked(port_no, 0, 0, 0x800);
            phy_restart(port_no);
        } else {
#if TRANSIT_LLDP
            /* make sure to transmit shutdown LLDPDU before link goes away */
            lldp_pre_port_disabled(port2ext(port_no));
#endif /* TRANSIT_LLDP */
            phy_state[port_no] = PORT_DISABLED;
            do_link_down(port_no);
            if (phy_map(port_no)) {
                phy_power_down(port_no);
            }
#if  MAC_TO_MEDIA
            else if (phy_map_serdes(port_no)) {
                uchar mac_if = phy_map_miim_no(port_no);
                if (mac_if == 9 || mac_if == 2) {
                    h2_pcs1g_clock_stop(port_no);
                }
            }
#endif
        }
    }
}
#endif


/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/
