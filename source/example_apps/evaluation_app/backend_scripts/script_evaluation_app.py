"""
Copyright 2021 Wirepas Ltd. All Rights Reserved.
See file LICENSE.txt for full license details.
"""

import sys                  # to handle script exit process
import time                 # to get message reception time
import argparse             # to parse command line given argument
from struct import unpack   # to handle message's payload byte arrays
from cmd import Cmd         # to provide the interactive command prompt
import logging              # to enable wirepas mqtt library logger


"""
check if all the script's required packages are installed (first Wirepas MQTT library
then Wirepas mesh messaging (i.e gateway API handler)).
"""
try:
    from wirepas_mqtt_library import WirepasNetworkInterface
except ModuleNotFoundError:
    print("Please install Wirepas mqtt library wheel: pip install wirepas-mqtt-library==1.0")
    sys.exit(-1)

try:
    import wirepas_mesh_messaging as wmm
except ModuleNotFoundError:
    print("Please install Wirepas mesh messaging wheel: pip install wirepas-mesh-messaging")
    sys.exit(-1)

"""
==== Script global variable ====
"""
# Message ID of Evaluation application messages set.

# "Periodic message" message ID.
MSG_ID_PERIODIC_MSG = 0
# "Button pressed event" message ID.
MSG_ID_BUTTON_EVENT_MSG = 1
# "Echo command response" message ID.
MSG_ID_ECHO_RESPONSE_MSG = 2
# "LED get state response" message ID.
MSG_ID_LED_GET_STATE_RESPONSE_MSG = 3
# "Periodic message period set" message ID.
MSG_ID_PERIODIC_MSG_PERIOD_SET_MSG = 128
# "LED set state" message ID.
MSG_ID_LED_SET_STATE_MSG = 129
# "LED get state" message ID.
MSG_ID_LED_GET_STATE_MSG = 130
# "Echo command" message ID.
MSG_ID_ECHO_COMMAND_MSG = 131
# Unknown or unsupported message ID value.
MSG_ID_INVALID_UNSUPPORTED_MSG = -1

# "Periodic message" expected byte length.
EXPECTED_LENGTH_PERIODIC_MSG = 13
# "Button event" message expected byte length.
EXPECTED_LENGTH_BUTTON_EVENT_MSG = 3
# "Echo command response" message expected byte length.
EXPECTED_LENGTH_ECHO_RESPONSE_MSG = 5
# "LED get state response" message expected byte length.
EXPECTED_LENGTH_LED_GET_STATE_RESPONSE_MSG = 5

# Button pressed event ID.
BUTTON_PRESSED_STATE = 2

# LED is switched OFF.
LED_STATE_OFF = 0
# LED is switched ON.
LED_STATE_ON = 1

# Application "periodic message period setup message" minimal allowed
# period value in milliseconds (2 seconds).
PERIODIC_MSG_PERIOD_SET_MIN_VAL_MS = 2000

# Application "periodic message period setup message" maximal allowed
# period value in milliseconds (20 minutes).
PERIODIC_MSG_PERIOD_SET_MAX_VAL_MS = 1200000

# List of all supported message ID by this script.
MSG_SUPPORTED_LIST = [MSG_ID_PERIODIC_MSG,
                      MSG_ID_BUTTON_EVENT_MSG,
                      MSG_ID_ECHO_RESPONSE_MSG,
                      MSG_ID_LED_GET_STATE_RESPONSE_MSG]
# Application payload endianness.
MSG_PAYLOAD_ENDIANNESS = "little"

# Endpoint where data coming from node running the evaluation application are expected.
UPLINK_PACKET_EVAL_APP_ENDPOINT = 1

"""
Evaluation application message set handling class & functions.
"""


def _bytes_to_str_hex_format(bytes_data):
    """ Returns a string of byte expressed in hexadecimal from a bytearray."""
    return "".join("{:02X} ".format(x) for x in bytes_data)


