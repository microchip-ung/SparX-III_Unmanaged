//Copyright (c) 2004-2020 Microchip Technology Inc. and its subsidiaries.
//SPDX-License-Identifier: MIT



#include "common.h"     /* Always include common.h at the first place of user-defined herder files */

#include "taskdef.h"
#include "hwport.h"
#include "vtss_common_os.h"

#if TRANSIT_LLDP
#include "lldp.h"
#endif /* TRANSIT_LLDP */

#if FRONT_LED_PRESENT
#include "ledtsk.h"
#endif

#if TRANSIT_EEE
#include "eee_api.h"
#endif

#if TRANSIT_SNMP
#include "traps.h"
#endif

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
 * Local data
 *
 *
 *
 ****************************************************************************/

#if TRANSIT_EEE
static BOOL eee_port_link_up[ NO_OF_PORTS + 1 ];
#endif /* TRANSIT_EEE */

/****************************************************************************
 *
 * Local Functions
 *
 ****************************************************************************/


/****************************************************************************
 *
 * Public Functions
 *
 ****************************************************************************/


/**
 * Notify applications when link goes up on a port.
 *
 * @note Called by phytsk.c
 */
void callback_link_up (vtss_port_no_t port_no)
{
    port_no = port_no;          /* make compiler happy */

#if FRONT_LED_PRESENT
    led_refresh();
#endif
#if TRANSIT_LLDP
    TASK(SUB_TASK_ID_LLDP_LINK,
         lldp_set_port_enabled(port2ext(port_no), LLDP_TRUE));
#endif /* TRANSIT_LLDP */
#if TRANSIT_EEE
    eee_port_link_up[port_no] = TRUE;
#endif /* TRANSIT_EEE */
}

/**
 * Notify applications when link goes down on a port.
 *
 * @note Called by phytsk.c.
 */
void callback_link_down (vtss_port_no_t port_no)
{
    port_no = port_no;          /* make compiler happy */

#if FRONT_LED_PRESENT
    led_refresh();
#endif
#if TRANSIT_LLDP
    TASK(SUB_TASK_ID_LLDP_LINK,
         lldp_set_port_enabled(port2ext(port_no), LLDP_FALSE));
#endif /* TRANSIT_LLDP */
#if TRANSIT_EEE
    eee_port_link_change(port_no, FALSE);
#endif /* TRANSIT_EEE */
}


#if TRANSIT_EEE
/**
 * Going into LPI will stress the DSP startup as it is not entirely
 * complete when link status becomes good and continues to train afterward.
 *
 * This requirement is noted in clause 22.6a.1 (for 100BASE-TX), 35.3a.1
 * (for 1000BASE-T), and 78.1.2.1.2 (for EEE as a whole).
 *
 * For EEE and it is highly desirable to have the 1 second delay from link
 * coming up prior to sending LPI.
 */
void callback_delayed_eee_lpi(void)
{
    vtss_port_no_t  port_no;

    for (port_no = 0; port_no < NO_OF_PORTS; port_no++) {
        if (eee_port_link_up[port_no]) {
            eee_port_link_up[port_no] = FALSE;
            eee_port_link_change(port_no, TRUE);
        }
    }
}
#endif /* TRANSIT_EEE */



/****************************************************************************/
/*                                                                          */
/*  End of file.                                                            */
/*                                                                          */
/****************************************************************************/

