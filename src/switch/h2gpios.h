//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#ifndef __H2_GPIOS_H__
#define __H2_GPIOS_H__

typedef enum {
    VTSS_GPIO_OUT,
    VTSS_GPIO_IN,
    VTSS_GPIO_IN_INT,
    VTSS_GPIO_ALT_0,
    VTSS_GPIO_ALT_1
} vtss_gpio_mode_t;


extern void  h2_gpio_mode_set (uchar gpio_no, vtss_gpio_mode_t mode);
extern uchar h2_gpio_read (uchar gpio_no);
extern void  h2_gpio_write (uchar gpio_no, uchar value);

extern void  h2_sgpio_enable();
extern uchar h2_sgpio_read(uchar sgpio_no, uchar bit_no);
extern void  h2_sgpio_write(uchar sgpio_no, uchar bit_no, ushort value);

#endif
