/**
    @mainpage Positioning Application

    @section intro Introduction

    This application serves as a reference mechanism on how to interact
    with the Wirepas Positioning Engine.

    @section modules Modules
    The application is structured in several modules:

    app_settings: contains the settings structures which are initialized
    to default values and might change on runtime according to configuration
    messages

    event: contains the logic behind each stack callback

    measurements: handles the internal data structures where beacons are
    stored and retrieve from.

    overhaul: handles commands sent to the node

    scheduler: upkeeps the state of the application at every access cycle tick.


    @section functional-desc Basic functional description

    The positoining application serves as an example on how to acquire network
    beacon data and exchange it with the network, for WPE consumption.

    This application works in two distinct modes:
    - NRLS
    - AUTO SCAN

    @subsection nrls-mode NRLS mode

    In the non router long sleep (NRLS) mode, the application is sleeping for
    certain periods of time. After each wake up, the application packs the
    scan results into a message and forwards it towards a sink.

    Before resuming sleep, the application waits for the message ack to arrive.
    If it takes too long (see amount of access cycle periods in scheduler.h),
    the application times out and forces a sleep.

    In case the data is sent successfully, the application will still check
    for the reception of the app config. If it has been acquired and decoded,
    the application will then proceed to the next sleep (or reboot in case
    of an OTAP command)

    Note: In NRLS mode the app config is always transmitted after a sleep period
    since the device is disassociating from the network.

    @subsection auto-mode Auto scan

    In the auto scan mode, the device can act as an anchor or a tag. In the
    case of a tag, the scan will be triggered explicitly by the stack at the
    interval rate defined in its settings.

    The application does not care about message acknowledgements and if the
    app config has been received, it will wait for the next scan period.


    In the anchor case, the application will not perform any explicit scans.
    Instead, the application will register the network scan and network beacon
    callbacks and when the stack triggers a scan, a message will be sent.

    @section ble_beacon BLE beacon

    The application allows sending of an Eddystone and/or iBeacon.
    It is possible to customize the beacon content by changing the
    beacon payload defined in ble_beacon.c.

    There are three different settings for the BLE beacon:

        OFF (0): BLE beacons are not sent
        Always ON (65535):  BLE beacons are sent all the time
        When OFFILE (1 to 1800): BLE beacons are sent when the node is outside the WM
        coverage (the activation/deactivation is according to the positioning update interval)

    The BLE default functionality is enabled in config.mk or through application configuration message.
    The beacon payload shall to be defined at compile time and cannot be changed afterwards.

    @section uicr UICR configuration

    To ease the customization of a node at flashing time the UICR configuration (nRF528xx specific) is introduced.

    Node configuration parameters are set with the following register content:

        NRF_UICR->CUSTOMER[0]  : 0x080 :  NODE ADDRESS  (32 bit, only the lower 24 LSB considered)
        NRF_UICR->CUSTOMER[1]  : 0x084 :  NETWORK ADDRESS (32 bit, only the lower 24 LSB considered)
        NRF_UICR->CUSTOMER[2]  : 0x088 :  NETWORK CHANNEL (8 bit)
        NRF_UICR->CUSTOMER[2]  : 0x089 :  NODE ROLE (8 bit)
        ...
        NRF_UICR->CUSTOMER[31]  :  shall contain PERSISTENT_MAGIC_KEY for the UICR register to be considered valid.

    It is possible to add other configuration parameters to the UICR by extending the configuration
    structure persistent_settings_t.

    @subsection device-settings Device settings and OTAP

    The settings of the application can be set through the usual Wirepas APIs,
    such as the Remote API, and a subset of them through custom app config
    payloads.

    The customization through the app config is necessary when the devices are
    operating in the NRLS mode. In the NRLS mode, the devices are connected
    to the network for very short periods of time, which means that remote
    API commands will not reach the devices.

    When setting the app config, it is important to pay attention to the
    device class. The device class will identify the start of a payload for
    those devices. If there is no matching between the class the device
    is currently configured to and the one present in the app config, the
    payload will be ignored.

    Regarding the payload, any combination of valid commands can go into it.
    However, our recommendation is that you always define the default class
    and at least the operation and measurement rate interval.

    As an example, assuming you want to specify settings for the device class A (0xFA),
    you would need to have an application paylod that would start or contain the
    following bytes:

        0xFA [Payload for command X] [Payload for command Y] [Another class definition]...

    @subsubsection device-classes Device classes

    The device class is set at build time according to the value in
    config.mk. The value in the config.mk is ensured to be valid and will default
    to the default class value (see app_settings.h).

    The following device classes are available:

        default
        A
        B
        C
        D
        E
        F

    @subsection payload-desc Application payload

    The asset tracking application is sending a payload (Little Endian)
    which consists of two parts.
    The first part consists of a sequence number and the second part
    consists of M measurement messages.



    Each, which depends
    on its type.

    In summary, the APDU consists of:

        [ Sequence Number (2 unsigned bytes)] [ [ Message 0 (N bytes) ] ... [ Message M (N bytes)] ]

    Each M-th measurement message has a variable number of bytes that depends
    on its type. All messages are structured as:

        Type (1 unsigned bytes)
        Length (1 unsigned byte)
        Fields based on message type (N bytes)


    Currently, the application is only sending RSS measurements, whose definition
    is described in the RSS measurement message section.

    @subsubsection rss Message type 0: RSS measurement

    The RSS measurements are sent in the payload with type = 0 and consist of
    an array of two fields for each observed Wirepas Mesh beacon.

    The array of measurements contains K (K=length/5) elements with the
    following structure:

        Node address: 4 unsigned bytes
        Scaled RSS: 1 unsigned byte

    Note that from version 5.x the network address is 4 bytes versus
    3 bytes in 4.x version. It is possible to send the legacy 4.x measurement
    packet format by setting 'use_legacy_measurement=yes' in config.mk. As this will only
    work correctly if it in ensured that all node addresses are below 0xFFFFFF it is only safe
    to be used if positioning app is compiled with SDK version 4.x.

    The scaled RSS field is a positive representation of the received signal
    strength in a 0.5 dBm resolution.

    The value in dBm is computed as:

        Scaled RSS * -1 / 2 [dBm]

    The full message structure looks like:

        [ Type | Length | [ [ Addr 1 | RSS 1 ] ... [ Addr N | RSS N ] ]

    @section debug Debug level
    The application supports multiple logging levels. The highest the value
    the more verbose the application will be.

    The current configured levels are:

        LVL_CRITICAL 50
        LVL_ERROR    40
        LVL_WARNING  30
        LVL_INFO  20
        LVL_DEBUG 10

    The level increments allows you to specify a custom level.

    Any prints above the configured logging level will be printed out.

    @section remarks Remarks

    In the current NRLS implementation the node will always receive the
    app config after a sleep period. Hence, the application logic is built
    around this fact.

    @section requirements Stack version requirement
    This application requires at least WM >= 3.4

*/

/**
    @file app.c
    @brief header file for app.c.

    @copyright: Wirepas Oy 2018
*/

#ifndef POS_H
#define POS_H

#include <stdbool.h>

void Pos_init(bool on_boot);
#endif /*POS_H*/