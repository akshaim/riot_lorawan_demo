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

int _send(const kernel_pid_t *interface)
{
    gnrc_pktsnip_t *pkt;
    msg_t msg;

    if (!(pkt = gnrc_pktbuf_add(NULL, lpp.buffer, lpp.cursor,
                                GNRC_NETTYPE_UNDEF))) {
        LOG_INFO("No space in packet buffer\n");
        return 1;
    }

    /* Reset Cayenne lpp buffer and reset the cursor */
    cayenne_lpp_reset(&lpp);

    /* Register for returned packet status */
    if (gnrc_neterr_reg(pkt) != 0) {
        LOG_INFO("Can not register for error reporting\n");
        return 0;
    }

    /* Forward the packet to LoRaWAN MAC for Uplink*/
    gnrc_netif_send(gnrc_netif_get_by_pid(*interface), pkt);

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

static int _sense(void)
{
    phydat_t res;

    saul_reg_t *dev = saul_reg;
    if (dev == NULL) {
        LOG_INFO("No SAUL devices present. Exit... \n");
        assert(false);
        return 1;
    }

    /* TODO: Use saul_reg_find_xxx functions */
    while (dev) {
        int dim = saul_reg_read(dev, &res);
        switch (dev->driver->type) {
            case SAUL_SENSE_TEMP :
                phydat_dump(&res, dim);
                cayenne_lpp_add_temperature(&lpp, 3, res.val[0]/10);
                break;
            case SAUL_SENSE_HUM :
                phydat_dump(&res, dim);
                cayenne_lpp_add_relative_humidity(&lpp, 4, res.val[0]);
                break;
            case SAUL_SENSE_LIGHT :
                phydat_dump(&res, dim);
                cayenne_lpp_add_luminosity(&lpp, 4, res.val[0]);
                break;

            /* More sensors can be added as per requirement */
            }

            dev = dev->next;
        }
        return 0;
}

/* Join the network */
static netopt_enable_t _join(const kernel_pid_t *interface)
{
    netopt_enable_t en = NETOPT_ENABLE;
    gnrc_netapi_set(*interface, NETOPT_LINK, 0, &en, sizeof(en));
    xtimer_usleep(JOIN_DELAY);
    gnrc_netapi_get(*interface, NETOPT_LINK, 0, &en, sizeof(en));
    return en;
}

/* Get interface ID */
static int _get_interface(kernel_pid_t *interface)
{
    gnrc_netif_t *netif = NULL;

    /* Iterate over all network interfaces */
    while ((netif = gnrc_netif_iter(netif))) {
        int _type;
        gnrc_netapi_get(netif->pid, NETOPT_DEVICE_TYPE, 0, &_type,
                        sizeof(_type));
        if (_type == NETDEV_TYPE_LORA) {
            *interface = netif->pid;
            return 0;
        }
    }
    return 1;
}

int main(void)
{
    LOG_INFO("LoRaWAN SAUL test application\n");

    kernel_pid_t interface_d;
    if(_get_interface(&interface_d)) {
        LOG_INFO("Couldn't find a LoRaWAN interface");
        return 1;
    }

    /* Wait for node to join NW */
    while(_join(&interface_d) != NETOPT_ENABLE) {}
    LOG_INFO("Device joined\n");

    /* Configure GNRC LoRaWAN using GNRC NetAPI */
    uint8_t port = CONFIG_LORAMAC_DEFAULT_TX_PORT; /* Default: 2 */

    gnrc_netapi_set(interface_d, NETOPT_LORAWAN_TX_PORT, 0, &port,
                    sizeof(port));

    /* Periodically poll sensor value(s) and transmit */
    while (1) {
        _sense();
        _send(&interface_d);
        xtimer_usleep(SEND_DELAY);
    }

    return 0;
}
