# Local provisioning

## Overview

This local provisioning library is a wrapper on top of the provisioning more generic library.
It allows to add a local provisioning mechanism in any application.

It's usage is demonstrated in the local_provisioning application and dualmcu_app.

### Definition

* **Provisioned node**: a node is considered provisioned when it has a valid node role, node address, network address, network channel and network keys
* **Unprovisioned node**: a node that is missing one of the setting of a provisioned node
* **Proxy node**: a provisioned node able to send joining beacon and send its configuration to an unprovisioned node
* **Joining node**: an unprovisioned node, trying to join a secured network (with network keys set)


A node enabling and configuring this library will start unprovisioned at first boot.
The application can initiate a provisioning session depending on its own logic. It can be after a button being pressed, periodically,...

At least one node with the same library in proxy mode must be already provisioned and connected to a Wirepas network in the same radio range for the joining node to receive its Wirepas network credentials.

Both must share a common secret (Pre Shared Key), in order for network credentials to be securely sent on the air between proxy and joining node.

Once a joining node is provisioned, it can become a proxy node. It allows the construction of a network in a secure way from a single provisioned node (typically a sink attached to a gateway and configured from a backend).

When local provisioning is enabled in a Wirepas network, all provisioned nodes will start broadcasting joining beacons containing a specific 4 bytes id.
This application specific id associated with a PSK, will be used by joining nodes to only try joining a network with a matching PSK (same id).

## How to control local provisioning in a network?

A started provisioned node will automatically wait information from the network to start acting as a provisioning proxy node.
This information is sent and controlled through app_config mechanism implemented and described in [shared_appconfig library](../shared_appconfig/shared_appconfig.md).

The reserved type for this feature is **0xc4**.

A script example is available in [backend_script](backend_script) folder.

### App config format for the type 0xc4

The format is a binary format to save room in app_config limited to 80 bytes for all the network.

It can either be of length 1 or 37 depending if a PSK is specified.

#### Without custom PSK
To disable Local Provisioning
> 00

To enable Local Provisioning with the PSK specified from node application init function
> 01

#### With PSK specified

To enable Local provisioning with a custom PSK:
> 02 | <4 bytes for id> | <32 bytes for psk> 

It can be useful in case a new batch of nodes with a new PSK must be added to an existing network. The proxy node will receive from the backend through app_config the PSK and the ID to be used.


## How to enable this library in your application?

To enable local provisioning library, please add the following line in your application makefile or config.mk

```c
LOCAL_PROVISIONING=yes
```
It will add all the required files and associated libraries into your application.

The following SDK libraries are used by the Local Provisioning library and must be used by the rest of the application rather than direct single mcu associated api.

* [provisioning](../provisioning)
* [scheduler](../scheduler)
* [shared_data](../shared_data)
* [shared_appconfig](../shared_appconfig)
* [stack_state](../stack_state)

## How to start a provisioning session from a node

### Initialize the library
The application must initialize the library.
It is not done automatically (compared to some other libraries) because it requires parameters.

The first parameter is the pre shared key and its id to use between the proxy and the joining node. It must be the same in both nodes.
It can also be NULL. In this case, a default PSK key embedded in the lib is used. It is not secure as this key is public.

The second parameter is a callback to be informed when the node enters in proxy mode (start sending joining beacon).
It can be useful to start/stop blinking a led for instance.

```c
static const local_provisioning_psk_t m_psk = {
    // 4 bytes id
    .id = 0x12345678,
    // 32 bytes psk
    .psk = { 0xAB, ....., 0x13}
};

static void on_prov_proxy_enabled_cb(bool enabled)
{
    // App is informed through this callback that proxy mode is enabled
    // It can blink a led for example
    ...
}

...

void App_init(const app_global_functions_t * functions)
{
    ...

    Local_provisioning_init(&m_psk,
                            on_prov_proxy_enabled_cb);
    ...
}


```

### Start a provisioning session

When the application wants to start a session, it has to explicitly call:

```c
static bool on_joining_end_cb(bool success)
{
    // Any code to handle success or failure of joining attempt
    ...
    // False to not reboot, True to reboot
    // If reboot is not done, node will consume more power
    return false;
}

Local_provisioning_start_joining(NULL, on_joining_end_cb);
```
The first parameter allows for the application to implement its own logic to choose the proxy node to initiate provisioning session. If not specified, the highest RSSI beacon matching the id set from init call will be used.

The second parameter is a callback that will be called at the end of the joining attempt.
If not set the node will reboot at the end of the session regardless of the result.
* If the joining was successful, the node will reboot to apply the received parameters.
* If the joining is unsuccessful, the stack is stopped and the node rebooted to save energy (as stack is started to attempt a joining session)
But if a node doesn't want to reboot at the end of a failure, it can return false. 

### Check provisioned status

For an application to know its status regarding provisioning, application can use following call:

```c
Local_provisioning_is_provisioned();
```

### Unprovision a node

An application can unprovision itself by calling the following function:

```c
Local_provisioning_reset_node();
```


## Special case of sink nodes

The local provisioning library is enabled also in [dualmcu_app](../..source/reference_apps/dualmcu_app) running in sink nodes in gateways.

It allows to configure a sink with network configuration (including network keys) and set the corresponding app_config to enable the provisioning procedure. All can be done from a backend.

