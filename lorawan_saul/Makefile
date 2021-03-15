# Name of the application
APPLICATION = gnrc_lorawan_saul

USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_lorawan
USEMODULE += gnrc_neterr
USEMODULE += fmt
USEMODULE += saul_default
USEPKG += cayenne-lpp

BOARD ?= demo_nucleo
EXTERNAL_BOARD_DIRS ?= $(CURDIR)/boards

# Uncomment as needed
# USEMODULE += dht
# USEMODULE += tsl4531x

RIOTBASE ?= $(CURDIR)/../RIOT

# Turn on developer helpers
DEVELHELP ?= 1

# use SX1276 by default
DRIVER ?= sx1276

USEMODULE += $(DRIVER)

include $(RIOTBASE)/Makefile.include

# Use `make menuconfig`  to modify the compile time configurations.
# System ---> Networking ---> Configure LoRaWAN MAC
# NOTE: Refer RIOT/examples/gnrc_lorawan/Makefile for additional details.
