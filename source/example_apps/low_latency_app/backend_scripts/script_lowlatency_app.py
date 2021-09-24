"""
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
Copyright 2021 Wirepas Ltd. All Rights Reserved.
See file LICENSE.txt for full license details.
"""

import sys                  # to handle script exit process
import argparse             # to parse command line given argument
from cmd import Cmd         # to provide the interactive command prompt
from struct import pack     # to pack datas to send

"""
check if all the script's required packages are installed (first Wirepas MQTT library
then Wirepas mesh messaging (i.e gateway API handler)).
"""
try:
    from wirepas_mqtt_library import WirepasNetworkInterface
    import wirepas_mesh_messaging as wmm
except ModuleNotFoundError:
    print("Please install Wirepas mqtt library wheel: pip install wirepas-mqtt-library==1.0")
    print("Please install Wirepas mesh messaging v1.1 wheel: pip install wirepas-mesh-messaging")
    sys.exit(-1)
"""
==== Script global variable ====
"""

# Messages id :
# 0x01 id is reserved for node to node message.
MSG_ID_LED_SET_STATE = 2
MSG_ID_GROUPING = 3

# LED is switched OFF.
LED_STATE_OFF = 0x00
# LED is switched ON.
LED_STATE_ON = 0x01

LED_ID = 1
# Application payload endianness.
MSG_PAYLOAD_ENDIANNESS = "little"

PACKET_LOW_LATENCY_APP_ENDPOINT = 25



def _bytes_to_str_hex_format(bytes_data):
    """ Returns a string of byte expressed in hexadecimal from a bytearray."""
    return "".join("{:02X} ".format(x) for x in bytes_data)


class BackendShell(Cmd):
    """
    LowLatency app shell.
    Class which allows a user to define the script behavior (i.e only receive data
    from node(s)).
    """

    intro = 'Simple backend console.   Type help or ? to list commands.\n'

    def __init__(self, wirepas_nw_iface):
        Cmd.__init__(self)
        Cmd.do_help(self, None)  # print supported command list.
        # Init wirepas network interface data structure.
        self.wni = wirepas_nw_iface
    
    def _convert_address(self, addr):
        """
        Function to check and convert an address parameter.
        :keyword
        addr (str) -- some node/network address got from command line either in decimal or hexadecimal format

        :return
        addr (int) -- node/network address converted from decimal or hexadecimal format to integer
        res (Bool) -- conversion status (True: conversion successful, False: conversion failed)
        """
        res = False

        # try to convert parameter from decimal or hexadecimal format to integer
        try:
            addr = int(addr)
            res = True
        except ValueError:
            try:
                addr = int(addr, 16)
                res = True
            except ValueError:
                pass
        return addr, res

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


    def do_send_multicast_grouping(self, arg):
        "Send a message to a specific node to add it to a multicast group."
        "Format: send_multicast_grouping <gateway ID> <sink ID> <destination address> <multicast address>"
        "Multicast address must respect the following format : 0x80xxxxxx"
        
        """
        Function to encode and send < Grouping command> message to node with given arguments.
        """ 
        #get command parameters.
        try:
            gw_id, sink_id, dst_addr, mcast_addr = arg.split()
        except ValueError:
            print("Wrong parameters format given ! Must be <gateway ID> <sink ID> <destination address> <multicast address>")
            return
        #Check parameters and convert them to the right data type.
        (dst_addr, res) = self._convert_address(dst_addr)
        if res:
            #conversion successful so process next parameters :
            (mcast_addr, res) = self._convert_address(mcast_addr)
            
        else:
            print("Wrong parameter value given ! <multicast address> must be an integer (decimal or hexadecimal format)")
            return

        # Create payload with format <message ID (1 byte)> <Multicast address (4 bytes)> .
        payload = pack('<BI', MSG_ID_GROUPING, mcast_addr)

        self._send_data_request_message(gw_id,
                                        sink_id,
                                        payload,
                                        dst_addr,
                                        PACKET_LOW_LATENCY_APP_ENDPOINT,
                                        PACKET_LOW_LATENCY_APP_ENDPOINT,
                                        1)
    
        def do_bye(self, arg):
            "Close network interface and the console and exit. Format: bye"
            exit()
            return True     


    def do_send_led_on_command(self, arg):
        "Send message to set node(s) LED state on. Format: send_led_on_command <gateway ID> <sink ID> <destination address>"
        """
        Function to encode and send <LED set state> message to node with the given argument
        from shell.
        """
        # get command parameters.
        try:
            gw_id, sink_id, dst_addr = arg.split()
        except ValueError:
            print("Wrong parameters format given ! Must be <gateway ID> <sink ID> <destination address> ")
            return

        # Check parameters and convert them to the right data type.
        (dst_addr, res) = self._convert_address(dst_addr)
        if res:
            # conversion successful so process next parameters
            led_state = LED_STATE_ON
        else:
            print("Wrong parameter value given ! <destination address> must be an integer (decimal or hexadecimal format)")
            return

        # Create payload with format <message ID (1 byte)> <LED ID (1 byte)> <LED state (1 byte)>
        payload = pack('<BBB', MSG_ID_LED_SET_STATE, LED_ID, led_state)
        self._send_data_request_message(gw_id,
                                        sink_id,
                                        payload,
                                        dst_addr,
                                        PACKET_LOW_LATENCY_APP_ENDPOINT,
                                        PACKET_LOW_LATENCY_APP_ENDPOINT,
                                        1)

    def do_send_led_off_command(self, arg):
        "Send message to set node(s) LED state off. Format: send_led_off_command <gateway ID> <sink ID> <destination address> "
        """
        Function to encode and send <LED set state> message to node with the given argument
        from shell.
        """
        # get command parameters.
        try:
            gw_id, sink_id, dst_addr = arg.split()
        except ValueError:
            print("Wrong parameters format given ! Must be <gateway ID> <sink ID> <destination address>")
            return

        # Check parameters and convert them to the right data type.
        (dst_addr, res) = self._convert_address(dst_addr)
        if res:
            # conversion successful so process next parameters
            led_state = LED_STATE_OFF
        else:
            print("Wrong parameter value given ! <destination address> must be an integer (decimal or hexadecimal format)")
            return

        # Create payload with format <message ID (1 byte)> <LED ID (1 byte)> <LED state (1 byte)>
        payload = pack('<BBB', MSG_ID_LED_SET_STATE, LED_ID, led_state)
        self._send_data_request_message(gw_id,
                                        sink_id,
                                        payload,
                                        dst_addr,
                                        PACKET_LOW_LATENCY_APP_ENDPOINT,
                                        PACKET_LOW_LATENCY_APP_ENDPOINT,
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


def main():
    # Get "Wirepas Network Interface" parameters from command line interface.
    nw_iface_params = network_interface_get_parameters()

    # Connect to MQTT broker.
    print("connecting to {}:{} ...".format(nw_iface_params.host, nw_iface_params.port))

    # Create "Wirepas Network Interface" and enable its logger
    #logging.basicConfig(format='%(levelname)s %(asctime)s %(message)s', level=logging.INFO)
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



    




