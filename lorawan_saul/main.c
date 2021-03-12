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

// #include "timex.h"
// #include "fmt.h"
// #include "dht.h"
// #include "dht_params.h"

#include "phydat.h"
#include "saul_reg.h"
#include <string.h>

#define DELAY           (10 * US_PER_SEC)
#define MODULE_DHT11

#define MSG           "{ '%s': [ { 'ts': %llu000, 'values':{'%s': %d, '%s': %d}}]}"

#include "board.h"

#include "net/gnrc/netapi.h"
#include "net/gnrc/netif.h"

#include "net/gnrc/pktbuf.h"
#include "net/gnrc/pktdump.h"
#include "net/gnrc/netreg.h"
#include "net/loramac.h"

#define LORAWAN_QUEUE_SIZE (4U)

// static gnrc_netif_t _netif;
static uint8_t interface = 3;

int _send(void)
{
    gnrc_pktsnip_t *pkt;
    char temp_s[]={"250"};

    uint8_t port = CONFIG_LORAMAC_DEFAULT_TX_PORT; /* Default: 2 */

    pkt = gnrc_pktbuf_add(NULL, temp_s, strlen(temp_s), GNRC_NETTYPE_UNDEF);

    /* register for returned packet status */
    if (gnrc_neterr_reg(pkt) != 0) {
        puts("Can not register for error reporting");
        return 0;
    }
    
    gnrc_netapi_set(interface, NETOPT_LORAWAN_TX_PORT, 0, &port, sizeof(port));

    gnrc_netif_send(gnrc_netif_get_by_pid(interface), pkt); /* Forward the packet to LoRaWAN MAC for Uplink*/

    /* wait for packet status and check */
    msg_t msg;
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

static int _sense(char *temp)
{
    phydat_t res;
    saul_reg_t *dev = saul_reg;

    if (dev == NULL) {
            puts("No SAUL devices present");
            return 1;
        }

    while (dev) {
        int dim = saul_reg_read(dev, &res);
        if (strcmp(saul_class_to_str(dev->driver->type), "ACT_SWITCH") &&
            strcmp(saul_class_to_str(dev->driver->type), "SENSE_BTN"))
            {   
                printf("\nDev: %s\tType: %s\n", dev->name,
                        saul_class_to_str(dev->driver->type));
                phydat_dump(&res, dim); 
                printf("Dataa : %d, scale %d\n", res.val[0], res.scale);
                temp = res.val[0];
            }
                dev = dev->next;
        }
    size_t n = fmt_s16_dfp(temp_s, temp, -1);
    temp_s[n] = '\0';
}   
static int _join(void)
{
    uint8_t enable = 1;
    int status = 0;
    gnrc_netapi_set(interface, NETOPT_LINK, 0, &enable, sizeof(enable));
    xtimer_usleep(DELAY);
    gnrc_netapi_get(interface, NETOPT_LINK, 0, &status, sizeof(status));
    return status;
}

int main(void)
{
    puts("LoRaWAN SAUL test application\n");
    char temp[];

    while(!_join()){} /* Wait for node to join NW */
    puts("Device joined\n");

    /* periodically read temp and humidity values */
    while (1) {
        _sense(temp)
        _send();
         xtimer_usleep(DELAY);
    }

    return 0;
}
