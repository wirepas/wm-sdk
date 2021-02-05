.. image:: wirepas_logo_2016_slogan_RGB.png

==============================================================
            Ruuvi sensor application brief
==============================================================


This document is a shorter version of the application note available for
Licensees under the Application SDK.

Introduction
------------

This application serves as an example on how to transport sensor data on
top of Wirepas Mesh (WM) network and is designed for use with Ruuvi Tags.

This application must run on a Ruuvi tag due to the sensors it interacts
with. Apart from that, the application relies on WM single MCU primitives
which follow the radio and platform agnostic philosophy of WM.


Functional overview
--------------------

This application queries measurements from sensors (environmental and
accelerometer) on Ruuvi boards and sends them at regular intervals toward the
Sink.

Data sent to the Sink is encoded as Type Length Value (TLV) format.
This application decides which sensors to read based on the configuration
present in the AppConfig.


Application datagrams
----------------------

On each reading period, the application sends active sensor measurements
towards the Sink. Packets are sent from endpoint 11 to endpoint 11.

The APDU is a succession of sensors measurement formated to TLV format.
The sensor data is only present in the payload if it has
been enabled through the configuration.

The formated data looks like this :

::

    [Type - 1byte]      [Length -  1byte] [Value - N bytes little endian]
    [0x01: Counter]     [0x02]            [uint16_t counter]
    [0x02: Temperature] [0x04]            [int32_t  temperature]
    [0x03: Humidity]    [0x04]            [uint32_t humidity]
    [0x04: Pressure]    [0x04]            [uint32_t pressure]
    [0x05: Accel X]     [0x04]            [int32_t  accel X]
    [0x06: Accel Y]     [0x04]            [int32_t  accel Y]
    [0x07: Accel Z]     [0x04]            [int32_t  accel Z]


The sensors range and format are the following:

::

    Counter : [0-65535], incremented by 1 each period.
    Temperature : [-40;+85°C], 1LSB is 0.01°C in 2's complement.
    Humidity : [0;100%], 1LSB is 1/1024 % of relative humidity (1% is 1024).
    Pressure : [300;1100 hPa], 1LSB is 0.01 Pascal.
    Accel on X axis : [-2g;+2g], 1LSB is 1mg in 2's complement.
    Accel on Y axis : [-2g;+2g], 1LSB is 1mg in 2's complement.
    Accel on Z axis : [-2g;+2g], 1LSB is 1mg in 2's complement.


For example, if only temperature and acceleration on X axis are enabled,
the payload will look like this :

::

    [0x01][0x02][Counter 2Bytes][0x02][0x04][Temp 4Bytes][0x05][0x04][X accel 4Bytes]


Application configuration
--------------------------

By default the application is configured to send the measurement off all
sensors every 10 seconds. This can be configured using the AppConfig
mechanism offered by the Wirepas stack.

The AppConfig commands are formated using TLV format. Any combination of
valid commands can be added to the AppConfig payload.

Selecting sensor data - SENSORS_ENABLE
----------------------------------------------------

Each sensor can be enabled/disabled individually through the SENSORS_ENABLE
command. If a sensor is disabled it does not appear in the data measurement
ADPU.

The SENSORS_ENABLE command has the following format :

::

    [Type - 1byte] [Length - 1byte] [Value - 6bytes]
    [0x01]         [0x06]           [Temp][Humi][Press][Acc X][Acc Y][Acc Z]


Value 1 enables the sensor; Value 0 disables it.

For example to enable the 3 accelerometer axis and disable other sensors,
the SENSORS_ENABLE command will look like this:

::

    [Type - 1byte] [Length -  1byte] [Value - 6bytes]
    [0x01]         [0x06]            [0x00][0x00][0x00][0x01][0x01][0x01]


Modifying sensor rate - SENSORS_PERIOD
----------------------------------------

The SENSORS_PERIOD command can be used to configure at what rate sensors
measurement are made and data sent towards the Sink.

The SENSORS_PERIOD command has the following format :

::

    [Type - 1byte] [Length - 1byte] [Value - 1-2 bytes]
    [0x02]         [0x01 or 0x02]   [sensor rate (little endian)]


For example to set the sensors rate to 5 minutes, the SENSORS_PERIOD
command will look like this:

::

    [Type - 1byte] [Length -  1byte] [Value - 2 bytes]
    [0x02]         [0x02]            [0x2C][0x01]


*Document version: 1.1A*
