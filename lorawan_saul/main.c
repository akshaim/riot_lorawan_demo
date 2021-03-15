/*
 * Copyright (C) 2020 Freie Universität Berlin
 *               2020 HAW Hamburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @brief       GNRC LoRaWAN - SAUL example application for RIOT
 *
 * @author      Akshai M    <akshai.m@fu-berlin.de>
 *              José Alamos <jose.alamos@haw-hamburg.de>
 *
 */

/* SAUL registry interfaces */
#include "saul_reg.h"

/* GNRC's network interfaces */
#include "net/gnrc/netif.h"

/* Cayenne LPP interfaces, used to encode payload */
#include "cayenne_lpp.h"

/* System logging header, offers "LOG_*" functions */
#include "log.h"

/* Unit system wait time to complete join procedure */
#define JOIN_DELAY      (10 * US_PER_SEC)

/* Sleep time in seconds */
#define SEND_DELAY           (30 * US_PER_SEC)

static cayenne_lpp_t lpp;

int _send(gnrc_netif_t *netif)
{
    gnrc_pktsnip_t *pkt;
    msg_t msg;

    if (!(pkt = gnrc_pktbuf_add(NULL, lpp.buffer, lpp.cursor,
                                GNRC_NETTYPE_UNDEF))) {
        LOG_INFO("No space in packet buffer\n");
        return 1;
    }

    /* Register for returned packet status */
    if (gnrc_neterr_reg(pkt) != 0) {
        LOG_INFO("Can not register for error reporting\n");
        return 0;
    }

    /* Send LoRaWAN packet
     * This function is a wrapper of gnrc_netapi_send
     */
    gnrc_netif_send(netif, pkt);

    /* Wait for packet status and check */
    msg_receive(&msg);

    if ((msg.type != GNRC_NETERR_MSG_TYPE) ||
        (msg.content.value != GNRC_NETERR_SUCCESS)) {
        LOG_INFO("Error sending packet: (status: %d\n)",
                  (int) msg.content.value);
    }
    else {
        LOG_INFO("Successfully sent packet\n");
    }

    return 0;
}

static void _sense(void)
{
    phydat_t res;
    int i = 0;
    saul_reg_t *dev;

    /* Reset Cayenne lpp buffer and reset the cursor */
    cayenne_lpp_reset(&lpp);

    while ((dev = saul_reg_find_nth(i))) {
        int dim = saul_reg_read(dev, &res);
        phydat_dump(&res, dim);
        switch (dev->driver->type) {
            case SAUL_SENSE_TEMP :
                cayenne_lpp_add_temperature(&lpp, 3, res.val[0]/10);
                break;
            case SAUL_SENSE_HUM :
                cayenne_lpp_add_relative_humidity(&lpp, 4, res.val[0]);
                break;
            case SAUL_SENSE_LIGHT :
                cayenne_lpp_add_luminosity(&lpp, 4, res.val[0]);
                break;
            default:
                break;
            /* More sensors can be added as per requirement */
        }
        i++;
    }
}

/* Join the network */
static netopt_enable_t _join(gnrc_netif_t *netif)
{
    netopt_enable_t en = NETOPT_ENABLE;
    gnrc_netapi_set(netif->pid, NETOPT_LINK, 0, &en, sizeof(en));
    xtimer_usleep(JOIN_DELAY);
    gnrc_netapi_get(netif->pid, NETOPT_LINK, 0, &en, sizeof(en));
    return en;
}

int main(void)
{
    LOG_INFO("LoRaWAN SAUL test application\n");

    gnrc_netif_t *netif;
    /* Try to get a LoRaWAN interface */
    if((netif = gnrc_netif_get_by_type(NETDEV_TYPE_LORA, NETDEV_INDEX_ANY))) {
        LOG_INFO("Couldn't find a LoRaWAN interface");
        return 1;
    }

    /* Wait for node to join NW */
    while(_join(netif) != NETOPT_ENABLE) {}
    LOG_INFO("Device joined\n");

    /* Configure GNRC LoRaWAN using GNRC NetAPI */
    uint8_t port = CONFIG_LORAMAC_DEFAULT_TX_PORT; /* Default: 2 */

    gnrc_netapi_set(netif->pid, NETOPT_LORAWAN_TX_PORT, 0, &port,
                    sizeof(port));

    /* Periodically poll sensor value(s) and transmit */
    while (1) {
        _sense();
        _send(netif);
        xtimer_usleep(SEND_DELAY);
    }

    return 0;
}
