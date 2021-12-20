# Copyright 2021 Wirepas Ltd. All Rights Reserved.
#
# See file LICENSE.txt for full license details.
#
import argparse
from wirepas_mqtt_library import WirepasNetworkInterface
import wirepas_mesh_messaging as wmm
from struct import pack
import random

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

    parser.add_argument('--gateway',
                        help="From which gateway to send config",
                        required=True)
    parser.add_argument('--sink',
                        help="From which sink to send config",
                        required=True)
    parser.add_argument('--node',
                        help="To which node to send the config",
                        required=True)

    parser.add_argument("cmd", choices=["disable",
                                        "enable",
                                        "periodic"])
    # Param in case of periodic
    parser.add_argument('--period',
                        help="How often in s to scan",
                        default="360")
    parser.add_argument('--scan_len',
                        help="How long to scan for beacons",
                        default="60")

    args = parser.parse_args()

    logging.basicConfig(format='%(levelname)s %(asctime)s %(message)s', level=logging.INFO)

    wni = WirepasNetworkInterface(args.host,
                                  args.port,
                                  args.username,
                                  args.password,
                                  insecure=args.insecure)

    try:
        id = random.randint(0, 256) # It could be any 32 bits
        if args.cmd == "disable":
            payload = pack('<IB', id, 0)
        elif args.cmd == "enable":
            print("Enable scanner")
            # Always any channel
            payload = pack('<IBB', id, 1, 0)
        elif args.cmd:
            print("Enable scanner periodicaly")
            # Always any channel
            payload = pack('<IBHHB', id, 2, int(args.period), int(args.scan_len), 0)

        res = wni.send_message(args.gateway, args.sink, int(args.node), 13, 13, payload)

        if res != wmm.GatewayResultCode.GW_RES_OK:
            print("Cannot send data to %s:%s res=%s" % (args.gatway, args.sink, res))
        else:
            print("Command correctly sent")
    except TimeoutError:
        print("Cannot send data to %s:%s", args.gatway, args.sink)
            
