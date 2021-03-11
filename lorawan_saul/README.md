GNRC LoRaWAN SAUL application
=============================

This application demonstrates a LoRaWAN application along with SAUL. The
application makes us of RIOT's own native LoRaWAN stack called GNRC LoRaWAN and
a generic actuator/sensor abstraction layer in RIOT called SAUL.

Usage
=====

1. Get the device join keys from network server. Run `make menuconfig` inside
    `lorawan_saul` application to modify the compile time configurations. The 
    configurations can be found in System ---> Networking --->
    Configure LoRaWAN MAC
2. Make suitable entries in `Makefile` locates in `lorawan_saul` application to
    add sensors, specify platform etc.
3. Wire the sensor(s).
4. If necessary,modify the Driver specific configurations using `make menuconfig`.
5. Run `make all flash term` to flash the firmware.




