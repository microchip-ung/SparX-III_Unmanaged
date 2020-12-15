//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include "common.h"     /* Always include common.h at the first place of user-defined herder files */

#include "vtss_luton26_reg.h"
#include "h2gpios.h"
#include "h2io.h"

/*****************************************************************************
 *
 *
 * Defines
 *
 *
 *
 ****************************************************************************/

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
 * Public data
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


/* ************************************************************************ */
void h2_gpio_mode_set (uchar gpio_no, vtss_gpio_mode_t mode)
/* ------------------------------------------------------------------------ --
 * Purpose     :
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    ulong mask = VTSS_BIT(gpio_no);
    H2_WRITE_MASKED(VTSS_DEVCPU_GCB_GPIO_GPIO_INTR_ENA, 0, mask); /* Disable IRQ */
    switch(mode) {
    case VTSS_GPIO_OUT:
    case VTSS_GPIO_IN:
    case VTSS_GPIO_IN_INT:
        H2_WRITE_MASKED(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(0), 0, mask); /* GPIO mode 0b00 */
        H2_WRITE_MASKED(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(1), 0, mask); /* -"- */
        H2_WRITE_MASKED(VTSS_DEVCPU_GCB_GPIO_GPIO_OE, mode == VTSS_GPIO_OUT ? mask : 0, mask);
        if(mode == VTSS_GPIO_IN_INT)
            H2_WRITE_MASKED(VTSS_DEVCPU_GCB_GPIO_GPIO_INTR_ENA, mask, mask);
        break;
    case VTSS_GPIO_ALT_0:
        H2_WRITE_MASKED(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(0), mask, mask); /* GPIO mode 0b01 */
        H2_WRITE_MASKED(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(1), 0,    mask); /* -"- */
        break;
    case VTSS_GPIO_ALT_1:
        H2_WRITE_MASKED(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(0), 0,    mask); /* GPIO mode 0b10 */
        H2_WRITE_MASKED(VTSS_DEVCPU_GCB_GPIO_GPIO_ALT(1), mask, mask); /* -"- */
        break;
    }
}

/* ************************************************************************ */
uchar h2_gpio_read (uchar gpio_no)
/* ------------------------------------------------------------------------ --
 * Purpose     :
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    ulong mask = VTSS_BIT(gpio_no);
    ulong value;
    H2_READ(VTSS_DEVCPU_GCB_GPIO_GPIO_IN, value);
    return VTSS_BOOL(value & mask);
}

/* ************************************************************************ */
void h2_gpio_write (uchar gpio_no, uchar value)
/* ------------------------------------------------------------------------ --
 * Purpose     :
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    ulong mask = VTSS_BIT(gpio_no);
    if(value) {
        H2_WRITE(VTSS_DEVCPU_GCB_GPIO_GPIO_OUT_SET, mask);
    } else {
        H2_WRITE(VTSS_DEVCPU_GCB_GPIO_GPIO_OUT_CLR, mask);
    }
}

#if defined(LUTON26_L10)
/* ************************************************************************ */
uchar h2_sgpio_read(uchar sgpio_no, uchar bit_no)
/* ------------------------------------------------------------------------ --
 * Purpose     :
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    ulong mask = VTSS_BIT(sgpio_no);
    ulong value;
    H2_READ(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_INPUT_DATA(bit_no), value);
    return VTSS_BOOL(value & mask);
}
#endif

/* ************************************************************************ */
void h2_sgpio_write(uchar sgpio_no, uchar bit_no, ushort value)
/* ------------------------------------------------------------------------ --
 * Purpose     :
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    switch(bit_no) {
    case 0:
        H2_WRITE_MASKED(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_PORT_CONFIG(sgpio_no), value, 0x007);
        break;
    case 1:
        H2_WRITE_MASKED(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_PORT_CONFIG(sgpio_no), value << 3, 0x038);
        break;
    case 2:
        H2_WRITE_MASKED(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_PORT_CONFIG(sgpio_no), value << 6, 0x1c0);
        break;
    case 3:
        H2_WRITE_MASKED(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_PORT_CONFIG(sgpio_no), value << 9, 0xe00);
        break;
    }
}

/* ************************************************************************ */
void h2_sgpio_enable()
/* ------------------------------------------------------------------------ --
 * Purpose     :
 * Remarks     :
 * Restrictions:
 * See also    :
 * Example     :
 ****************************************************************************/
{
    uchar gpio;

    /* Enable GPIO 0-3 as SGPIO pins */
    for (gpio = 0; gpio < 4; gpio++) {
        h2_gpio_mode_set(gpio, VTSS_GPIO_ALT_0);
    }

    /* Enable ports */
    H2_WRITE(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_PORT_ENABLE, LED_PORTS);

    H2_WRITE(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG,
             VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_BMODE_0(3UL) |
             VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_BMODE_1(1) | /* 10Hz */
             VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_BURST_GAP(0x1F) |
             VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_PORT_WIDTH(0x1) |
             /* VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_LD_POLARITY | */
             VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CONFIG_SIO_AUTO_REPEAT);

    /* Setup clock */
    H2_WRITE(VTSS_DEVCPU_GCB_SIO_CTRL_SIO_CLOCK,
             VTSS_F_DEVCPU_GCB_SIO_CTRL_SIO_CLOCK_SIO_CLK_FREQ(0x32));

}
