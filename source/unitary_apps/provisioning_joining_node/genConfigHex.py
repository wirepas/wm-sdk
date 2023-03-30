#!/usr/bin/env python
# -*- coding: utf-8 -*-

# genConfigHex.py - A tool to prepare an Hex file containing the UID, shared
#                   key and provisioning method of a node. The generated hex
#                   is then flashed at the same time of the app
#                   (final_image_provisioning_joining_node.hex) to customize
#                   the node.
#
# Requires:
#   - Python 2 v2.7 or newer (uses argparse, hextool.py)
#     or Python 3 v3.2 or newer
#   - hextool.py

import yaml
import sys
import os
import platform
import argparse
import textwrap
import struct
sys.path.append(
    os.path.join(os.path.split(sys.argv[0])[0], "..", "..", "..", "tools")
    )
import hextool

STORAGE_MAGIC = 0x647650fb

def to_int(param):
    if type(param) is str:
        return ord(param)
    else:
        return param

def to_bytes(param):
    if platform.sys.version_info.major >= 3:
        if type(param) is str:
            if param.upper().startswith("0X"):
                param = param.upper().replace('0X', '')
                param = bytes.fromhex(param)
            else:
                param = bytes(param, 'utf-8')
        elif type(param) is int:
            param = param.to_bytes(max(1, (param.bit_length() + 7) // 8), byteorder='big')
    else:
        if type(param) is str:
            if param.upper().startswith("0X"):
                param = param.upper().replace('0X', '').replace(' ', '')
                param = bytes(param.decode("hex"))
            else:
                param = bytes(param)

    return param

def parse_address(param):
    try:
        address = int(param, 0)
        return address
    except ValueError:
        raise ValueError("invalid address value: '%s'" % param)

def main():
    '''Main program'''
    # Determine program name, for error messages.
    pgmname = os.path.split(sys.argv[0])[-1]

    # Determine help text width.
    try:
        help_width = int(os.environ['COLUMNS'])
    except (KeyError, ValueError):
        help_width = 80
    help_width -= 2

    parser = argparse.ArgumentParser(
        prog=pgmname,
        formatter_class=argparse.RawTextHelpFormatter,
        description=textwrap.fill(
            "A tool to generate a hex file to customize node for \
                provisioning."))

    parser.add_argument("infilespec",
                        metavar="INFILESPEC", default="config.yml", help="yml personalization file")
    parser.add_argument("--output", "-o",
                        metavar="OUTFILESPEC",
                        help="The output file")
    parser.add_argument("--address", "-a",
                        type=parse_address,
                        metavar="VALUE",
                        required=True,
                        help="Address of the storage area ([area:app_persistent]) "
                             "defined in mcu/<mcu>/ini_files/<mcu>_app.ini file\n"
                             "Default values per mcu are :\n"
                             " - NRF52832         :  499712  (0x7A000)\n"
                             " - NRF52833         :  499712  (0x7A000)\n"
                             " - NRF52840         :  1024000 (0xFA000)\n"
                             " - EFR32xg1x (512k) :  503808  (0x7B000)\n"
                             " - EFR32xg1x (1024k):  1028096 (0xFB000)\n"
                             " - EFR32xg22 (512k) :  491520  (0x78000)\n"
                             " - EFR32xg21 (768k) :  753664  (0xB8000)\n"
                             " - EFR32xg21 (1024k):  1015808 (0xF8000)")

    try:
        args = parser.parse_args()

    except (ValueError, IOError, OSError) as exc:
        sys.stdout.write("%s: %s\n" % (pgmname, exc))
        return -1

    try:
        with open(args.infilespec, 'r') as ymlfile:
            cfg = yaml.load(ymlfile, Loader=yaml.FullLoader)
    except yaml.YAMLError as exc:
        sys.stdout.write("%s: Error opening %s (%s)\n" % (pgmname,
                                                          args.infilespec,
                                                          exc))
        return -1

    try:
        if "uid" in cfg['provisioning'].keys():
            uid = to_bytes(cfg['provisioning']["uid"])
        elif cfg['provisioning']["method"] == 3:
            node_uid = to_bytes(cfg['provisioning']['node_uid'])
            node_uid_type = to_bytes(cfg['provisioning']['node_uid_type'])
            authenticator_uid = to_bytes(cfg['provisioning']['authenticator_uid'])
            authenticator_uid_type = to_bytes(cfg['provisioning']['authenticator_uid_type'])
            uid = authenticator_uid_type + authenticator_uid + node_uid_type + node_uid
        else:
            raise KeyError

        method = to_bytes(cfg['provisioning']['method'])
    except KeyError:
        sys.stdout.write("%s: UID and Method are mandatory\n" % (pgmname))
        return -1
    try:
        key = to_bytes(cfg['provisioning']['factory_key'])
    except KeyError:
        key = b''

    sys.stdout.write("%s - UID: %s (len: %d)\n" % (pgmname, uid.hex(), len(uid)))
    sys.stdout.write(
        "%s - KEY: %s (len: %d)\n" % (pgmname,
                        "".join("{:02X}".format(to_int(x)) for x in key), len(key)))
    sys.stdout.write("%s - Method: %d\n" % (pgmname, int.from_bytes(method, byteorder='big')))

    data = struct.pack("<I", STORAGE_MAGIC) + \
        method + \
        struct.pack("B", len(uid)) + \
        uid + \
        struct.pack("B", len(key)) + \
        key

    memory = hextool.Memory()
    memory.cursor = args.address
    memory += data

    # Save output file.
    if args.output is not None:
        hextool.save_intel_hex(memory, filename=args.output)
        sys.stdout.write("%s - Output file: %s\n" % (pgmname, args.output))
    else:
        filename = os.path.splitext(args.infilespec)[0] + ".hex"
        hextool.save_intel_hex(memory, filename=filename)
        sys.stdout.write("%s - Output file: %s\n" % (pgmname, filename))


# Run main.
if __name__ == "__main__":
    sys.exit(main())