class RxMsg:
    """
    Evaluation app received message class.
    This is the class to use when a message sent by a node running the Wirepas "evaluation_app"
    has to be decoded and displayed.
    """
    def __init__(self, payload=None):
        """
        Initialize class instance attributes.

        :keyword
        payload (bytearray) -- received message data payload (default None)

        :return:
        raw_data (bytearray) -- received message data payload (default None)
        msg_id (byte) -- message ID decoded from given data payload (value is None at init time)
        msg_fields (list) -- message decoded fields data (value is None at init time)
        """
        self.raw_data = payload
        self.msg_id = None
        self.msg_fields = []

    def _parse_periodic_message(self):
        """ Parse Evaluation app <periodic message>. """
        # Fill fields list with new payload data
        self.msg_fields.append(list(unpack('<I', self.raw_data[1:5])).pop())  # counter value
        self.msg_fields.append(self.raw_data[5:])               # data pattern

        return True

    def _parse_button_event_message(self):
        """ Parse Evaluation app <button event message>. """
        # Fill fields list with new payload data
        self.msg_fields.append(list(unpack('<B', self.raw_data[1:2])).pop())  # button ID
        self.msg_fields.append(list(unpack('<B', self.raw_data[2:])).pop())  # button state

        return True

    def _parse_led_get_state_response_message(self):
        """ Parse Evaluation app <led get state response message>. """
        # Fill fields list with new payload data
        self.msg_fields.append(list(unpack('<B', self.raw_data[1:2])).pop())  # LED ID
        self.msg_fields.append(list(unpack('<B', self.raw_data[2:3])).pop())  # LED state

        return True

    def _parse_echo_response_message(self):
        """ Parse Evaluation app <parse echo response message>. """
        # Fill fields list with new payload data
        self.msg_fields.append(list(unpack('<I', self.raw_data[1:])).pop())  # Echo request travel time

        return True

    def _display_periodic_message(self):
        """ Display Evaluation app <periodic message> in a fancy way. """
        print("<periodic message>" + '\n'
              + "Raw data -> " + _bytes_to_str_hex_format(self.raw_data) + '\n'
              + "Message ID -> " + str(self.msg_id) + '\n'
              + "Counter value -> " + str(self.msg_fields[0]) + '\n'
              + "Data pattern -> " + _bytes_to_str_hex_format(self.msg_fields[1]))

    def _display_button_event_message(self):
        """ Display Evaluation app <button pressed notification> in a fancy way. """
        # Check received button state to display a string instead of a binary
        # value.
        if self.msg_fields[1] == BUTTON_PRESSED_STATE:
            button_state = "PRESSED"
        else:
            button_state = "UNKNOWN"

        print("<button pressed notification>" + '\n'
              + "Raw data -> " + _bytes_to_str_hex_format(self.raw_data) + '\n'
              + "Message ID -> " + str(self.msg_id) + '\n'
              + "Button ID -> " + str(self.msg_fields[0]) + '\n'
              + "Button state -> " + button_state)

    def _display_echo_response_message(self):
        """ Display Evaluation app <echo response message> in a fancy way. """
        print("<echo response message>" + '\n'
              + "Raw data -> " + _bytes_to_str_hex_format(self.raw_data) + '\n'
              + "Message ID -> " + str(self.msg_id) + '\n'
              + "travel time -> " + str(self.msg_fields[0]) + " ms")

    def _display_led_get_state_response_message(self):
        """ Display Evaluation app <led get state message> in a fancy way. """
        # Check received LED state to display a string instead of a binary
        # value.
        if self.msg_fields[1] == LED_STATE_OFF:
            led_state = "OFF"
        elif self.msg_fields[1] == LED_STATE_ON:
            led_state = "ON"
        else:
            led_state = "UNKNOWN"

        print("<led get state response message>" + '\n'
              + "Raw data -> " + _bytes_to_str_hex_format(self.raw_data) + '\n'
              + "Message ID -> " + str(self.msg_id) + '\n'
              + "LED ID -> " + str(self.msg_fields[0]) + '\n'
              + "LED state -> " + led_state)

    def _get_msg_id(self):
        """
        Function to parse <msg ID> field from Evaluation app message payload and
        initialize class instance msg_id attribute.
        """
        message_id = None

        # Check that the payload has at least one byte.
        if len(self.raw_data) > 0:
            # Get message ID field from payload (little-endian bytes order and
            # one unsigned byte to parse.
            message_id = unpack('<B', self.raw_data[0:1])
            # convert to int type
            message_id = int.from_bytes(message_id, MSG_PAYLOAD_ENDIANNESS)

            if message_id not in MSG_SUPPORTED_LIST:
                # Unknown or unsupported message ID received so return an invalid value.
                message_id = MSG_ID_INVALID_UNSUPPORTED_MSG
        else:
            message_id = MSG_ID_INVALID_UNSUPPORTED_MSG

        self.msg_id = message_id

    def parse(self):
        """
        Function to parse Evaluation app message payload and
        initialize class instance attributes. It returns the parsing status (i.e OK or not)

        :return
        parse_status (Bool) -- Evaluation app message payload parsing status (True: success/
        False: fail)
        """
        parse_status = False

        # Clear fields list
        self.msg_fields.clear()
        # Clear message ID attribute
        self.msg_id = None

        # Check payload data has the right data type before trying to parse anything.
        if type(self.raw_data) is bytes:
            # Parse message ID
            self._get_msg_id()

            # Check if message ID parsing was successful and parse message
            # payload accordingly.
            if self.msg_id == MSG_ID_PERIODIC_MSG:
                if len(self.raw_data) == EXPECTED_LENGTH_PERIODIC_MSG:
                    parse_status = self._parse_periodic_message()

            elif self.msg_id == MSG_ID_BUTTON_EVENT_MSG:
                if len(self.raw_data) == EXPECTED_LENGTH_BUTTON_EVENT_MSG:
                    parse_status = self._parse_button_event_message()

            elif self.msg_id == MSG_ID_ECHO_RESPONSE_MSG:
                if len(self.raw_data) == EXPECTED_LENGTH_ECHO_RESPONSE_MSG:
                    parse_status = self._parse_echo_response_message()

            elif self.msg_id == MSG_ID_LED_GET_STATE_RESPONSE_MSG:
                if len(self.raw_data) == MSG_ID_LED_GET_STATE_RESPONSE_MSG:
                    parse_status = self._parse_led_get_state_response_message()
            else:
                # invalid or unsupported message ID received.
                pass
        else:
            # Invalid payload type: return parsing failed.
            pass

        return parse_status

    def display(self, gateway_id, sink_id, node_id, timestamp):
        """
        Function to display Evaluation app message payload fields.

        :keyword
        gateway_id -- gateway identifier where received message comes from
        sink_id -- sink identifier where received message comes from
        node_id -- node identifier where received message comes from
        timestamp -- received message arrival time
        """
        # format timestamp as YYYY-MM-DD HH:MM:SS
        timestamp = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(timestamp))

        # Print header information of received message.
        print("{} received from gateway <{}>, sink <{}>, node <{}> :".format(timestamp, gateway_id, sink_id, node_id))

        # Print message data in a fancy way.
        if self.msg_id == MSG_ID_PERIODIC_MSG:
            self._display_periodic_message()
        elif self.msg_id == MSG_ID_BUTTON_EVENT_MSG:
            self._display_button_event_message()
        elif self.msg_id == MSG_ID_ECHO_RESPONSE_MSG:
            self._display_echo_response_message()
        elif self.msg_id == MSG_ID_LED_GET_STATE_RESPONSE_MSG:
            self._display_led_get_state_response_message()
        else:
            print("<Unknown or unsupported message>")

        print('')  # separate each new message display by one empty line


