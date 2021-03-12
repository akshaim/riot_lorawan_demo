/*
 * Copyright (C) 2020 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    lorawan_saul_example
 * @brief       LoRaWAN SAUL application
 * @{
 *
 * @file
 * @brief       GNRC LoRaWAN example application for RIOT
 *
 * @author      Akshai M   <akshai.m@fu-berlin.de>
 *
 * @}
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "thread.h"
#include "xtimer.h"

#include "phydat.h"
#include "saul_reg.h"

#include "timex.h"
#include "fmt.h"

#include "board.h"

#include "net/gnrc/netapi.h"
#include "net/gnrc/netif.h"

#include "net/gnrc/pktbuf.h"
#include "net/gnrc/pktdump.h"

#include "net/loramac.h"

#define DELAY           (10 * US_PER_SEC)
#define LORAWAN_QUEUE_SIZE (4U)

static uint8_t interface = 3;

int _send(void)
{
    gnrc_pktsnip_t *pkt;

    msg_t msg;
    char temp_s[]={"250"};
    /* TODO: Replace by cayenne sensor */

    if (!(pkt = gnrc_pktbuf_add(NULL, temp_s, strlen(temp_s), GNRC_NETTYPE_UNDEF))) {
        puts("No space in packet buffer");
        return 1;
    }

    /* register for returned packet status */
    if (gnrc_neterr_reg(pkt) != 0) {
        puts("Can not register for error reporting");
        return 0;
    }
    
    /* Forward the packet to LoRaWAN MAC for Uplink*/
    gnrc_netif_send(gnrc_netif_get_by_pid(interface), pkt);

    /* wait for packet status and check */
    msg_receive(&msg);

    if ((msg.type != GNRC_NETERR_MSG_TYPE) ||
        (msg.content.value != GNRC_NETERR_SUCCESS)) {
        printf("Error sending packet: (status: %d\n)", (int) msg.content.value);
    }
    else {
        puts("Successfully sent packet");
    }

    return 0;
}

static int _sense(void)
{
    phydat_t res;

    saul_reg_t *dev = saul_reg;
    if (dev == NULL) {
        puts("No SAUL devices present. Exit...");
        assert(false);
        return 1;
    }

    /* TODO: Use saul_reg_find_xxx functions */
    while (dev) {
        int dim = saul_reg_read(dev, &res);
        /* TODO: Add the temperature and humidity sensors */
        if (strcmp(saul_class_to_str(dev->driver->type), "ACT_SWITCH") &&
            strcmp(saul_class_to_str(dev->driver->type), "SENSE_BTN"))
            {   
                printf("\nDev: %s\tType: %s\n", dev->name,
                        saul_class_to_str(dev->driver->type));
                phydat_dump(&res, dim); 
                printf("Dataa : %d, scale %d\n", res.val[0], res.scale);
                //temp = res.val[0];
                /* TODO: Load cayenne buffer */
            }
                dev = dev->next;
        }
    return 0;
}   

static netopt_enable_t _join(void)
{
    netopt_enable_t en = NETOPT_ENABLE;
    gnrc_netapi_set(interface, NETOPT_LINK, 0, &en, sizeof(en));
    xtimer_usleep(DELAY);
    gnrc_netapi_get(interface, NETOPT_LINK, 0, &en, sizeof(en));
    return en;
}

int main(void)
{
    puts("LoRaWAN SAUL test application\n");
    //char temp[];

    while(_join() != NETOPT_ENABLE) {} /* Wait for node to join NW */
    puts("Device joined\n");

    /* Configure GNRC LoRaWAN using GNRC NetAPI */
    uint8_t port = CONFIG_LORAMAC_DEFAULT_TX_PORT; /* Default: 2 */

    gnrc_netapi_set(interface, NETOPT_LORAWAN_TX_PORT, 0, &port, sizeof(port));

    /* periodically read temp and humidity values */
    while (1) {
        _sense();
        _send();
         xtimer_usleep(DELAY);
    }

    return 0;
}
