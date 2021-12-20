# Copyright 2021 Wirepas Ltd. All Rights Reserved.
#
# See file LICENSE.txt for full license details.
#

#
# This script is an example built on top of wirepas-mqtt-library wheel >= 1.1 to
# demonstrate the local provisioning feature.
#
# Once started, it connects to an mqtt broker and will do the following operation:
#    - It will configure any new connected sink without a network address to the new_config hardcoded bellow as an example
#    - It will wait for user input to enable or disable the local provisioning in the network
#
import argparse
import logging
import hashlib
import sys

try:
    from wirepas_mqtt_library import WirepasNetworkInterface, WirepasTLVAppConfigHelper
    import wirepas_mesh_messaging as wmm
except ModuleNotFoundError:
    print("Please install Wirepas mqtt library wheel (>= 1.1): pip install wirepas-mqtt-library==1.1")
    sys.exit(-1)

local_provisioning_enabled = False

# Hardcoded config as an example
new_config = {
    "node_role": 17, # Sink with Low Latency flag: 0x11
    "network_address": 123456,
    "network_channel": 16,
    # Fake keys to illustrate the concept
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

    args = parser.parse_args()

    def enable_local_provisioning(enable, psk_id=None, psk=None):
        global local_provisioning_enabled

        app_config_helper = WirepasTLVAppConfigHelper(wni,
                                                      network=new_config["network_address"])

        res = app_config_helper.setup_local_provisioning(enable, psk_id, psk)\
                               .update_entries(override=True)
        if res:
            local_provisioning_enabled = enable
        print("Updating local provisioning result = %s" % res)

    def on_config_changed():
        for gw, sink, config in wni.get_sinks(network_address=None):
            try:
                network_address = config["network_address"]
                continue
            except KeyError:
                print("Sink {}/{} not configured yet".format(gw, sink))
            
            print("Configuring the sink for new network")
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

            if local_provisioning_enabled:
                enable_local_provisioning(local_provisioning_enabled)


    logging.basicConfig(format='%(levelname)s %(asctime)s %(message)s', level=logging.WARNING)
    wni = WirepasNetworkInterface(args.host,
                                  args.port,
                                  args.username,
                                  args.password,
                                  insecure=args.insecure)

    wni.set_config_changed_cb(on_config_changed)

    while True:
        choice = input("\n(E)nable local provisioning, (D)isable local provisioning, (Q)uit:\n")
        if choice=="Q":
            break
        elif choice=="E":
            print("Enabling local provisioning in network 0x%x" % new_config["network_address"])
            enable_local_provisioning(True)
        elif choice=="D":
            print("Disabling local provisioning in network 0x%x" % new_config["network_address"])
            enable_local_provisioning(False)
        else:
            print("Wrong choice E, D or Q")
