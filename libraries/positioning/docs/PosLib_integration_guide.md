# Table of content

- [Introduction](#introduction)
- [PosLib features](#poslib-features)
- [API](#api)
- [PosLib application configuration](#poslib-application-configuration)
- [PosLib measurement message](#poslib-measurement-message)
- [Important notes](#important-notes)
- [Simple integration example](#simple-integration-example)
- [References](#references)

# Introduction

The purpose of Positioning Library (PosLib) is to provide an easy way to use service for collecting the measurements required 
for Wirepas Positioning System (WPS). PosLib can be instantiated in any custom application. A complete integration example is provided 
in Positioning Application (PosApp).

This guide is targeted for developers planning to integrate PosLib in their custom application.
As a prerequisite, it is assumed that the reader is familiar with developing an application using the Wirepas SDK. 

PosLib was developed such that the Wirepas Massive (WM) stack API can be shared between PosLib and the application (i.e. concurrent  access to the API is possible).
For this purpose the following shared libraries are used:
- **Scheduler**: allows to schedule multiple application tasks
- **Shared data**: allows multiple modules to access data sending and receiving API    
- **Shared neigbours**: shares the network/cluster beacon reception
- **Shared beacons**: allows concurrent  control of BLE beacon API 
- **Shared offline**: encapsulates the no-router long sleep (NRLS) API
- **Shared appconfig**: provides a scheme for sharing the network wide 80 bytes application configuration by several applications/modules running in the same network
- **Voltage sampling**: samples the battery voltage and sends it through the data message

:exclamation: **NOTE**: The application shall use the shared libraries listed above when trying to access any WM APIs wrapped by those. Failing to do so will result 
in malfunction of the PosLib. Therefore is required for the developer to get familiar with these shared libraries.

:exclamation: **NOTE**: PosLib is provided as is and changes to PosLib code or behaviour are not recommended and are not supported by Wirepas.

# PosLib features

PosLib provides the following features:
* **measurement collection**: the received signal strength indicator (RSSI) of the anchors surrounding the node are collected and sent as a data packet over WM (either periodically and/or on request).    
* **application configuration**: dynamic reconfiguration of the node through WM application configuration.
* **motion**: adjustment of the measurement update rate based on node motion status (the main purpose is power saving and reducing network load).
* **anchor/tag support modes**: there are dedicated operating modes for anchors and tags.
* **node classes**: it is possible to create groups of nodes (classes) which share the same base configuration parameters. 
* **BLE beacons**: conditional sending of configurable BLE Eddystone UID and/or iBeacons.
* **battery voltage sampling**: each measurement message will contain the battery voltage record (if enabled).
* **LED notification event**: it is possible to trigger an event through application configuration     
* **Directed-advertiser**: a Wirepas Massive (WM) non-connected communication mode for tags to reduce power consumption and to enable higher tag density
* **Mini-beacon**: additional beacons transmitted from anchors to support the measurement and to optimize tag power consumption
   
# API

PosLib API is defined in `libraries\positioning\poslib.h` and consists of the following:

## Configuration

PosLib configuration API allows to set or get its configuration:

```c 
poslib_ret_e PosLib_setConfig(poslib_settings_t * settings);
poslib_ret_e PosLib_getConfig(poslib_settings_t * settings);
```

Note that the settings are only accepted if the parameter values are valid (an error will be returned otherwise).
The configuration API functions can be called at any time.

PosLib settings defined in `poslib_settings_t` consist of:
* **node mode**: the positioning operating mode of the node (see [[1]](#References) for details)
* **node class**: there can be up to 7 classes used `0xFF...0XF9`; (note that class `0xF8` is used only for node specific settings)
* **measurement update rates:**
  * **update_period_static_s**:  used when node is static or motion monitoring is not supported
  * **update_period_dynamic_s**: used when node is dynamic (not used if set to 0 or motion disabled)
  * **update_period_offline_s**: used when node is outside WM coverage (not used if set to 0)
* **BLE settings:**
  * **type:** BLE beacon types to be enabled (Eddystone, iBeacon or both)
  * **mode:** BLE operating mode (off, always on, on when outside WM coverage)
  * **Edystone:** sets the Eddystone beacon channel(s), transmit power and update rate (`poslib_ble_mode_config_t`)
  * **iBeacon:** sets the iBeacon beacon channel(s), transmit power and update rate (`poslib_ble_mode_config_t`)
  * **activation_delay_s:** time after which BLE beacons are automatically sent if the node is outside WM coverage
* **motion:** 
  * **enabled:**  indicates that application supports motion monitoring
  * **threshold_mg:** threshold for motion detection (PosLib will change the value if provided through application configuration)
  * **duration_ms:** duration of acceleration above the set threshold (PosLib will change the value if provided through application configuration)
* **da:**
  * **routing_enabled:** re-routing of received DA data packets by a LL router
  * **follow_network:** automatic neighbour discovery
* **mbcn:**
  * **enabled:** indicates that application supports sending mini-beacons
  * **tx_interval_ms:** provides the update rate for mini-beacon broadcasts (miliseconds), currently supports 250ms, 500ms or 1000ms
  * **records:** defines records to be included in mini-beacon payload
* **led notification:** provides an event notification (e.g. to control a LED for pick-to-light application) controlled through PosLib application configuration
   
:exclamation: **NOTES**: 
* all measurement update periods have a min/max value set by `POSLIB_MIN_MEAS_RATE_S/POSLIB_MAX_MEAS_RATE_S` (currently 8/86400 seconds).
* although The Eddystone/iBeacon update rate can be set individually the effective used rate is the smallest of the two (limitation of the WM API).

The configuration set by the application can be changed at run time through WM application configuration feature. Every time PosLib configuration changes 
the event `POSLIB_FLAG_EVENT_CONFIG_CHANGE` is generated (see more about events in section [Events](#events)). Is is recommended that the application subscribes to this event and updates its own configuration.
It is also recommended that the application will store the configuration into persistent memory such that it will remain the same upon reboot. 

## Measurement update control

As stated previously the may purpose of the PosLib is to collect the required measurements for positioning.  
The measurement collection can be triggered in two ways: 
* **periodically:** with a predefined or dynamically adjusted update period
* **on-demand:** initiated at application asynchronous request (oneshot)

The two update modes can be also combined (i.e. it is possible to trigger an oneshot update while in periodic mode).   

### Periodic update

A periodic update is started/stopped by calling:

```c 
poslib_ret_e PosLib_startPeriodic(void);
poslib_ret_e PosLib_stopPeriodic(void);
```

Note that PosLib shall be configured before calling startPeriodic otherwise an error will be returned and the periodic update will not start.
Once periodic update is started it will run indefinitely or until stopPeriodic is called. 

Note that application configuration decoding is only activated when the periodic updates starts and de-activated when the periodic update is stoped.
If configuration is changed while the periodic update is executed the new settings will get into effect at the next update.
The periodic mode if the typical operating mode of the PosLib.

### Oneshot update

The application can trigger a single asynchronous update by calling:

```c 
poslib_ret_e PosLib_startOneshot(void);
```

Note that PosLib must be configured before calling startOneshot otherwise an error will be returned
and the update will not start. The update will be triggered imediatelly unless an update is already in progress at the call moment.

On-demand (oneshoot) update can be used regardless if the periodic update is started or not. However, the use of just oneshoot
update (i.e. periodic update is not started) is recommended only in special applications (where positioning updates are very infrequent and 
triggered by application asynchronous events). Note that the application configuration decoding is only available when periodic update is started (if 
required then periodic mode shall be started with a high update period).
  
## Events

While running, PosLib will generate the set of events defined in `poslib_events_e` according to its instantanous state.  
The application can register/deregister to those events using:

```c 
poslib_ret_e PosLib_eventRegister(poslib_events_e event, poslib_events_listen_info_f cb, uint8_t * id);
void PosLib_eventDeregister(uint8_t id);
```

It is possible to register to multiple events in one registration call (e.g. the registration to start/end update events can be done by registering to:
`POSLIB_FLAG_EVENT_UPDATE_START | POSLIB_FLAG_EVENT_UPDATE_END`). 

Note that maximum 8 subscribers are supported.

### LED notification event
The events `POSLIB_FLAG_EVENT_LED_ON` and `POSLIB_FLAG_EVENT_LED_OFF` are generated as a result of a notification event triggered through application
configuration. The application shall register to these events and turn the LED on/off accordly.

## Status

The instantaneous status of the PosLib can be retrieved using:

```c 
poslib_status_e PosLib_status(void);
````
which will return one of the following state:
* **STOPPED:**  positioning updates are not started
* **IDLE:** positioning update is scheduled
* **UPDATE_START:** a positioning update is ongoing

## Motion

The application can notify PosLib of the node motion state (static/dynamic) by calling:
```c 
poslib_ret_e PosLib_motion(poslib_motion_mode_e mode);
```

If the periodic update is started and motion support is enabled then PosLib will adjust the update rate according to the injected static 
or dynamic node state. Otherwise the call does not have an effect.

## Required defines

The application should define the following at compilation time:
* to enable battery voltage sampling: `CONF_VOLTAGE_REPORT` 
* to enable motion application configuration decoding support: `MOTION_SUPPORTED` 
* to enable forcing of positioning mode through application configuration: `CONF_USE_PERSISTENT_MEMORY` (note that application shall 
store the positioning setting persistently and handle possible role change).   

# PosLib application configuration

PosLib will register to the shared application configuration library and receive all configuration payloads for type 0xC1.

:exclamation: **NOTE**: Shared application configuration type 0xC1 is reserved for PosLib and shall not be used by any other application
in the same network.
 
The format of the PosLib application configuration is described in [[1]](#References).

# PosLib measurement message

PosLib will send the measurement message using source endpoint 238 and destination endpoint 238.

:exclamation: **NOTE**: Enpoints 238/238 are reserved for PosLib and no other application operating in the same network shall use them. 

The measurement message contains the RSSI measurement, the voltage measurement and the node info records as it's format is described in [[1]](#References). 

# Important notes

## NRLS
Non-router long sleep (NRLS) is a special mode of the WM stack which allows to place it into offline state (i.e. disconnected from the network) during the intervals 
when the node does not need to send or receive data (more details on NRLS can be found in [[2]](#References)). PosLib uses the NRLS for tags in order to save power and 
increase the number of nodes able to connect to WM concurently. Please note that while WM stack is in offline state the application can run normally. PosLib interfaces to 
the NRLS API throgh the `shared_offline` library which allows multiple application modules to control stack online/offline state. When the positioning update  is completed 
PosLib will request teh stack to enter to offline immediately. In the case that application still requires that the stack stays online (e.g. the application needs to send/receive 
additional data) then the application shall register to the `shared_offline` library using `Shared_Offline_register` API (the registration shall be done when the 
`POSLIB_FLAG_EVENT_ONLINE` is generated). While the application is registered, the offline request made by PosLib will not be executed. Once the application completes it's 
processing it shall call `Shared_Offline_unregister`. This will allow the offline request made by PosLib to proceed.  Through this flow the application network activity is in 
sync with PosLib and will result in minimal power consumption.

## Operating mode and network role
    
As described in [[1]](#References) there are special operating modes for anchors and tags. While operating as anchor the network role of the node shall be router 
while when operating as tag the network role of the node shall be non-router. PosLib does not accept a configuration (either from the application or through application 
configuration which does not fullfil the previous stated rule). The only exception is when the operating mode change request is done through application configuration 
individually for the node and persistent storage setting is enabled. In such a situation the application will notice in the configuration update event that the opperating mode 
and network role do not match and at that moment it shall change the network role to correct it (i.e. this is a request to impose the operating mode). An example is available in 
the PosApp.    

# Simple integration example

To instantiate PosLib in an application the following three steps must be followed:

1. initialize the libraries used by PosLib
```c
    Mcu_voltageInit();
```
2. configure PosLib
```c
     PosLib_setConfig(&settings);
```
3. register to event(s)
```c 
    PosLib_eventRegister(event, cb, &id);
```
4. start the position update (shall be called after stack start)
```c  
    PosLib_startPeriodic();  
```

:exclamation: **NOTE**: The number of task available for the application scheduler is fixed and defined at compile time. The application shall add the number of tasks it needs.

# References

[1] [Wirepas Positioning Application Reference Manual v1.4](https://developer.wirepas.com/support/solutions/articles/77000498897)

[2] [Non-Router Long Sleep (NRLS)](https://developer.wirepas.com/a/solutions/articles/77000406955?portalId=77000019115) 