class BackendShell(Cmd):
    """
    Evaluation app shell.
    Class which allows a user to define the script behavior (i.e only receive data
    from node(s) or only send command to node(s)).
    """
    intro = 'Simple backend console.   Type help or ? to list commands.\n'

    def __init__(self, wirepas_nw_iface):
        Cmd.__init__(self)
        Cmd.do_help(self, None)  # print supported command list.
        # Init wirepas network interface data structure.
        self.wni = wirepas_nw_iface

    def _convert_dst_address(self, dst_addr):
        """
        Function to check and convert destination address parameter.
        :keyword
        dst_addr (str) -- destination address got from command line either in decimal or hexadecimal format

        :return
        dst_addr (int) -- destination address converted from decimal or hexadecimal format to integer
        res (Bool) -- conversion status (True: conversion successful, False: conversion failed)
        """
        res = False

        # try to convert parameter from decimal or hexadecimal format to integer
        try:
            dst_addr = int(dst_addr)
            res = True
        except ValueError:
            try:
                dst_addr = int(dst_addr, 16)
                res = True
            except ValueError:
                print("Wrong parameter value given ! <destination address> must be an integer (decimal or hexadecimal format)")

        return dst_addr, res

    def _send_data_request_message(self, gateway_id, sink_id, payload, dst_addr, src_ep, dst_ep, qos):
        """ Function to forge mqtt request message to send to gateway."""

        # Display "send data request" message.
        print("Sending message to gateway <{}>, to sink <{}>, to node <{}> with payload={}".format(gateway_id,
                                                                                                   sink_id,
                                                                                                   dst_addr,
                                                                                                   _bytes_to_str_hex_format(payload)))
        # Send message.
        try:
            res = self.wni.send_message(gateway_id,
                                        sink_id,
                                        dst_addr,
                                        src_ep,
                                        dst_ep,
                                        payload,
                                        qos,
                                        cb=on_gateway_answer_callback)
        except Exception as e:
            print(str(e))


    def do_enable_messages_reception(self, arg):
        "Enable evaluation application messages reception to/from endpoints 1/1. Format: enable_messages_reception"

        # Register for incoming data and store reception filter ID
        filter_id = self.wni.register_data_cb(on_message_event_data_received_callback,
                                              src_ep=UPLINK_PACKET_EVAL_APP_ENDPOINT,
                                              dst_ep=UPLINK_PACKET_EVAL_APP_ENDPOINT
                                              )

        # Wait for user input to stop messages reception.
        input("Press any key to end messages reception.\n")

        # Unregister from data reception and return to main shell menu.
        self.wni.unregister_data_cb(filter_id)

    def do_send_periodic_msg_period_set_command(self, arg):
        "Send message to change the periodic message period. Format: send_periodic_msg_period_set_command <gateway ID> <sink ID> <destination address> <period value>"

        """
        Function to encode and send <periodic message set period> message to node with the given argument
        from shell.
        """
        # get command parameters.
        try:
            gw_id, sink_id, dst_addr, new_period_ms = arg.split()
        except ValueError:
            print("Wrong parameters format given ! Must be <gateway ID> <sink ID> <destination address> <period value>")
            return

        # Check parameters and convert them to the right data type.
        (dst_addr, res) = self._convert_dst_address(dst_addr)
        if res:
            # conversion successful so process next parameter
            new_period_ms = int(new_period_ms)
        else:
            return

        # check period value range.
        try:
            if ((new_period_ms < PERIODIC_MSG_PERIOD_SET_MIN_VAL_MS)
               or (new_period_ms > PERIODIC_MSG_PERIOD_SET_MAX_VAL_MS)):
                raise ValueError
        except ValueError:
            print("Invalid period range given ! Must be between {} and {}".format(
                                                                                  PERIODIC_MSG_PERIOD_SET_MIN_VAL_MS,
                                                                                  PERIODIC_MSG_PERIOD_SET_MAX_VAL_MS))
            return

        # Create payload with format <message ID (1 byte)> <period value (4 bytes)>
        payload = (MSG_ID_PERIODIC_MSG_PERIOD_SET_MSG.to_bytes(1, MSG_PAYLOAD_ENDIANNESS)
                   + new_period_ms.to_bytes(4, MSG_PAYLOAD_ENDIANNESS))

        self._send_data_request_message(gw_id,
                                        sink_id,
                                        payload,
                                        dst_addr,
                                        UPLINK_PACKET_EVAL_APP_ENDPOINT,
                                        UPLINK_PACKET_EVAL_APP_ENDPOINT,
                                        1)

    def do_send_led_set_state_command(self, arg):
        "Send message to set node(s) LED state. Format: send_led_set_state_command <gateway ID> <sink ID> <destination address> <LED ID> <LED state>"

        """
        Function to encode and send <LED set state> message to node with the given argument
        from shell.
        """
        # get command parameters.
        try:
            gw_id, sink_id, dst_addr, led_id, led_state = arg.split()
        except ValueError:
            print("Wrong parameters format given ! Must be <gateway ID> <sink ID> <destination address> <LED ID> <LED state>")
            return

        # Check parameters and convert them to the right data type.
        (dst_addr, res) = self._convert_dst_address(dst_addr)
        if res:
            # conversion successful so process next parameters
            led_id = int(led_id)
            led_state = led_state.lower()
            if led_state == "on":
                led_state = LED_STATE_ON
            elif led_state == "off":
                led_state = LED_STATE_OFF
            else:
                print("Wrong parameters value given ! <LED ID> must be an integer and <LED state> either 'on' or 'off' ")
                return
        else:
            return

        # Create payload with format <message ID (1 byte)> <LED ID (1 byte)> <LED state (1 byte)>
        payload = (MSG_ID_LED_SET_STATE_MSG.to_bytes(1, MSG_PAYLOAD_ENDIANNESS)
                   + led_id.to_bytes(1, MSG_PAYLOAD_ENDIANNESS)
                   + led_state.to_bytes(1, MSG_PAYLOAD_ENDIANNESS))

        self._send_data_request_message(gw_id,
                                        sink_id,
                                        payload,
                                        dst_addr,
                                        UPLINK_PACKET_EVAL_APP_ENDPOINT,
                                        UPLINK_PACKET_EVAL_APP_ENDPOINT,
                                        1)

    def do_send_led_get_state_command(self, arg):
        "Send message to get node(s) LED state. Format: send_led_get_state_command <gateway ID> <sink ID> <destination address> <LED ID>"

        """
        Function to encode and send <LED get state command> message to node with the given argument
        from shell.
        """
        # get command parameters.
        try:
            gw_id, sink_id, dst_addr, led_id = arg.split()
        except ValueError:
            print("Wrong parameters format given ! Must be <gateway ID> <sink ID> <destination address> <LED ID>")
            return

        # Check parameters and convert them to the right data type.
        (dst_addr, res) = self._convert_dst_address(dst_addr)
        if res:
            # conversion successful so process next parameter
            led_id = int(led_id)
        else:
            return

        # Create payload with format <message ID (1 byte)> <LED ID (1 bytes)>
        payload = (MSG_ID_LED_GET_STATE_MSG.to_bytes(1, MSG_PAYLOAD_ENDIANNESS)
                   + led_id.to_bytes(1, MSG_PAYLOAD_ENDIANNESS))

        self._send_data_request_message(gw_id,
                                        sink_id,
                                        payload,
                                        dst_addr,
                                        UPLINK_PACKET_EVAL_APP_ENDPOINT,
                                        UPLINK_PACKET_EVAL_APP_ENDPOINT,
                                        1)

    def do_send_echo_command(self, arg):
        "Send echo command. Format: send_echo_command <gateway ID> <sink ID> <destination address>"

        """
        Function to encode and send <echo command> message to node with the given argument
        from shell.
        """
        # get command parameters.
        try:
            gw_id, sink_id, dst_addr = arg.split()
        except ValueError:
            print("Wrong parameters format given ! Must be <gateway ID> <sink ID> <destination address>")
            return

        # Check parameters and convert them to the right data type.
        (dst_addr, res) = self._convert_dst_address(dst_addr)
        if res is False:
            return

        # Create payload with format <message ID (1 byte)>
        payload = MSG_ID_ECHO_COMMAND_MSG.to_bytes(1, MSG_PAYLOAD_ENDIANNESS)

        self._send_data_request_message(gw_id,
                                        sink_id,
                                        payload,
                                        dst_addr,
                                        UPLINK_PACKET_EVAL_APP_ENDPOINT,
                                        UPLINK_PACKET_EVAL_APP_ENDPOINT,
                                        1)

    def do_bye(self, arg):
        "Close network interface and the console and exit. Format: bye"

        exit()
        return True


