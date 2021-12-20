# Low latency application

## Application scope

This application is an example to demonstrate the capabilities of low-latency (LL) mode. 

The demonstrated features are :
- Low-Latency
- Node to Node active flooding (broadcast) --> Adaptive Flooding
- Downlink active flooding (unicast, multicast, broadcast)

This application is a lighting control application, which will pilots leds using two buttons events. 
Depending on the board used, the button position might be different. 
The "upper" button is used to switch all leds on, and the "Lower" button is used to switch all leds off. 
All nodes are by default configured in Low-latency mode. 

- Node to Node active flooding:
The board where the button is pressed will send a message to its neighbours, thanks to Adaptive Flooding process, and hops by hops the message will be broadcast to the whole network.

- Downlink active flooding : 
Depending on which mode is selected: <br> 
    * Unicast: message of light ON/OFF will be send to a specific node.
    * Broadcast: message will be send to all nodes.
    * Multicast: message will be send to a group of nodes sharing the same group address.


The python script found in the **backend_scripts** folder allows a user to interact
with the nodes running the app using the **wirepas-gateway-API** if the nodes are part of a
Wirepas Massive Network connected to a Wirepas gateway. In this configuration, a remote communication
channel can be established via MQTT protocol as commands (described above) sent by the user are published
on dedicated MQTT topics from which gateway can read and parse to send them to the Wirepas Massive network. By
subscribing to dedicated topics the user can also get messages (from the set listed above) coming from nodes
running the Low-latency application. See Readme.md file provided with the script to get more information about its usage and how communication via wirepas-gateway-API is handled.

## Application API

From a communication point, to send and receive data this application:
- supports unicast, broadcast and multicast addressing mode
    meaning it will process messages from sink to all nodes, to one specific node or to a group of nodes, and messages nodes to nodes. 

- receives and generates internal Wirepas Massive network packet format with a specific
    payload format (\<Message ID\>\<Message specific data format\>) for each
    messages. See the "Messages description" section to get all message' data format.

### Messages description
This section covers the description of the low_latency application supported
messages (sent or received) which are :
- a switch message, when a button has been pressed, sent as active flooding, unack broadcast.
- a downlink light on/off message, send as active flooding, unicast, broadcast or multicast
- a grouping message, when a board is wanted to join another group. Send as an acknowledged unicast to a device. 

All these message's data formats are explained in the following sections.
The general format is \<Message ID\>\<Message specific data format\> and all
fields are expressed in little-endian format.

#### Switch message
This message format is :
\<Message ID\>\<counter value\>\<switch command pattern \>.

The table given below presents the field byte size and meaning:

| FIELD         | SIZE |                         DESCRIPTION                                 |
|     :---:     |:---: |                            :---:                                    |
| Message ID    | 1    | Message identifier. Hex value is 0x01                               |
| button ID     | 1    | Button id in [0;1]                                                  | 
| Action        | 1    | Action Type : Press : 1, Release : 0                                |
| Counter value | 2    | Running counter value (starting from 0 with an increment step of 1) |

The command is sent as Active Flooding (unack broadcast) once during press.

When another board receives this switch message, it turns on the LED’s if Button 0 is pressed, and off if buttons 1 is pressed. 


#### Downlink Light ON/OFF message
This message format is :
\<Message ID\>\<Brightness\>.

The table given below presents the field bytes size and meaning:

|       FIELD       | SIZE |                            DESCRIPTION                                 |
|       :---:       |:---: |                              :---:                                     |
| Message ID        | 1    | Message identifier. Hex value is 0x02                                  |
| Brightness        | 1    | 0: light OFF, 1 light ON                                               |

The command is sent as Active flooding. It can be broadcast, multicast or unicast.
Note : Dimming is not implemented but can be easily added.

#### Grouping message
This message format is :
\<Message ID\>\<New multicast address\>.
The table given below presents the field byte size and meaning:

|     FIELD                | SIZE |                  DESCRIPTION                      |
|     :---:                |:---: |                     :---:                         |
| Message ID               | 1    | Message identifier. Hex value 0x02                |
| New multicast address    | 4    | New address on 4 bytes, little endian             |


This message is send downlink to add a node to a group, by changing its multicast address.

## Application architecture

The low-latency application implements a user application which runs in a cooperative
manner with the Wirepas Massive stack which is in charge of handling all Wirepas network
related tasks. The [diagram](https://wirepas.github.io/wm-sdk/) displayed on the
linked page illustrates how the user application interacts with the Wirepas Massive Stack.

This application depends on:
* the Single-MCU api to send and receive messages
* the SDK HAL to control the LEDs and monitor button's event
* the SDK Libraries to
    * process and filter incoming packets
    * schedule the different application's processing

## Testing

To test the application, a minimal network must be set as described in the
Wirepas provided "How to" documents and via the Wirepas terminal the message
data given in the same document must be send to the node running the Low-latency application code.


By default, the application operates in **Low-Latency** mode meaning
the node is constantly listening to the radio channel thus minimizing packet transmission latency
at the expense of increased power consumption.

