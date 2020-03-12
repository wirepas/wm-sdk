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

# Import hextool.py, but do not write a .pyc file for it.
dont_write_bytecode = sys.dont_write_bytecode
sys.dont_write_bytecode = True
sys.path.append(
    os.path.join(os.path.split(sys.argv[0])[0], "..", "..", "tools")
    )
import hextool
sys.dont_write_bytecode = dont_write_bytecode
del dont_write_bytecode

STORAGE_MAGIC = 0x647650fb
# Define the address of the storage area, or use --address from command line.
# - NRF52832     : 512000  (0x7D000)
# - NRF52833     : 512000  (0x7D000)
# - NRF52840     : 1036288 (0xFD000)
# - EFR32 (512k) : 518144  (0x7E800)
# - EFR32 (1024k): 1042432 (0xFE800)
STORAGE_AREA_ADDRESS = 0x7D000


def convert2bytes(param):
    if platform.sys.version_info.major >= 3:
        if type(param) is str:
            if param.upper().startswith("0X"):
                param = param.upper().replace('0X', '')
                param = bytes.fromhex(param)
            else:
                param = bytes(param, 'utf-8')
        elif type(param) is int:
            param.to_bytes((param.bit_length() + 7) // 8, byteorder='big')
    else:
        if type(param) is str:
            if param.upper().startswith("0X"):
                param = param.upper().replace('0X', '').replace(' ', '')
                param = bytes(param.decode("hex"))
            else:
                param = bytes(param)

    return param

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
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=textwrap.fill(
            "A tool to generate a hex file to customize node for \
                provisioning."))

    parser.add_argument("infilespec",
                        metavar="INFILESPEC", help="yml personalization file")
    parser.add_argument("--output", "-o",
                        metavar="OUTFILESPEC",
                        help="output file")
    parser.add_argument("--address", "-a",
                        metavar="VALUE",
                        help="Storage area address",
                        type=int)

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
        uid = convert2bytes(cfg['provisioning']['uid'])
        method = convert2bytes(cfg['provisioning']['method'])
    except KeyError:
        sys.stdout.write("%s: UID and Method are mandatory\n" % (pgmname))
        return -1
    try:
        key = convert2bytes(cfg['provisioning']['factory_key'])
    except KeyError:
        key = b''

    sys.stdout.write("%s - UID: %s (len: %d)\n" % (pgmname, uid, len(uid)))
    sys.stdout.write(
        "%s - KEY: %s (len: %d)\n" % (pgmname,
                                        ''.join(format(ord(x), '02X') +
                                        " " for x in key), len(key)))
    sys.stdout.write("%s - Method: %d\n" % (pgmname, method))

    data = struct.pack("<I", STORAGE_MAGIC) + \
        struct.pack("B", method) + \
        struct.pack("B", len(uid)) + \
        uid + \
        struct.pack("B", len(key)) + \
        key

    memory = hextool.Memory()
    if args.address is not None:
        memory.cursor = args.address
    else:
        memory.cursor = STORAGE_AREA_ADDRESS
    memory += data

    # Save output file.
    if args.output is not None:
        hextool.save_intel_hex(memory, filename=args.output)
        sys.stdout.write("%s - Output file: %s\n" % (pgmname, args.output))
    else:
        hextool.save_intel_hex(memory,
                               filename=os.path.splitext(args.infilespec)[0] +
                               ".hex")
        sys.stdout.write("%s - Output file: %s\n" %
                         (pgmname,
                          os.path.splitext(args.infilespec)[0] + ".hex"))


# Run main.
if __name__ == "__main__":
    sys.exit(main())
