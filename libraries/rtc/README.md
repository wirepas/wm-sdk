# Wirepas Library RTC

## Overview
This library allows Wirepas devices to receive a real-time clock from a gateway without specific hardware.

This service handles the RTC time reception messages and maintains the time locally to the node.


## Functions

| Function | Parameters |  Description | Note |
| ------------- | ----  | ---  | ---  |
| RTC_init | void | Initialize the stack state library. | The function is called automatically when importing the library |
| RTC_getUTCTime | rtc_timestamp_t * now | Store expected UTC RTC time in the variable now. | |
| RTC_getLocalTime | rtc_timestamp_t * now | Store expected RTC time with the local timezone in the variable now | |
| RTC_getTimezoneOffsetInSeconds | long * timezoneOffsetInSeconds | Get configured timezone offset of the node | The offset is provided by the gateway |
| RTC_addInitializeCb | on_rtc_initialized callback | Add a new callback to be informed when RTC time is available from network | The callback will be called only once: at the initialization of the time references |
| RTC_removeInitializedCb | on_rtc_initialized callback | Remove an event callback from the list. | |

## How to use

To import the library in an application, the line "RTC=yes" must be present in the makefile of the application and RTC_CBS variable must contain the number of callbacks to be launched at the first rtc message reception (can be 0 if information is not needed by application).


## How does it work

This service gets the global time in UTC timezone from a gateway.
At the moment the time is received, the local clock and the rtc received are taken as references.
RTC reference time is taken as rtc timestamp parameter in the payload and added to the travel time of the packet between the sink and the node.
It is therefore, supposed to be the RTC time at the time where the message is received at the device.

Each time an application needs the RTC time, the library returns the sum of the last known RTC time (received from the network) and the elapsed time since the reception.


## Messages format

TLV encoding is used to allow extension in future.
The messages are composed of:

A version on 2 bytes to ensure the content will be well parsed by the RTC library.

And the content is encoded with TLV with the following content:

| Parameter | Type | Number of bytes | Description |
| ------------- | ----    | ---  | ---  |
| rtc timestamp | unsigned long long | 8 | Timestamp of the current rtc time in the UTC timezone |
| timezone offset | long | 4 | Local timezone offset in seconds |


### TLV encoding

To encode messages with TLV, the type(id) of each parameter must be given as follow:

RTC_ID_TIMESTAMP = 0,  
RTC_ID_TIMEZONE_OFFSET = 1  

Then the length of the value is coded in hexadecimal.
And finally the value itself is encoded in hexadecimal.

For example, a message containing:

The RTC version equal to 1: 0x0001 which is not encoded.
And the TLV encoded content with:

| Parameter | Type | Number of bytes | Value |
| ------------- | ----    | ---  | ---  |
| rtc timestamp | 0 | 8 | 0x000001850aeb3964 |
| timezone offset | 1 | 4 | 0x1c20 (7200s = 2h) |

Each parameter is encoded with TLV as byte(type)(1 byte) - byte(length)(1 byte) - byte(value)(length bytes) individually and their bytes are concatenated.

The message is therefore encoded with TLV in little-endian as:
b'\x01\x00\x00\x08\x64\x39\xeb\x0a\x85\x01\x00\x00\x01\x04\x20\x1c\x00\x00'


## Time accuracy

Due to the multiple steps, the RTC time might lose some precision:

Time is taken from a distant ntp server. The estimated latency may be inaccurate as it is calculated from the round-trip of the asymmetrical exchange. This is similar for all systems relying on NTP, but it must be acknowledged for a better understanding of the time precision.

RTC time is taken at a gateway service level while the travel time calculation starts at the sink device level. From the gateway service to the sink device, we lose some accuracy.
An uncalculated latency may also occur when delivering the RTC data payloads to the nodes.

However, in total, the RTC service gives the time to the nodes in the network with 1 second accuracy.

The [SDK RTC application](../../source/unitary_apps/rtc_app/) example provides a [backend script](../../source/unitary_apps/rtc_app/backend_script/) to calculate at a gateway level the time difference between the nodes' expectation of the RTC and its real value. This script can be used to validate the time accuracy of the RTC system and the time drift of the nodes.


### Use conditions

Natural clock drift occurs in the devices, so the gateway RTC service must periodically send a timestamp to the network to keep the devices synchronized.
