# Control Router application

## Application scope
This application play the role of a light fixture in a lighting network. It is meant to be used with the control_node application that plays the role of the light switch. It uses the control router library that manages all the low level aspects of the control router using directed advertiser (controlling DA node diagnostics).

When receiving a switch (On or Off) event from a control node, the app will broadcast it to the other lights in the network. A small delay (compensated by trave time) is added before switching the light to synchronize all nodes in the network.

The control node configuration is received through appconfig data (using the shared appconfig library TLV format).

## Application api
When receiving a switch event from the control node, the router will broadcast it in the network on the same endpoints [21:21]. The node adds the control node address to the packet before forwarding it. Other than that the content is the same. It is described in `control_app_switch_fwd_t (control_node/common.h)` structure.

The app config data is used to share the configuration sent to the control router to the nodes. The `shared_appconfig` library is used to decode the received appconfig data. To set the appconfig data, the backend must comply with the raw format described below:
`[Version 2B][Nb entry 1B][Type 1B][Length 1B][Diag Period ms 4B][DA packet TTL 4B]`
Example : F6 7E 01 C0 0D 00 00 2E 93 02 FA 00 00 00
  - Shared AppConfig Version: 0x7EF6
  - Shared AppConfig Number of entry: 1
  - Shared AppConfig Type: 0xC0
  - Shared AppConfig Length of data: 13 bytes
  - 12h DA diag period (0x02932E00)
  - 250ms DA max packet ttl (0x000000FA)

*All multi byte values are encoded in Little Endian*

Once decoded the appconfig data is stored in `control_app_appconfig_t` structure.

For each received packets from a control node, the router will generate an ACK. It's content is a copy of the received appconfig and is described in `control_app_ack_t`structure.

## Application architecture
The application makes a straight forward usage of the router part of the control library. The library provides a callback to generate ACK control nodes, and forward automatically DA diagnostics from control nodes to the Sink (See control library for details).

A packet callback is registered to receive light switch event. These events are then broadcasted only to LL nodes in the network (CSMA-ONLY flag is set).
A defered task is used to turn On/Off the Led and compensate for the travel time of packet.

The shared_appconfig library is used to receive the control node configuration.

## Testing
To test this application a minimum of two nodes are needed. One running the `control_node` application (the light switch) and another running the `control_router` application (the light fixture).

A Sink connected to a gateway and a backend is optionnal. It is needed to modify control node parameters, test the otap functionnality and receive diagnostics from the control node.

Serial logs are enabled by default and help to understand what the application is doing. The logs are sent over the serial port of the board at 115200 bauds. They can be disabled(enabled) by (un)commenting # APP_PRINTING=yes in the makefile. Log level can be controlled by modifying DEBUG_LOG_MAX_LEVEL at the beginnning of app.c and in each library .c file.

## Customization
The led driver to control the light is very simple and can be replaced by any driver to control the hardware on the board (DALI driver, dimming, ...).

When receiving a switch event the control router only broadcast it to the whole network. It's behavior can be modified to only send the event to a group of lights using multicast for example.

The control node sends a boolean in the switch event packet. This can be modified to accomodate more complex behaviors (Dimming, RGB data, ...).
The same way the content of appconfig data and the generated ACK can be modified to pass some configuration or change the behavior of the control node.

The packet exchanged between the control node and the control router are described in the `source/example_apps/control_node/common.h` file. Each modification must done in the control node and in the control router application at the same time.
