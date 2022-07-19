# Copyright 2021 Wirepas Ltd. All Rights Reserved.
#
# See file LICENSE.txt for full license details.
#
import argparse
from wirepas_mqtt_library import WirepasNetworkInterface
import wirepas_mesh_messaging as wmm
from struct import unpack
import time

import logging

if __name__ == "__main__":
    parser = argparse.ArgumentParser(fromfile_prefix_chars='@')
    parser.add_argument('--host',
                        help="MQTT broker address")
    parser.add_argument('--port', default=8883,
                        type=int,
                        help="MQTT broker port")
    parser.add_argument('--username', default='mqttmasteruser',
                        help="MQTT broker username")
    parser.add_argument('--password',
                        help="MQTT broker password")
    parser.add_argument('--insecure',
                        dest='insecure',
                        action='store_true',
                        help="MQTT use unsecured connection")

    args = parser.parse_args()

    logging.basicConfig(format='%(levelname)s %(asctime)s %(message)s', level=logging.INFO)

    wni = WirepasNetworkInterface(args.host,
                                  args.port,
                                  args.username,
                                  args.password,
                                  insecure=args.insecure)

    def on_beacon_received(data):
        timestamp = time.strftime('%m/%d/%Y %H:%M:%S',
                              time.gmtime((data.rx_time_ms_epoch - data.travel_time_ms)/1000))
        print("From {} @ {} with RSSI={} dbm: ".format(data.source_address, timestamp, unpack('b', data.data_payload[1:2])[0]) +
          "".join("{:02X} ".format(x) for x in data.data_payload[3:]))

    # Register for any data
    wni.register_data_cb(on_beacon_received, src_ep=18, dst_ep=18)

    input("Press any key to quit")
            
