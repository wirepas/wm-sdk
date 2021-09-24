# Low-latency application python script

## Script scope
This script allows to send Low-Latency application's messages to nodes, part of a Wirepas Massive Network 
via a Wirepas gateway connected to the mesh using the **Wirepas-gateway-API**.

## Script building blocks
The script is built on several python modules listed below:
- **wirepas-mqtt-library** version 1.0 (from Wirepas) mainly to control a Wirepas network (e.g send and receive).
- **Cmd** (from python built-in modules) to create an interactive shell which allows users to list all script commands, their help and enter
commands to send to the network.

## Usage Example
This script is only able to send messages as a controller 
(i.e it ask user input, parses the given command, encodes it and send the command to a wirepas network).

The general and full script command line format is:
    python script_lowlatency_app.py -s \<MQTT broker host address\> -p \<MQTT broker port> -u \<username (optional) \> -pw \<password (optional)\> -fu \<select unsecure connection mode to MQTT broker (i.e disable SSL/TLS) (optional)\> <br>
Note: full command line interface can also be obtained with:
    python  script_lowlatency_app.py -h

### Send messages
To send messages:
1. Use the same steps as described in the previous section to execute the script
2. In the interactive shell, enter the following command to display the list of supported commands:
    (cmd) ? <br>
Note: Here only the send_* and bye commands must be used.
3. Enter the command to send to the network. An example is given below (send_led_off_command command is sent):
    (cmd) send_led_off_command gateway_name sink_name node_id <br>
The commands send_led_off_command and send_led_on_command are set OFF or ON led 1 of the board.

NOTE: This command works in unicast, broadcast and multicast, just by changing the node address with the one needed.

For more infos about multicast and broadcast addresses, please have a look at the enum 
app_special_addr_e <br> 
https://wirepas.github.io/wm-sdk/master/d2/d39/app_8h.html#a2fe888af6a0b27cd2455757d0823a313a3b39aa91fd467f2e0dfd127f0d974f77

From there the script is endlessly waiting for user input. To stop the script, just enter the following command
which will close the connection to the Wirepas network: <br>
    (cmd) bye

## Script execution flow

The script works as follow:
1. get network interface parameters from command line (thanks to the network_interface_get_parameters() function)
2. create a Wirepas network interface and connect to the Wirepas network
3. start an interactive shell (thanks to the Backendshell class)
4. Wait for user input
5. Encode and send the message thanks to the Wirepas Network interface.
