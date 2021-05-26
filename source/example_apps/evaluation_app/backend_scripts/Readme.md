# Evaluation application python script

## Script scope

This script allows to send and received specific Evaluation app's messages to/from nodes, part of a Wirepas Mesh network
via a Wirepas gateway connected to the mesh using the **Wirepas-gateway-API**.

## Script building blocks
The script is built on several python modules listed below:
- **wirepas-mqtt-library** version 1.0 (from Wirepas) mainly to control a Wirepas network (e.g send and receive).
- **Cmd** (from python built-in modules) to create an interactive shell which allows users to list all script commands, their help and enter
commands to send to the network.

## Usage example
In order to be able to receive and send messages at the same time, the script needs to be run
in two separate shells. One will only act as a viewer (i.e it receives, decodes and display messages coming from
a Wirepas Mesh network) and the other as a controller (i.e it asks user input, parses the given command, encodes
it and send the command to a Wirepas Mesh network).

The general and full script command line format is:
    python script_evaluation_app.py -s \<MQTT broker host address\> -p \<MQTT broker port> -u \<username (optional) \> -pw \<password (optional)\> -fu \<select unsecure connection mode to MQTT broker (i.e disable SSL/TLS) (optional)\> <br>
Note: full command line interface can also be obtained with:
    python script_evaluation_app.py -h

The following sections describe how to use the script to receive or/and send messages from/to the Evaluation application message set.

### Receive messages
To receive messages:
1. open a shell either bash or Window PowerShell
2. go to where this script is located on your machine using the "cd" shell command
3. enter a command which follows the format given in the previous section. An example is given (connection to a local MQTT broker with unsecure connection mode):<br>
    python script_evaluation_app.py -s 192.168.1.30 -h 1883 -fu <br>
Note: the parameters given must be identical to the one defined in gateway.env file on the Wirepas gateway.
4. in the new prompt enter:
    (cmd) enable_messages_reception <br>
Note: this command enables the reception of data published on the "gw-event/send_data/+/+/+/1/1" topic where messages sent by nodes running
the Evaluation app are received.

From there the script is endlessly waiting for messages to be received or "Enter" key pressed input event to stop reception.

### Send messages
To send messages:
1. Use the same steps as described in the previous section to execute the script
2. In the interactive shell, enter the following command to display the list of supported commands:
    (cmd) ? <br>
Note: Here only the send_* and bye commands must be used.
3. Enter the command to send to the network. An example is given below (echo command is sent):
    (cmd) send_echo_command wp_evaluation_app_gw sink1 348791499 <br>
Note: To get each message format see the associated HowTo documentation or enter "help \<command name\>" (e.g help send_echo_command).

From there the script is endlessly waiting for user input. To stop the script, just enter the following command
which will close the connection to the Wirepas network: <br>
    (cmd) bye

## Script execution flow
The script works as follow:
1. get network interface parameters from command line (thanks to the network_interface_get_parameters() function)
2. create a Wirepas network interface and connect to the Wirepas network
3. start an interactive shell (thanks to the Backendshell class)
    - if *enable_messages_reception* command is selected:
        - subscribe to the topic where messages are published
        - wait for the **on_message_event_data_received_callback** to be called. <br>
        This is in this function that all the message application specifc payload handling is done i.e decoding (here implemented by the RxMsg class) to parse the payload
    - if *send_\** command are selected:
        - wait for user input
        - encode and send the message thanks to the Wirepas network interface

