# Evaluation application

## Application scope

This application is an evaluation application (i.e some sort of hello world)
allowing to discover Wirepas mesh network capabilities. It embeds some use cases
to generate and process messages from a node it is running on.

This application provides to a node the following capabilities:
* Send messages to a sink
    - a periodic message is sent at a configurable interval
    - a message is sent when a node's button is pressed

* Receive messages from a sink
    - to control a node's LEDs state (i.e switch ON or OFF)
    - to change the period of the periodic message

* Execute some requests coming from a sink
    - to return a node's LED state (i.e is the LED ON or OFF ?)
    - to return the message travel time from sink to node

The python script found in the **backend_scripts** folder allows a user to interact
with the nodes running the app using the **wirepas-gateway-API** if the nodes are part of a
Wirepas Mesh Network connected to a Wirepas gateway. In this configuration, a remote communication
channel can be established via MQTT protocol as commands (described above) sent by the user are published
on dedicated MQTT topics from which gateway can read and parse to send them to the Wirepas Mesh network. By
subscribing to dedicated topics the user can also get messages (from the set listed above) coming from nodes
running the Evaluation application. See Readme.md file provided with the script to get more information about
its usage and how communication via wirepas-gateway-API is handled.

## Application API

From a communication point, to send and receive data this application:
* supports both unicast & broadcast addressing mode
    meaning it will only process messages intended for the node it is running on
    or intended to all the nodes within the network

* integrates the endpoint functionnality of Wirepas which corresponds to
    an application channel (E.g. if node has multiple sensors it could use
    different endpoint to send/receive each sensor’s data).
    In this software, all messages have a dedicated source and destination
    endpoint fixed to the value 1

* receives and generates internal Wirepas mesh network packet format with a specific
    payload format (\<Message ID\>\<Message specific data format\>) for each
    messages. See the "Message description" section to get all message' data format.

### Messages description
This section covers the description of the evaluation application supported
messages (sent or received) which are :
* a periodic message with a configurable interval
* a set periodic message period
* a button was pressed
* a LED set state
* a LED get state request
* a LED get state response
* a message travel time from sink to node calculation request (echo request)
* a travel time response (echo response)

All these message's data formats are explained in the following sections.
The general format is \<Message ID\>\<Message specific data format\> and all
fields are expressed in little-endian format.

#### Periodic message
This message format is :
\<Message ID\>\<counter value\>\<easy to spot data pattern\>.

The table given below presents the field byte size and meaning:

| FIELD         | SIZE |                         DESCRIPTION                                 |
|     :---:     |:---: |                            :---:                                    |
| Message ID    | 1    | Message identifier. Decimal Value is 0                              |
| Counter value | 4    | Running counter value (starting from 0 with an increment step of 1) |
| Data pattern  | 8    | Fixed binary pattern: “0x0A 0x0B 0x0B 0x0A 0x0A 0x0C 0x0D 0x0C”     |

#### Set periodic message period
This message format is :
\<Message ID\>\<new period value\>.

The table given below presents the field bytes size and meaning:

|       FIELD       | SIZE |                            DESCRIPTION                                 |
|       :---:       |:---: |                              :---:                                     |
| Message ID        | 1    | Message identifier. Decimal Value is 128                               |
| New period value  | 4    | Interval value expressed in milliseconds (2000-1200000, default 10000) |

#### Button pressed notification message
This message format is :
\<Message ID\>\<Button ID\>\<Button state\>.

The table given below presents the field byte size and meaning:

|     FIELD     | SIZE |                                             DESCRIPTION                                                  |
|     :---:     |:---: |                                                :---:                                                     |
| Message ID    | 1    | Message identifier. Decimal Value is 1                                                                   |
| Button ID     | 1    | Pressed button ID (Decimal value 0 is the first user available button on the node)                       |
| Button state  | 1    | Button state expressed in binary format (0x02 => Button pressed. It should always be the returned value) |

#### LED state set message
This message format is :
\<Message ID\>\<LED ID\>\<LED state\>.

The table given below presents the field byte size and meaning:

|     FIELD     | SIZE |                                 DESCRIPTION                                  |
|     :---:     |:---: |                                    :---:                                     |
| Message ID    | 1    | Message identifier. Decimal Value is 129                                     |
| LED ID        | 1    | LED identifier (Decimal value 0 is the first user available LED on the node) |
| LED state     | 1    | LED state (Decimal value 0 => switch off, 1 => switch on)

#### LED state get request message
This message format is :
\<Message ID\>\<LED ID\>.

The table given below presents the field byte size and meaning:

| FIELD         | SIZE |                                             DESCRIPTION                                              |
|     :---:     |:---: |                                                :---:                                                 |
| Message ID    | 1    | Message identifier. Decimal Value is 130                                                             |
| LED ID        | 1    | LED identifier from which to get state (Decimal value 0 is the first user available LED on the node) |

#### LED state get response message

This message format is :
\<Message ID\>\<LED ID\>\<LED state\>.

The table given below presents the field byte size and meaning:

|     FIELD     | SIZE |                                 DESCRIPTION                                  |
|     :---:     |:---: |                                    :---:                                     |
| Message ID    | 1    | Message identifier. Decimal Value is 3                                       |
| LED ID        | 1    | LED identifier (Decimal value 0 is the first user available LED on the node) |
| LED state     | 1    | LED state (Decimal value 0 => switched off, 1 => switched on)                |

#### Echo messages (request \& response)

##### Request
This message only consists of a Message ID which has a decimal value of 131.

##### Response
This message format is :
\<Message ID\>\<Travel time\>.

The table given below presents the field byte size and meaning:

|     FIELD     | SIZE |                    DESCRIPTION                        |
|     :---:     |:---: |                       :---:                           |
| Message ID    | 1    | Message identifier. Decimal Value is 2                |
| Travel time   | 4    | Message travel time from sink to node in milliseconds |

## Application architecture

The evaluation application implements a user application which runs in a cooperative
manner with the Wirepas Mesh stack which is in charge of handling all Wirepas network
related tasks. The [diagram](https://wirepas.github.io/wm-sdk/) displayed on the
linked page illustrates how the user application interacts the Wirepas Mesh Stack.

This application depends on:
* the Single-MCU api to send and receive messages
* the SDK HAL to control the LEDs and monitor button's event
* the SDK Libraries to
    * process and filter incoming packets
    * schedule the different application's processing

## Testing

To test the application, a minimal network must be set as described in the
Wirepas provided "How to" documents and via the Wirepas terminal the message
data given in the same document must be send to the node running the evaluation code.

## Customization
By default, the evaluation application operates in **Low-energy** mode meaning
the node is not constantly listening to the radio channel reducing power
consumption. It is possible to change the operating mode (at compile time)
to **Low-latency** (node is always listening to the radio channel) by editing
the _config.mk_ file in the folder application.
To do that **default_operation_mode** should be set according to the desired
mode. A value of 1 will enable the Low-latency mode and a value of 0 will enable
the Low-energy one (default configuration).
