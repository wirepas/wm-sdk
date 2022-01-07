# Copyright 2021 Wirepas Ltd. All Rights Reserved.
#
# See file LICENSE.txt for full license details.
#
import argparse
from ctypes import FormatError
from random import randint
from wirepas_mqtt_library import WirepasNetworkInterface
import wirepas_mesh_messaging as wmm
import logging
from time import time
import hashlib
from struct import pack, unpack

# Hardcoded config, same as in app config.mk
new_config = {
    "node_role": 17, # Sink with Low Latency flag: 0x11
    "network_address": 0x456efc,
    "network_channel":2,
    "cipher_key": bytes([0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88]),
    "authentication_key": bytes([0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88]),
    "started": True
}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(fromfile_prefix_chars='@')
    parser.add_argument('--host',
                        help="MQTT broker address")
    parser.add_argument('--port',
                        type=int,
                        default=8883,
                        help="MQTT broker port")
    parser.add_argument('--username',
                        help="MQTT broker username")
    parser.add_argument('--password',
                        help="MQTT broker password")
    parser.add_argument('--insecure',
                        dest='insecure',
                        action='store_true',
                        help="MQTT use unsecured connection")
    parser.add_argument('--gateway_id',
                        help="Gateway id to test",
                        required=True)
    parser.add_argument('--sink_id',
                        help="Gateway id to test",
                        default='sink1')

    args = parser.parse_args()

    logging.basicConfig(format='%(levelname)s %(asctime)s %(message)s', level=logging.WARNING)

    session = 0
    num_packets = 0
    size_packets = 0
    rx_dic = None
    first_message = None
    last_message = None

    wni = WirepasNetworkInterface(args.host,
                                  args.port,
                                  args.username,
                                  args.password,
                                  insecure=args.insecure)

    def on_data_received(data):
        global rx_dic
        global first_message

        rx_session, rx_message_id = unpack('<HI', data.data_payload[0:6])
        if rx_session != session:
            print("Rx for different session")
            return

        if rx_dic == None:
            rx_dic = {}
            first_message = time()
        try:
            _ = rx_dic[rx_message_id]
            print("Rx duplicate id!")
        except KeyError:
            last_message = time()
            rx_dic[rx_message_id] = time()

        session_duration = last_message - first_message
        rx_packets = rx_dic.__len__()
        if session_duration < 1:
            pps = -1
        else:
            pps = rx_packets/session_duration

        print("RX %d packets in %fs pps = %f" % (rx_packets, session_duration, pps))

    wni.register_data_cb(on_data_received, network=new_config["network_address"], gateway=args.gateway_id, src_ep=1)

    def start_session(number_of_packets=100, packet_size=50):
        global session
        global rx_dic
        try:
            session = randint(1, 1000)
            rx_dic = None
            print("Starting session ", session)
            res = wni.send_message(args.gateway_id, args.sink_id, 0xffffffff, 10, 10, payload=pack('<HIB', session, number_of_packets, packet_size))
            if res != wmm.GatewayResultCode.GW_RES_OK:
                print("Cannot start session ", res)
        except TimeoutError:
            print("Cannot send data to %s:%s", gw, sink)

        print("Session started")

    found = False
    # Configure the gateway to test
    for gw, sink, config in wni.get_sinks():
        if gw != args.gateway_id:
            continue

        if sink != args.sink_id:
            continue

        found = True

        # Sink is found, let's configure it
        try:
            node_address = config["node_address"]
        except KeyError:
            print("Node address not set, generate one by hashing sink name")
            h = hashlib.blake2b(digest_size=4)
            sink_unique_name = "{}{}".format(gw, sink)
            h.update(bytes(sink_unique_name, encoding='utf8'))
            node_address = h.hexdigest()
            print("New node address is ", int(node_address, 16))
            # Add node address to the sink
            new_config["node_address"] = int(node_address, 16)

        try:
            res = wni.set_sink_config(gw, sink, new_config)
            if res != wmm.GatewayResultCode.GW_RES_OK:
                print("Cannot set new config to %s:%s res=%s" % (gw, sink, res.res))
        except TimeoutError:
            print("Cannot set new config to %s:%s Timeout" % (gw, sink))
        break

    if not found:
        print("Gateway/Sink %s/%s not found" % (args.gateway_id, args.sink_id))
        exit()

    while True:
        choice = input("\n(S)tart session, (Q)uit:")
        if choice=="Q":
            break
        elif choice=="S":
            print("Starting a test session")
            try:
                packets = int(input("Hom many packets?"))
                size = int(input("Packet size?"))
            except FormatError:
                print("Wrong format")
                continue
            start_session(packets, size)
        else:
            print("Wrong choice S or Q")