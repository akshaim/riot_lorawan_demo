GNRC LoRaWAN SAUL application
=============================

This application demonstrates a LoRaWAN application along with SAUL. The
application makes us of RIOT's own native LoRaWAN stack called GNRC LoRaWAN and
a generic actuator/sensor abstraction layer in RIOT called SAUL.

For this demo boards will use a mixture of `dht` and `tsl4531x` sensors.
For each board this has to be configured beforehand (either uncommenting the
Makefile or passing e.g `USEMODULE=dht` as environment variable).

Usage
=====

1. Get the device join keys from network server. Run `make menuconfig` inside
    `lorawan_saul` application to modify the compile time configurations. The 
    configurations can be found in System ---> Networking --->
    Configure LoRaWAN MAC
2. Make suitable entries in `Makefile` locates in `lorawan_saul` application to
    add sensors, specify platform etc.
    Alternatively, `USEMODULE` and `DRIVER` can be passed as environment
    variables when calling `make`
3. Wire the sensor(s).
4. Run `make all flash` to compile and flash the application.
5. Run `make term` to access the terminal of the board.
