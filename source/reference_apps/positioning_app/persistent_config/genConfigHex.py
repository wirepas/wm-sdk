#!/usr/bin/env python
# -*- coding: utf-8 -*-

# genConfigHex.py - A tool to prepare an Hex file containing the persistent
#                   storage area for positioning application. The generated
#                   hex file is then flashed at the same time of the app
#                   (final_image_positioning_app.hex) to customize the node.
#
# Requires:
#   - Python 3 v3.7 or newer
#                   Relies on dict preserving the order of items. Feature that
#                   was (implemented in python 3.6 and) documented in python 3.7.
#                   Relies on Python f-string formating implemented in python 3.6.
#   - hextool.py

import yaml
import sys
import os
import platform
import argparse
import textwrap
import struct
sys.path.append(
    os.path.join(os.path.split(sys.argv[0])[0], "..", "..", "..", "..", "tools")
    )
import hextool

STORAGE_MAGIC = 0x1E75FED8

def parse_address(param):
    try:
        address = int(param, 0)
        return address
    except ValueError:
        raise ValueError( f"invalid address value: {param}" )

def check_value_against_range(type, value, ranges):
    range_delimiter='-'
    if type in ['int8_t', 'int16_t', 'int32_t']:
        range_delimiter='...'
    for r in ranges.split(','):
        if range_delimiter in r:
            s,e = r.split(range_delimiter)
            if int(s) <= value <= int(e):
                return True
        else:
            if value == int(r):
                return True
    return False

def check_config_against_layout(args, layout, cfg):

    # Check that all structures and variables in configuration file has entry in layout file.
    for st in cfg:
        if st not in layout:
            print( f"ERROR: structure {st} in {args.infilespec} does not exist in layout file {args.layout}" )
            return False
        for va in cfg[st]:
            if va not in layout[st]:
                print( f"ERROR: variable {st}.{va} in {args.infilespec} does not exist in layout file {args.layout}" )
                return False
    return True

def append_data(data, type, val, st, va):
    if type == 'int8_t':
        data = data + struct.pack("b", val)
    elif type == 'int16_t':
        data = data + struct.pack("<h", val)
    elif type == 'int32_t':
        data = data + struct.pack("<i", val)
    elif type == 'uint8_t':
        data = data + struct.pack("<B", val)
    elif type == 'uint16_t':
        data = data + struct.pack("<H", val)
    elif type == 'uint32_t':
        data = data + struct.pack("<I", val)
    elif type == 'bool':
        data = data + struct.pack("<?", val)
    elif type == 'mbcn_record_uint8_t':
        l = len(val)
        if l < 16:
            val = val + [0]*(16-l)
        format_string = "<" + "B"*16
        data = data + struct.pack(format_string, *val)
    else:
        print( f"ERROR: variable {st}.{va} unknown type {type}" )
        return False,data
    return True,data

def process_layout_and_config(args, layout, cfg):

    data = struct.pack("<I", STORAGE_MAGIC)

    # Go through layout and analyze value for all variables in all structures.
    for st in layout:
        for va in layout[st]:
            val = 0
            if 'type' not in layout[st][va]:
                print( f"ERROR: type of {st}.{va} not defined in layout file {args.layout}" )
                return False,data
            if 'optional' not in layout[st][va]:
                print( f"ERROR: optionality of {st}.{va} not defined in layout file {args.layout}" )
                return False,data
            if va in cfg[st]:
                val = cfg[st][va]
            else:
                if layout[st][va]['optional'] == False:
                    print( f"ERROR: mandatory {st}.{va} missing" )
                    return False,data
                else:
                    if 'default_value' not in layout[st][va]:
                        print( f"ERROR: optional {st}.{va} does not have default value in layout file {args.layout}" )
                        return False,data
                    val = layout[st][va]['default_value']
            if 'range' in layout[st][va]:
                if not check_value_against_range(layout[st][va]['type'], val, str(layout[st][va]['range'])):
                    print( f"ERROR: variable {st}.{va} value {val} not within range {layout[st][va]['range']}" )
                    return False,data
            status,data = append_data(data, layout[st][va]['type'], val, st, va)
            if not status:
                return False,data

    return True,data

def main():
    '''Main program'''

    # Determine program name, for error messages.
    pgmname = os.path.split(sys.argv[0])[-1]

    # Verify python version is 3.7 or newer.
    if sys.version_info.major < 3 or (sys.version_info.major == 3 and sys.version_info.minor < 7):
        print( f"{pgmname} ERROR: Python version {sys.version_info.major}.{sys.version_info.minor} not supported")
        return -1

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
                positioning application."))

    parser.add_argument("infilespec",
                        metavar="INFILESPEC", help="yml personalization file")
    parser.add_argument("--output", "-o",
                        metavar="OUTFILESPEC",
                        help="The output file")
    parser.add_argument("--layout", "-l",
                        metavar="LAYOUTFILESPEC",
                        help="The layout file")
    parser.add_argument("--address", "-a",
                        type=parse_address,
                        metavar="VALUE",
                        required=True,
                        help="Address of the storage area ([area:app_persistent])\n"
                             "defined in scratchpad_ini/scratchpad_<mcu>.ini file\n"
                             "Default values per mcu are :\n"
                             " - NRF52832         : 512000  (0x7D000)\n"
                             " - NRF52833         : 512000  (0x7D000)\n"
                             " - NRF52840         : 1036288 (0xFD000)\n"
                             " - EFR32xg1x (512k) : 518144  (0x7E800)\n"
                             " - EFR32xg1x (1024k): 1042432 (0xFE800)\n"
                             " - EFR32xg22 (512k) : 499712  (0x7A000)\n"
                             " - EFR32xg21 (1024k): 1024000 (0xFA000)")

    try:
        args = parser.parse_args()
    except (ValueError, IOError, OSError) as exc:
        sys.stdout.write( f"{pgmname}: {exc}\n" )
        return -1

    try:
        with open(args.layout, 'r') as ymllayoutfile:
            layout = yaml.load(ymllayoutfile, Loader=yaml.FullLoader)
    except yaml.YAMLError as exc:
        sys.stdout.write( f"{pgmname}: Error opening {args.layout} ({exc})\n" )
        return -1

    try:
        with open(args.infilespec, 'r') as ymlfile:
            cfg = yaml.load(ymlfile, Loader=yaml.FullLoader)
    except yaml.YAMLError as exc:
        sys.stdout.write( f"{pgmname}: Error opening {args.infilespec} ({exc})\n" )
        return -1

    if not check_config_against_layout(args, layout, cfg):
        return -1

    status,data = process_layout_and_config(args, layout, cfg)
    if not status:
        return -1

    memory = hextool.Memory()
    memory.cursor = args.address
    memory += data

    # Determine name for output file.
    if args.output is not None:
        outfilename=args.output
    else:
        outfilename = os.path.splitext(args.infilespec)[0] + ".hex"
    # Save output file.
    hextool.save_intel_hex(memory, filename=outfilename)
    sys.stdout.write( f"{pgmname} - Output file: {outfilename}\n" )


# Run main.
if __name__ == "__main__":
    sys.exit(main())
