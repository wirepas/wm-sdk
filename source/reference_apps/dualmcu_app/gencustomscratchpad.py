#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# gencustomscratchpad.py - A tool to generate custom readable scratchpads
#
# Requires:
#   - Python 2 v2.7 or newer or Python 3 v3.2 or newer

import sys
import os
import struct


# Magic 16-byte string for locating a combi scratchpad in Flash
SCRATCHPAD_V1_TAG = b"SCR1\232\223\060\202\331\353\012\374\061\041\343\067"

# Maximum number of bytes in an input file
MAX_NUM_BYTES_PER_FILE = (1024 * 1024)  # One megabyte

# Special CMAC tag constant value that marks a custom readable scratchpad
CMAC_TAG_CONSTANT = b"\xff" * 16


def crc16_ccitt(data, initial = 0xffff):
    '''Simple and slow version of CRC16-CCITT'''

    # OPTIMIZE: Avoid extra conversions between bytearrays and bytes
    crc = initial
    data = bytearray(data)

    for b in data:
        crc = (crc >> 8) | ((crc & 0xff) << 8)
        crc ^= b
        crc ^= (crc & 0xf0) >> 4
        crc ^= (crc & 0x0f) << 12
        crc ^= (crc & 0xff) << 5

    return crc


def make_header(data):
    '''
    From bootloader.h:

    struct
    {
        /** Number of bytes, not including header */
        uint32_t length;
        /** CRC16-CCITT, not including any header bytes */
        uint16_t crc;
        /** Sequence number of data in scratchpad: 0 .. 255 */
        bl_scratchpad_seq_t seq;    /* uint8_t */
        /** Padding, reserved for future use, must be 0 */
        uint8_t pad;
        /** Scratchpad type information for bootloader: bl_header_type_e */
        uint32_t type;
        /** Status code from bootloader: bl_scratchpad_status_e */
        uint32_t status;
    };

    typedef enum
    {
        ...
        /** Scratchpad contains data that the bootloader can process */
        BL_SCRATCHPAD_TYPE_PROCESS                  = 0x00000000
    } bl_scratchpad_type_e;

    typedef enum
    {
        /** Bootloader has not yet processed the scratchpad contents */
        BL_SCRATCHPAD_STATUS_NEW                    = 0xFFFFFFFF,
        ...
    } bl_scratchpad_status_e;
    '''

    # Calculate length of scratchpad contents
    length = len(CMAC_TAG_CONSTANT) + len(data)

    if length % 16 != 0:
        raise ValueError("data length not multiple of 16")

    # Calculate CRC of scratchpad contents
    crc = crc16_ccitt(CMAC_TAG_CONSTANT)
    crc = crc16_ccitt(data, crc)

    pad = 0x00

    bltype = 0x00000000     # BL_SCRATCHPAD_TYPE_PROCESS
    blstatus = 0xffffffff   # BL_SCRATCHPAD_STATUS_NEW
    seq = 255               # Will be overwritten when scratchpad will be
                            # uploaded to sink

    return struct.pack("<LH2B2L", length, crc, seq, pad, bltype, blstatus)


def main():
    '''Main program'''

    # Determine program name, for error messages
    pgmname = os.path.split(sys.argv[0])[-1]

    # Parse command line arguments
    if len(sys.argv) != 3:
        sys.stderr.write("Usage: %s input_data_file output_otap_file" % pgmname)
        return 1

    infile = sys.argv[1]
    outfile = sys.argv[2]

    # Read input file
    with open(infile, "rb") as f:
        indata = f.read(MAX_NUM_BYTES_PER_FILE + 1)
        if len(indata) > MAX_NUM_BYTES_PER_FILE:
            sys.stderr.write("%s: input file too large" % pgmname)
            return 1

    # Write output file
    with open(outfile, "wb") as f:
        f.write(SCRATCHPAD_V1_TAG)
        f.write(make_header(indata))
        f.write(CMAC_TAG_CONSTANT)
        f.write(indata)

    return 0


# Run main
if __name__ == "__main__":
    sys.exit(main())
