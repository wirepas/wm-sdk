# Local provisioning

## Overview

This local provisioning library is a wrapper on top of the provisioning more generic library.
This wrapper allows to easily add a local provisioning mechanism in any application.

It's usage is demonstrated in the local_provisioning application.

A node enabling and configuring this library will start unprovisioned at first boot.
The application can initiate a provisioning session depending on its own logic. It
can be after a button being pressed, periodically,...

At least one node with the same library in proxy mode must be already provisioned and connected to a Wirepas network in the same radio range for the joining node to receive its Wirepas network credentials.

## How to enable this library in an application

In order to enable if from your app, please add the following line in your application makefile or config.mk

```c
LOCAL_PROVISIONING=yes
```
It will add all the required files and associated library into your application.

The following SDK libraries are used by the Local Provisioning libraries and must be used by the rest of the application rather than direct single mcu associated api.

* provisioning
* scheduler
* shared_data
* shared_appconfig
* stack_state

## How to start a provisioning session for a node

### Initialize the library
The application must initially initialize the library.
It is not done automatically (compare to some other libraries) because it requires parameters.

The first parameter is the pre shared key to use between the proxy and the joining node. It must be the same in both nodes.
It can also be NULL. In this case, a default PSK key embedded in the lib is used. It is not secure as this key is public.

The second parameter is a callback to be informed when the node enters in proxy mode (start sending joining beacon).

```c

static const uint8_t psk[32] = { 0xAB, ....., 0x13};

static void on_prov_proxy_enabled_cb(bool enabled)
{
    # App is informed through this calback that proxy mode is enabled
    # It can blink a led for example
    ...
}

...

Local_provisioning_init(psk,
                        on_prov_proxy_enabled_cb);
...

```

### Start a provisioning session

When the application wants to start a session, it has to explicitly call:

```c
Local_provisioning_start_joining(NULL);
```
The optional parameter allows for the app to implement its own logic to choose the proxy node to initiate provisioning session. If not specified, the highest RSSI will be used. It is important to implement it if two distincts networks with local joining enabled are in the same area.

The node will reboot at the end of the session regardless of the result.
* If the joining was successful, the node will reboot to apply the received parameters.
* If the joining is unsuccessful, the stack is stopped and the node rebooted to save energy.

In order for an app to know its status regarding provisioning, app can use following call:

```c
Local_provisioning_is_provisioned();
```
A node is considered provisioned when it has a valid Wirepas network address and channel, a node address (not exchanged through local provisioning) and network security keys.

## How to control provisioning in a Wirepas network

When a node is provisioned and the stack is started from the application, if the local provisioning library is initialized, it will automatically wait for network information to start acting as a provisioning proxy node.
This information is sent and controlled throught app config mechanism implemented and described in shared_appconfig library.

The reserved type for this feature is 0xc4.

### Enable local provisioning

According to shared app config format, enabling local provisioning can
be done with following app_config injected in network:

f6 7e 01 c4 01 01

A PSK can be specified when enabling provisionoing (to be described).

### Disable local provisioning

Disabling it from all nodes can be done with following app_config:

f6 7e 01 c4 01 00

## Special case of sink nodes

The local provisioning library is enabled in dualmcu app running in sink nodes in most of the gateway.

Typical use case is to configure a sink with network configuration (including network keys) and set the corresponding app config to enable the provisioning procedure.

## Application demo

The local_provisioning application demonstrates its usage with following behavior.

1. Node starts un-provisioned
    * Led 0 is on
2. When button 0 is pressed, node try to join a network
    * Led 0 starts blinking
3. Node rebootq
    * Led 0 is on again if node was not able to join a network. Back to 1.
    * Led 1 is on if node is provisioned
4. Led 1 starts blinking if joining is enabled (through app_config)
5. Node starts sending message every 10 seconds
6. If button 1 is pressed, node is un-provisioned => Back to 1.
