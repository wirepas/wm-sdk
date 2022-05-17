# Table of content

- [Introduction](#introduction)
- [Getting started](#getting-started)
- [Settings](#settings)
- [Motion](#motion)
- [Directed-advertiser and mini-beacons](#directed-advertiser-and-mini-beacons)
- [Other features](#other-features)
- [References](#references)

# Introduction

The positioning application (PosApp) implements the complete application required for the Wirepas Positioning System (WPS). 
The application is targeted to run on asset tracking tags and anchors and can be used as is or customised further. The application core is 
the positioning library (PosLib) which provides all needed functionality to collect WPS measurements. In addition the application has the following modules:
* **settings:** responsible for handling PosLib settings
* **motion:** responsible for determining the motion state of the node

The application can run on both nRF52 and EFR32 chipset families as supported by Wirepas Massive (WM) stack. Note that for EFR32BG22 only
tag functionality (i.e. non-router network role) is supported. The list of boards supported is visible in `config.mk`. 

The application version numbering scheme is (sdk major).(sdk minor).(sdk maintenance).(application version). 
 
In the following it is assumed that the reader is familiar with developing an application using the Wirepas SDK. It is recommended 
to read also [[1]](#References) and [[2]](#References).

# Getting started

PosApp is a standard SDK application and the compile time settings are set in `config.mk`.  These can be changed by either editing `config.mk` or
by providing them as compile time parameters.

The following application settings can be controlled through `config.mk` parameters:
| Parameter             | Description                                                    |
|-----------------------|----------------------------------------------------------------|
| default_network_address |  Node network address |
| default_network_channel | Node network channel | 
| default_network_cipher_key | Network encryption key. (recommended to be set) |
| default_network_authen_key | Network authentication key. (recommended to be set) |
| app_specific_area_id |  Application area ID. Shall be unique in the network |
| default_role | Router (headnode) LE: 0x01, Router LL: 0x11, Non-router (subnode) LE: 0x02, Advertiser: 0x04 (note that anchors are always routers while tags non-routers and battery powered nodes shall always be low-energy)| 
| default_poslib_device_class |  Positioning class for node 0xFF...0xF9 (see `poslib_class_e` in `poslib.h`)|
| default_poslib_device_mode | Tag NRLS: 1, Tag autoscan: 2, Anchor autoscan: 3, Anchor oportunistic: 4 (`poslib_mode_e` in `poslib.h`) |
| default_update_period_static_s | Measurement rate if node is static or motion monitoring not supported | 
| default_update_period_dynamic_s | Measurement rate if node is dynamic. (not used if set to 0 or motion monitoring not supported)| 
| default_update_period_offline_s | Measurement rate when the node is outside network coverage. (not used if set to 0) |
| default_bletx_mode |  (OFF: 0, always ON: 1, when outside coverage: 3 (see `poslib_ble_mode_e` in `poslib.h`)|
| default_bletx_activation_delay_s | BLE beacons activation delay when outside WM coverage |
| default_bletx_type | (see Eddystone: 1, iBeacon 2, both: 3 (see `poslib_ble_type_e` in `poslib.h`) | 
| default_bletx_interval_ms | Default update period for BLE beacons. range 100 ... 60000 milliseconds | 
| default_bletx_power | BLE beacon transmit power. (ceiled to maximum supported)|
| default_voltage_report |  enables/disable voltage sampling ans sedning in PosLib : yes/no (recommended yes) |
| default_debug_level| Logging level. LVL_DEBUG: 4, LVL_INFO: 3,LVL_WARNING: 2 ,LVL_ERROR: 1, LVL_NOLOG: 0| 
| use_persistent_memory | Saves/retrieved the settings from persistent storage: yes/no |
| button_enabled | Enable button for triggering oneshot update: yes/no (currently only supported on nRF52) | 
| led_notification_enabled | Enable led notification feature: yes/no |
| motion_sensor | Defines the motion sensor type and bus used.  LIS2DH12 over I2C: lis2dh12_i2c, LIS2DH12 over SPI: lis2dh12_spi |
| default_motion_enabled | Enables/disables motion support: 1/0 |
| default_motion_threshold_mg | Acceleration threshold above which motion will be detected [mg]| 
| default_motion_duration_ms | Duration for acceleration to be above threshold for motion to be detected [ms]|
| default_mbcn_enabled | Enables (1) or disable (0) mini-beacon sending |
| default_mbcn_tx_interval_ms | Mini-beacon transmit rate in miliseconds: only 250ms, 500ms or 1000ms are allowed| 
| default_da_routing_enabled | Enables (1) or disable (0) re-routing of received DA data packets by a LL router|
| default_da_follow_network | Enables (1) or disable (0) the use of automatic neighbour discovery|

A separate build should be generated for anchor and tags with the corresponding parameters set.

# Settings

The settings module is responsible for storing and retrieving from persistent storage the node network and PosLib settings.
The module API is defined in `poslib_settings.h` and consistes of:
```c
bool PosApp_Settings_configureNode(void);
void PosApp_Settings_get(poslib_settings_t * settings);
bool PosApp_Settings_store(poslib_settings_t * settings);
```  

The initial settings are taken from the provided compile time parameters. It is important that those settings are valid
as otherwise PosLib will not start. Once PosLib is started it's settings can be updated over the network through the application 
configuration feature. Please note that the settings module will change the node network role if does not match the node positioning 
mode (e.g. if role is router but mode is tag then the role will be changed to non-router; when this happen a reboot will 
occur, see [[2]](#References)). If persistent storage is not enabled then positioning mode will be forced to match the node role according to the 
defaults set by `POSAPP_TAG_DEFAULT_ROLE` and `POSAPP_ANCHOR_DEFAULT_ROLE`.   

## Manufacturing node customization

During manufacturing it is possible to inject custom settings parameters for each node straight to the persistent storage during flashing. 
For generating the flash hex image of the persistent settings an utility is provided under persistent_config folder. Note that the resulting hex 
file shall either be combined with the application hex. If flashed separately then the persistent setting hex shall be flashed first.

# Motion

The motion module implements all required functionality for detecting the motion state of the node. Currently the ST Microelectronics LIS2DH12 accelerometer 
is supported for nRF52 platform (e.g. `ruuvitag` board). The sensor can be either connected over I2C or SPI. 

All the module files are located under `motion` folder: 
```
├── motion
   ├── acc_interface.h
   ├── lis2
   │   ├── lis2_dev.h
   │   ├── lis2_dev_i2c.c
   │   ├── lis2_dev_spi.c
   │   ├── lis2dh12_wrapper.c
   │   └── lis2dh12_wrapper.h
   ├── makefile_motion.mk
   ├── motion.c
   └── motion.h
```
and the module API is defined in `motion.h` consiting of:
```c
posapp_motion_ret_e PosAppMotion_init();
posapp_motion_ret_e PosAppMotion_startMonitoring(posapp_motion_mon_settings_t * cfg);
posapp_motion_ret_e PosAppMotion_stopMonitoring();
posapp_motion_status_e PosAppMotion_getStatus();
```

To enable motion support the parameter `motion_sensor` shall be set to either `lis2dh12_i2c` or `lis2dh12_spi` (for `ruuvitag` board use `lis2dh12_spi`). 
The module assumes that the `board.h` contains the following defines according to the bus type used:
* **I2C:** 
  * `BOARD_LIS2DH12_INT1_PIN <pin>` or `BOARD_LIS2DH12_INT2_PIN <pin>` to indicate the interrupt pin connected to accelerometer 
  * `BOARD_I2C_LIS2DH12_SA0 0/1` to indicate the address value  
  * `BOARD_I2C_LIS2DH12_SA0_PIN <pin>`  to indicate the SA0 pin
* **SPI:**
  * `BOARD_SPI_LIS2DH12_CS_PIN <pin>`: to indicate the chip select pin 
  
In addition the standard defines for configuring the I2C/SPI HAL are required (see `boards\ruuvitag` for an example).
Note that the accelerometer ST drivers provided on GitHub will be pulled at first compile. 

For minimal power consumption the implementation uses the accelerometer built-in motion detection. The motion configuration consists of
acceleration threshold and duration. If the acceleration is above the threshold for the set duration then the node is considered to be in 
motion (an interrupt will be generated as long the condition persists). When no motion interrupts are generated for `MOTION_STATIC_TIMEOUT_MS` 
seconds (default 60 sec) the node will be considered static.  Every time the motion state changes PosApp will notify PosLib.

## Adding support for other accelerometer sensors

The motion module assumes an interface to the accelerometer as defined in `acc_interface.h` and other accelerometers can be added by simply 
implementing the interface.

# Directed-advertiser and mini-beacons

## Introduction

### Directed-advertiser (DA) based tags

Directed-advertiser is a Wirepas Massive (WM) non-connected communication node role.  A tag configured in DA mode can only communicate with low-latency (LL) routers with DA support enabled. However, it can get RSSI measurements from any low-latency or low-energy (LE) routers or from nodes with mini-beacons enabled (see next section).

### Mini-beacons (MBCN)

Mini-beacons are additional signals transmitted by an anchor to supplement the measurement and allow tag power optimization.

DA positioning will include the following components:

* DA tag: a tag running PosApp/Lib with DA support

* DA WM infrastructure: a WM infrastructure composed of LL routers with DA support

* Dedicated anchors: a LE non-router sending mini-beacons

## Getting started

### Building an router/anchor supporting DA tags

For an anchor the `make` shall contain the following options set:

| Parameter             | Description                                                    |
|-----------------------|----------------------------------------------------------------|
`default_poslib_device_mode=4`  | opportunistic anchor|
`default_role=1` | router role|
`default_role_flag=16`  | low-latency|
`default_mbcn_enabled=1` | mini-beacons enabled|
`default_mbcn_tx_interval_ms=1000` | mini-beacon transmit interval 1000ms|
`default_da_routing_enabled=1` | DA data routing enabled|
`default_da_follow_network=0` | Automatic neigbour discovery disabled|

### Building an DA tag

For an DA tag the `make` shall contain the following options set:

| Parameter             | Description                                                    |
|-----------------------|----------------------------------------------------------------|
|`default_poslib_device_mode=5`  | DA tag|
|`default_role=4` | advertiser role|
|`default_role_flag=0`  | low-energy|
|`default_mbcn_enabled=1` | mini-beacons enabled|
|`default_mbcn_tx_interval_ms=1000` | mini-beacon transmit interval 1000ms|
|`default_da_routing_enabled=1` | DA data routing disabled (no effect for tag)|
|`default_da_follow_network=0` | Automatic neigbour discovery disabled|

### Building a dedicated anchor

A dedicated anchor is a WM node installed in fixed/known location sending mini-beacons.
The node role can be either non-router/router/autorole in LL or LE. The typical setup is
a LE non-router battery powered which will allow an easy and quick installation. 

For a dedicated anchor the `make` shall contain the following options set:

| Parameter             | Description                                                    |
|-----------------------|----------------------------------------------------------------|
|`default_poslib_device_mode=4`  | opportunistic anchor|
|`default_role=2` | non-router role|
|`default_role_flag=0`  | low-energy|
|`default_mbcn_enabled=1` | mini-beacons enabled|
|`default_mbcn_tx_interval_ms=1000` | mini-beacon transmit interval 1000ms|
|`default_da_routing_enabled=0` | DA data routing not enabled|
|`default_da_follow_network=0` | Automatic neighbour discovery disabled|

## Sending data from a DA tag

As a DA node can only communicate with a router sending data to the sink will require two parts:

### DA tag data sending

Sending data can be done through the new introduced API:
```c
 app_lib_data_send_res_e PosLib_sendData(app_lib_data_to_send_t * data, app_lib_data_data_sent_cb_f sent_cb);
```

The function prototype is identical with the Shared Data library `Shared_Data_sendData` (see the library documentation for details).
Note that the DA tag source address will not be visible to the sink as the packet is re-routed by the router application.
As a result, the DA tag address shall be embedded into data payload. 

To send data to the sink set `data->dest_address = APP_ADDR_ANYSINK` in the call. If the node is DA tag the destination address
will be changed to the best router with DA support (if available). In case the node is not in DA role the standard data sending will be used.

Data sending can be done to any app allowed endpoints (EP) but don't use any of PosLib endpoints (238:238, 238:0)!

Data sending shall be synchronized with PosLib positioning update. This can be done easily by registering for the
`POSLIB_FLAG_EVENT_UPDATE_END` event using `PosLib_eventRegister` and sending the data in the provided callback. 

If there is the need to prepare/acquire the data before this can be triggered by registering to `POSLIB_FLAG_EVENT_UPDATE_START` event.

### DA data re-routing  
 
The LL DA enabled router will receive the data packet sent by the tag and will have to re-route it to the sink.
This is not done automatically and if the re-routing is not implemented the packed will be lost! 
The router shall:

* register a unicast data receive handler using the shared data library for the expected EP
 
See `Shared_Data_addDataReceivedCb` and set:
```c
    item->filter.mode=SHARED_DATA_NET_MODE_UNICAST 
    item->filter.src_endpoint=... 
    item->filter.dest_endpoint= ...
```
* In the data received callback send the received data packet to sink.

See `Shared_Data_sendData` 

## Sending data to a DA tag from router/sink

Currently not supported. 

## Customizing mini-beacon payload

Mini-beacons are owned by PosLib but their content can be customised by the application within limits.

In the PosLib settings the `poslib_mbcn_config_t` defines the mini-beacon content.
The content can be augmented by filling one or several records defined by:
```c
typedef struct
{
    uint8_t type;   // according to poslib_mbcn_record_types_e
    uint8_t length;  // shall be <= POSLIB_MBCN_RECORD_MAX_SIZE, invalid record when 0
    uint8_t value[POSLIB_MBCN_RECORD_MAX_SIZE];
} poslib_mbcn_record_t;
```

Each record is a type-lenth-value (TLV) and there types are predefined in `poslib_mbcn_record_types_e`.
Note that `POSLIB_MBCN_TX_INTERVAL` and `POSLIB_MBCN_FEATURES` are reserved for PosLib and shall not be used.

It is important that the mini-beacon payload is kept to minimum posible and therefore only add strictly the 
required records (i.e. not the ones good to have).

## Receiving and decoding mini-beacons by the app

To receive the mini-beacons in the app register a data receive handle using `Shared_Data_addDataReceivedCb`.
```c
    item->filter.mode=SHARED_DATA_NET_MODE_BROADCAST 
    item->filter.src_endpoint=POSLIB_MBCN_SRC_EP
    item->filter.dest_endpoint=POSLIB_MBCN_DEST_EP
```
The received payload can then be decoded using the `PosLib_decodeMbcn` PosLib API utility function.
Mini-beacons are only received when PosLib makes a positioning update and there can be several or zero mini-beacons from 
an anchor.

# Other features

## LED notification

As described in [[2]](#References)) PosLib can send a LED event notification according the settings sent over network through application configuration.
PosApp subscribes to this event and turns a LED on/off. This feature is enabled through `led_notification_enabled` parameter and the LED index is defined by `LED_ID`
(the LED pin is defined according to `BOARD_LED_PIN_LIST` from `board.h`).  

## Button triggered position update

An example for triggering a position measurement update at a press of a button is provided. The feature is enabled through `button_enabled` and the used button index is 
defined by `BUTTON_ID` (the button pin is defined to `BOARD_BUTTON_PIN_LIST` from `board.h`).  Currently this feature is only supported in nRF52 family.

# References

[1] [Wirepas Positioning Application Reference Manual v1.4](https://developer.wirepas.com/support/solutions/articles/77000498897)

[2] [PosLib integration guide](../../../libraries/positioning/docs/PosLib_integration_guide.md)
