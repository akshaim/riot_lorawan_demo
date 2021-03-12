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
#include "fmt.h"
// #include "dht.h"
// #include "dht_params.h"

#include "phydat.h"
#include "saul_reg.h"
#include <string.h>

#include "cayenne_lpp.h"

static cayenne_lpp_t lpp;

#define DELAY           (10 * US_PER_SEC)
#define MODULE_DHT11

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

static void _print_buffer(const uint8_t *buffer, size_t len, const char *msg)
{
    printf("%s: ", msg);
    for (uint8_t i = 0; i < len; i++) {
        printf("%02X", buffer[i]);
    }
}

int _send(void) 
// int _send(const char *temp_s)
{
    gnrc_pktsnip_t *pkt;
    // char tmp[] = "\x10";
    uint8_t port = CONFIG_LORAMAC_DEFAULT_TX_PORT; /* Default: 2 */

    pkt = gnrc_pktbuf_add(NULL, lpp.buffer,lpp.cursor, GNRC_NETTYPE_UNDEF);

    cayenne_lpp_reset(&lpp);

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

static int _sense(char *_arg1)
{
    phydat_t res;
    saul_reg_t *dev = saul_reg;

    int temp = 0;

    if (dev == NULL) {
            puts("No SAUL devices present");
            return 1;
        }

    while (dev) {
        int dim = saul_reg_read(dev, &res);
        if (strcmp(saul_class_to_str(dev->driver->type), "ACT_SWITCH") &&
            strcmp(saul_class_to_str(dev->driver->type), "SENSE_BTN") &&
            strcmp(saul_class_to_str(dev->driver->type), "SENSE_HUM")) /*temporary filter */
            {   
                printf("\nDev: %s\tType: %s\n", dev->name,
                        saul_class_to_str(dev->driver->type));
                phydat_dump(&res, dim); 
                printf("Dataa : %d, scale %d\n", res.val[0], res.scale);
                temp = res.val[0];
            }
                dev = dev->next;
        }
        cayenne_lpp_add_temperature(&lpp, 3, temp/10);
        size_t n = fmt_s16_dfp(_arg1, temp, -1);

        _print_buffer(lpp.buffer, lpp.cursor, "Result");
        _arg1[n] = '\0';
        printf("Temp Value : %s\n",_arg1);
        return 0;
}

/* join the network */
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
    char arg1[5];

    while(!_join()){}

    puts("Device joined\n");

    /* periodically sense and send sensor values */
    while (1) {
        _sense(arg1);
        printf("Sensed Value : %s\n",arg1);
        _send();
         xtimer_usleep(DELAY);
    }
    
    return 0;
}
