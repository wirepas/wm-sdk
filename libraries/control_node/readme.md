# Control library

## Overview
The control library abstract all the complexity related to the use of the Direct Advertiser role in the context of a light switch. It has been developed to minimize the packet latency between the node and the router and the power consumption.
The library is split in two. Building a lighting switch control requires:
* Implementing an application for the light switch using the `control_node` library.
* Integrating the `control_router` library into the application running on the light controller (note: the `control_router` library was designed to run concurrently with the other functionality of the light controller application).

The libraries implement the diagnostics gathering and sending. These are automatically handled by the libraries (i.e. user application intervention is not needed). They are sent automatically the by control node and forwarded to the Sink by the router.

The control_node features are:
* Manages background connectivity in a power optimized manner (router selection, SNDP, scans…).
* Send DA diagnostic packet at configured interval.
* User application can send a DA unicast to the network and upon successful transmission an ACK with data payload will be received from the router (best router is automatically selected and a retry mechanism is implemented).
* Provides, through a callback, the user data content part of the received ACK data.

The control_router features are:
* Forward received DA diagnostics automatically to the Sink.
* Provides an API to generates the ACK packet with Otap parameter.

*__Note :__ Shared Data and App_Scheduler are used by the control libraries. The associated callback of the single-MCU API must not be used in the application.*

## Usage

### Control Node:
#### Initialization
Prior to initialize the library the node must be configured to DA mode.
`Control_Node_init` must be called to initialize the library. Configuration structure allow to set the TTL of the sent packets and the interval at which DA diagnostics are sent. `Control_Node_init` can safely be called again to re-initialize the library (to modify diagnostic interval for example).

Code example to initialize the library:

```
    control_node_ret_e ctrl_ret;
    control_node_conf_t conf = {
        .diag_period_ms = 43200000; //12h diag interval
        .packet_ttl_ms = 250; //250ms packet TTL
        .ack_cb = control_node_ack_cb, //the ACK received callback
    };

    // Basic configuration of the node with a unique node address
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        // Manage error here.
    }

    // Set Directed Advertiser node role
    app_res = lib_settings->setNodeRole(APP_LIB_SETTINGS_ROLE_ADVERTISER);
    if (app_res != APP_RES_OK)
    {
        // Manage error here.
    }

    // Initialize the library
    ctrl_ret = Control_Node_init(&conf);
    if (ctrl_ret != CONTROL_RET_OK)
    {
        // Manage error here.
    }
```

#### Sending data
`Control_Node_send` must be used to send packet. It offers an automatic router selection and a retry mechanism.

Automatic router selection : The router (i.e destination address) to send the packet to is selected automatically. The best router is selected based on its RSSI and the last time it has been eared.
Retry mechanism : The sent packet have a time to leave (TTL) set at library init. It means if the packets is not sent successfully after the configured time, it will be removed from the stack buffers. To enhance the success rate, if after TTL/2 ms the packet is still not sent, it will be also be sent to the second best router in the list.

*__Note :__ The retry mechanism can result in the same packet being received by two routers.*

#### Receiving data
A node configured as Directed Advertiser can't receive packets. The only way to receive data is in the ACK sent by the router in reply of a packet sent by the control node. It means a control node can't receive data if it does not send packet.

If the application has registered the `control_node_ack_cb_f ack_cb` in init structure, it will be called each time an ACK is received.

* __Note :__ The ACK callback is called for each received ACK. They can be generated from packets sent by the application but also by the DA Diagnostics packets automatically sent by the library.*

### Control Router
#### Initialization
Prior to initialize the library the node must be configured as a low latency router.
`Control_Router_init` must be called to initialize the library. Configuration structure allow to set the TTL of the sent packets and the interval at which DA diagnostics are sent.

Code example to initialize the library:
```
    control_node_ret_e ctrl_ret;
    control_router_conf_t conf = {
        .ack_gen_cb = acklistener_cb // Function called to generate the ACK sent to the control node
    };

    // Basic configuration of the node with a unique node address
    if (configureNodeFromBuildParameters() != APP_RES_OK)
    {
        // Manage error here.
    }

    // Set the node as low latency router
    app_res = lib_settings->setNodeRole(APP_LIB_SETTINGS_ROLE_HEADNODE_LL);
    if (app_res != APP_RES_OK)
    {
        // Manage error here.
    }

    // Initialize the library
    ctrl_ret = Control_Router_init(&conf);
    if (ctrl_ret != CONTROL_RET_OK)
    {
        // Manage error here.
    }
```
#### Receiving data
Data from the control node is received through the normal data path.

#### Generating Acknowledge
The control node can't receive data normally. The only way for the control router to send data to a control node is to generate an ACK when receiving a packet.
If the application has registered the `app_llhead_acklistener_f ack_gen_cb;` in the init structure, the callback to generate an ACK will be called for each received packet. 'in' struct param point to the received packet, it can help to generate the appropiate data to send to the control node.

*__Note :__ The execution time is very limited in the callback. It must not exceed a couple of milli-seconds.*

### DA Diagnostics
As the control node is configured as Directed Advertiser, the stack can't send diagnostics. To fix this, the control node library send a simplified diagnostic packet. It contains scratchpad status, battery voltage and success rate, timings of sent diagnostics. The interval at which diagnostics are sent can be configured at control node initialization.
The DA diagnostic packet sent by the control node will be received by the control router library and automatically forwarded to the Sink. When forwarding the packet, the control router library will add the control node address to the packet.
The DA diagnostics packet are sent on endpoints [237:237]. The format received by the Sink is the following:

| Name            | Size    | Description                                              |
| --------------- | ------- | -------------------------------------------------------- |
| address         | 4 Bytes | The address of the control node that sent the diagnostic |
| voltage         | 2 Bytes | Battery voltage of the node in mV |
| proc_otap_seq   | 1 Bytes | The processed scratchpad sequence of the node |
| stored_otap_seq | 1 Byte  | The stored scratchpad sequence of the node |
| success         | 2 Bytes | Number of diagnostic packets sent successfully |
| error           | 2 Bytes | Number of errors when sending diagnostics |
| timing_us       | 4 Bytes | Time in µS to send the last diagnostic |

*All multi byte values are encoded in Little-Endian.*

### Control Node Otap
Thanks to the new otap control mechanism the control node can updated like any other nodes. The control node can receive the scratchpad normally, but it can't receive the target scratchpad and action info through DNPD mechanism (introduced in stack v5.1). To solve this problem, the stack automatically appends this information to the ACK sent by the router.
The stored and processed scratchpad sequence can be checked in the DA diagnostics sent by the control node as it can't respond to remote API commands.

The rest of the otap process is normal.

*__Note :__ If a control node never send any packet (from application, and diagnostics are disabled), it will never receives the target scratchpad and action info in the ACK packet and it won't be possible to otap the node.*

## Example
See control_node and control_router applications for an example how to use these libraries. They are meant to work together.