"""
Network interface related functions.
"""


def network_interface_get_parameters():
    """
    Function to get from the script's command line arguments to configure the Wirepas Network Interface
    and return the parsing process results.

    :return: parser.parse_args() instance with the following attributes
    host (str) -- MQTT broker address: Required (default None)
    port (int) -- MQTT broker port number to use for connection: Required (default None)
    force_unsecure (Bool) -- MQTT broker connection mode which enable or not the SSL/TLS usage (default False)
    username (str) -- Username to use for MQTT broker connection: Optional (default None)
    password (str) -- Password to use for MQTT broker connection: Optional (default None)
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', '--host', type=str, default=None,
                        help="MQTT broker address. (No default value.)")
    parser.add_argument('-p', '--port', type=int, default=None,
                        help="MQTT broker port. (No default value.)")
    parser.add_argument('-fu', '--force_unsecure', action='store_true',
                        help="MQTT broker connection mode i.e secure or not. (Default secure)")
    parser.add_argument('-u', '--username', type=str, default=None,
                        help="MQTT broker username. (No default value.)")
    parser.add_argument('-pw', '--password', type=str, default=None,
                        help="MQTT broker password. (No default value.)")

    return parser.parse_args()

def on_gateway_answer_callback(gw_error_code, param):
    """
    Callback called when gateway publishes messages on the "gw-response/" topic in response to a request
    (e.g send message).
    In this script a message will be printed only if an error is raised.
    """
    if gw_error_code != wmm.GatewayResultCode.GW_RES_OK:
        print("Message sending failed: res=%s. Caller param is %s" % (gw_error_code, param))


def on_message_event_data_received_callback(data):
    """
    Callback called when new messages produced by the evaluation application
    on source and destination endpoints #1 are received on the "gw-event/received_data/"
    where all data received by a gateway from a sink can be read.
    See wirepas-mqtt-library module documentation for more info on function parameters.
    """

    # Init and process received message payload.
    msg = RxMsg(data.data_payload)
    if msg.parse():
        msg.display(data.gw_id, data.sink_id, data.source_address, (data.rx_time_ms_epoch/1000))
    else:
        print("Invalid message received !\n")


def main():
    # Get "Wirepas Network Interface" parameters from command line interface.
    nw_iface_params = network_interface_get_parameters()

    # Connect to MQTT broker.
    print("connecting to {}:{} ...".format(nw_iface_params.host, nw_iface_params.port))

    # Create "Wirepas Network Interface" and enable its logger
    logging.basicConfig(format='%(levelname)s %(asctime)s %(message)s', level=logging.INFO)
    wni = WirepasNetworkInterface(nw_iface_params.host,
                                  nw_iface_params.port,
                                  nw_iface_params.username,
                                  nw_iface_params.password,
                                  insecure=nw_iface_params.force_unsecure)

    # Launch shell used to send or receive evaluation application messages
    # to/from the Wirepas Massive network.
    BackendShell(wni).cmdloop()  # Main process.


if __name__ == "__main__":
    main()
