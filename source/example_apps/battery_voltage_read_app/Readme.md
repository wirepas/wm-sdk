# Battery voltage read application

## Application scope

This application is an example which shows how to integrate in your own
code a node's battery voltage read process and how to send to the backend.

## Application API

The "Battery voltage read" example measures the battery voltage once per minute and
calculates the averaged value, on the last 60 samples, once per hour. It then sends it
to the WNT backend within a Wirepas Massive data packet. This type of data uses
reserved endpoints (i.e **248 as source endpoint** and **255 as destination endpoint**)
to be able from the cloud to easily distingush it from other packets.<br>
**Please note that this is a feature only available as a backward compatibility option
to be able to have the voltage reporting when in WNT Client.** <br>
The payload is encoded using **cbor (Concise Binary Object Representation)**.
The format is given below:<br>
    `<map> <key> <major type additional information> <value> <primitive>` <br>
where <br>
* *map*, *major type additional information* and *primitive* are cbor encoding
defined symbols expressed on one byte each
* *key* is the data type identifier (i.e voltage and value is 2) expressed
on one byte
* *value* is the voltage expressed in millivolts on two bytes (MSB first) <br>

To better understand the packet payload encoding it can be parsed for instance
on this [website](http://cbor.me/).

An example of a full battery voltage packet payload (in hexadecimal format)
is given below with its decoding: <br>
`bf 02 19 0b 3a ff` <br>
`|| || || || || ||` <br>
`|| || || || || vv --- primitive` <br>
`|| || || vv vv --- value (2874 millivolts)` <br>
`|| || vv --- major type additional information` <br>
`|| vv --- key (voltage)` <br>
`vv --- map` <br>
Optionally the result can be printed on a node UART interface if application logging
is enabled.

## Application architecture

The "Battery voltage read" application implements a user application which runs
in a cooperative manner with the Wirepas Massive stack which is in charge of handling
all Wirepas network related tasks. The [diagram](https://wirepas.github.io/wm-sdk/)
displayed on the linked page illustrates how the user application interacts with the
Wirepas Massive stack. <br>

This application depends on:
* the Single-MCU API to configure the node's network parameters and start the Wirepas
Massive stack
* the SDK HAL *HAL_VOLTAGE* to control the low level battery voltage reading operation
(i.e ADC configuration and handling) and optionally *HAL_UART* to log the measuremement
on a UART interface.
* the SDK *scheduler* library to schedule the voltage read task 
* the SDK *debug_log* utility to print the voltage measurement on a UART interface
* the third-party *tinycbor* library to encode data in cbor format

## Testing
To test the application two options are available: <br>
1. standalone (i.e only one node UART interface directly connected to a computer)
* check that the "APP_PRINTING=yes" and "HAL_VOLTAGE=yes" lines are left uncommented
in the *makefile* file located in the application folder
* compile the code and flash it to a node
* connect the node's UART interface to a computer
* start a serial terminal utility (e.g PUTTY or minicom) with the following parameters
(logger default configuration)
    * BAUDRATE = 115200 by default
    * DATA BITS = 8
    * STOP BITS = 1
    * PARITY = none
    * FLOW CONTROL = none
2. network configuration (i.e one or more node(s) connected to a Wirepas Massive network)
* a minimal network must be set which consists of at least one node running
this code and one sink (flashed with the dualmcu application) connected to a gateway which
has access to a WNT backend.
* the diagnostics must then be enabled from WNT client.
* the battery voltage measurement can be visualized within WNT client or the full message
payload with the Wirepas Terminal tool if the sink is connected to a computer via a USB cable
(if available on the hardware).
    * Please note the "show diagnostics" option (View -> Show Diagnostics) in Wirepas Terminal
must be enabled to be able to see the voltage diagnostics packet.

## Customization
By default the application:
* produces one voltage measurement per hour
(average calculated on 60 samples with a one minute acquisition period). The average
filter size can be adjusted so as the sampling frequency which will both define
the measurement send interval.
To do that the `MEA_INTERVAL` and `AVG_FILTER_SIZE` constants can be edited in
the *app.c* source file.
Furtuhermore, the application logs can be enabled or disabled by uncommenting/commenting
the "APP_PRINTING=yes" line in the application's *makefile* file.
* assumes the battery voltage can be measured on the MCU VDD pin (Nordic Semiconductor)
or Analog VDD pin (Silicon Labs). This can be changed by editing the corresponding
*MCU_voltageGet()* function declared in *voltage.c* file located in
**\<path to SDK\>/wm-sdk/mcu/\<MCU type\>/hal/** folder.
