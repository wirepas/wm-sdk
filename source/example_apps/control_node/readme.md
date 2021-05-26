# Control Node application

## Application scope
This application plays the role of a switch in a lighting network. It is meant to be used with the control_router application that plays the role of the light fixture. It uses the control node library that manages all the low level aspects of control node using directed advertiser (connectivity to CSMA-CA headnode, Applicative diagnostics).

When pressing on button 1, the app will send a switch pressed event to the strongest RSSI router. When receiving the event, the router will broadcast it amoung the other lights of the network and turn on/off the lights.

The application gets its configuration through the ACK received when a packet is sent.

## Application api
The switch event is sent on endpoints [21:21]. Its content is described in `control_app_switch_t (common.h)` structure.

The application configuration is received in the ACK packet. Diagnostics interval and sent packet TTL can be configured. It is descibed in `control_app_ack_t (common.h)` structure.

## Application architecture
The application architecture is very simple. It uses the control library to configure the node in Directed Advertiser and send packets.

The Button driver is used to generate an interrupt when the button is pressed and send the switch event.

## Testing
To test this application a minimum of two nodes are needed. One running the `control_node` application (the light switch) and another running the `control_router` application (the light fixture).

A Sink connected to a gateway and a backend is optionnal. It is needed to modify control node parameters, test the otap functionnality and receive diagnostics from the control node.

Serial logs can be enabled to help understand what the application is doing. The logs are sent over the serial port of the board at 115200 bauds. To enable them, uncomment # APP_PRINTING=yes in the makefile. Log level can be controlled by modifying DEBUG_LOG_MAX_LEVEL at the beginnning of app.c and in each libraries c files.

### Otaping the control node
As explained in the control library readme (`libraries/control_node/readme.md`), a node configured as Directed Advertiser follow the normal otap process.
Only limitation is that the control node will receives the target scratchpad and action info only when it sends a packet (and receives an ACK), and not in a synchronous manner like the rest of the network.

## Customization
The button driver to control the switch is very simple and can be replaced by any driver to control the hardware on the board (capacitive touch, slider, ...).

The application sends a boolean in the switch event packet. This can be modified to accomodate more complex behaviors (dimming, RGB data, ...).
The same way the content of the ACK received by the control node can be modified to pass some configuration or change the behavior of the node.

The packet exchanged between the control node and the control router are described in the `source/example_apps/control_node/common.h` file. Each modification must done in the control node and in the control router application at the same time.