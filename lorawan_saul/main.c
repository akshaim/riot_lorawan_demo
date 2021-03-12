/*
 * Copyright (C) 2020 Freie Universität Berlin
 *               2020 HAW Hamburg
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
 *              José Alamos <jose.alamos@haw-hamburg.de>
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
#include "fmt.h"
// #include "dht.h"
// #include "dht_params.h"

#include "phydat.h"
#include "saul_reg.h"
#include <string.h>

#include "cayenne_lpp.h"

#define DELAY           (10 * US_PER_SEC)

#include "board.h"

#include "net/gnrc/netapi.h"
#include "net/gnrc/netif.h"

#include "net/gnrc/pktbuf.h"
#include "net/gnrc/pktdump.h"
#include "net/gnrc/netreg.h"
#include "net/loramac.h"

#include "cayenne_lpp.h"

#define DELAY           (10 * US_PER_SEC)
#define LORAWAN_QUEUE_SIZE (4U)

static cayenne_lpp_t lpp;

// static gnrc_netif_t _netif;
static uint8_t interface = 3;

/*
static void _print_buffer(const uint8_t *buffer, size_t len, const char *msg)
{
    printf("%s: ", msg);
    for (uint8_t i = 0; i < len; i++) {
        printf("%02X", buffer[i]);
    }
}
*/

int _send(void) 
// int _send(const char *temp_s)
{
    gnrc_pktsnip_t *pkt;
    // char tmp[] = "\x10";
    uint8_t port = CONFIG_LORAMAC_DEFAULT_TX_PORT; /* Default: 2 */

    cayenne_lpp_reset(&lpp);
    msg_t msg;

    if (!(pkt = gnrc_pktbuf_add(NULL, lpp.buffer, lpp.cursor, GNRC_NETTYPE_UNDEF))) {
        puts("No space in packet buffer");
        return 1;
    }

    cayenne_lpp_reset(&lpp);

    /* register for returned packet status */
    if (gnrc_neterr_reg(pkt) != 0) {
        puts("Can not register for error reporting");
        return 0;
    }
    
    gnrc_netapi_set(interface, NETOPT_LORAWAN_TX_PORT, 0, &port, sizeof(port));

    gnrc_netif_send(gnrc_netif_get_by_pid(interface), pkt); /* Forward the packet to LoRaWAN MAC for Uplink*/

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
            puts("No SAUL devices present");
            return 1;
        }

    while (dev) {
        int dim = saul_reg_read(dev, &res);
        if (!strcmp(saul_class_to_str(dev->driver->type), "SENSE_TEMP"))  /*temporary filter */
            {   
                phydat_dump(&res, dim); 
                printf("Dataa : %d, scale %d\n", res.val[0], res.scale);
                cayenne_lpp_add_temperature(&lpp, 3, res.val[0]/10);
            }
            dev = dev->next;

            //to do add additional sensors
        }
        return 0;
}

/* join the network */
static netopt_enable_t _join(void)
{
    netopt_enable_t enable = 1;
    gnrc_netapi_set(interface, NETOPT_LINK, 0, &enable, sizeof(enable));
    xtimer_usleep(DELAY);
    gnrc_netapi_get(interface, NETOPT_LINK, 0, &enable, sizeof(enable));
    return enable;
}

int main(void)
{
    puts("LoRaWAN SAUL test application\n");

    while(_join() != NETOPT_ENABLE){}

    puts("Device joined\n");

    /* periodically sense and send sensor values */
    while (1) {
        _sense();
        //printf("Sensed Value : %s\n",arg1);
        _send();
         xtimer_usleep(DELAY);
    }
    
    return 0;
}
