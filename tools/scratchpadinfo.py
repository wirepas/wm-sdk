#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# scratchpadinfo.py - A tool to examine scratchpad files
#
# Requires:
#   - Python 2 v2.7 or newer (uses argparse)
#     or Python 3 v3.2 or newer
#   - PyCryptodome v3.0 or newer

import sys
import os
import re
import zlib
import struct
import argparse
import textwrap

from Crypto.Hash import CMAC
from Crypto.Cipher import AES
from Crypto.Util import Counter
from Crypto.Random import get_random_bytes


# Constants

# Magic 16-byte string for locating a combi scratchpad in Flash
SCRATCHPAD_V1_TAG = b"SCR1\232\223\060\202\331\353\012\374\061\041\343\067"

# Minimum number of bytes in a valid combi scratchpad file
SCRATCHPAD_MIN_LENGTH = 48

# Maximum number of bytes in a combi scratchpad file
SCRATCHPAD_MAX_LENGTH = (16 * 1024 * 1024)  # 16 megabytes


# Functions

def crc16_ccitt(data, initial = 0xffff):
    '''Simple and slow version of CRC16-CCITT'''

    crc = initial
    data = bytearray(data)

    for b in data:
        crc = (crc >> 8) | ((crc & 0xff) << 8)
        crc ^= b
        crc ^= (crc & 0xf0) >> 4
        crc ^= (crc & 0x0f) << 12
        crc ^= (crc & 0xff) << 5

    return crc


def create_argument_parser(pgmname):
    '''Create a parser for parsing the command line.'''

    # Determine help text width.
    try:
        help_width = int(os.environ['COLUMNS'])
    except (KeyError, ValueError):
        help_width = 80
    help_width -= 2

    parser = argparse.ArgumentParser(
        prog = pgmname,
        formatter_class = argparse.RawDescriptionHelpFormatter,
        description = textwrap.fill("A tool to examine scratchpad files"))
    parser.add_argument("--configfile", "-c",
        metavar = "FILE",
        help = "a configuration file with keys (not set by default)")

    parser.add_argument("--keyname", "-k",
        metavar = "NAME",
        help = "name of key to try for authentication (try all by default)")
    parser.add_argument("--dump", "-d",
        metavar = "PREFIX",
        help = "write decrypted, decompressed data to files "
               "with a given filename prefix")
    parser.add_argument("--quiet", "-q",
        action = 'store_true',
        help = "reduce verbosity")
    parser.add_argument("infile",
        metavar = "INFILE",
        help = "compressed, encrypted scratchpad file to examine")

    return parser


def read_config(filename):
    '''Read encryption and authentication keys from a configuration file.'''

    def _get_key(value):
        '''Parse an encryption or authentication key value

        Bytes are separated by commas or spaces.'''
        value = value.replace(",", " ")
        return bytearray.fromhex(value)

    keys = {}
    config = {"keys": keys}

    with open(filename, "rU") as file:
        key_name = None
        for line in file:
            sre_key = re.match(r"\[key:([A-Za-z0-9_]+)\].*$", line)
            sre_auth = re.match(r"auth *[:=] *([A-Fa-f0-9, ]+).*$", line)
            sre_encrypt = re.match(r"encrypt *[:=] *([A-Fa-f0-9, ]+).*$", line)

            if sre_key:
                if key_name != None:
                    if "auth" not in keys[key_name]:
                        raise ValueError(
                            "missing authentication key value for key: %s" %
                            key_name)
                    if "encrypt" not in keys[key_name]:
                        raise ValueError(
                            "missing encryption key value for key: %s" %
                            key_name)

                key_name = sre_key.group(1)
                if key_name in keys:
                    raise ValueError("duplicate key: %s" % key_name)
                keys[key_name] = {}
            elif sre_auth:
                if "auth" in keys[key_name]:
                    raise ValueError(
                        "duplicate authentication key value for key: %s" %
                        key_name)
                keys[key_name]["auth"] = _get_key(sre_auth.group(1))
            elif sre_encrypt:
                if "encrypt" in keys[key_name]:
                    raise ValueError(
                        "duplicate encryption key value for key: %s" %
                        key_name)
                keys[key_name]["encrypt"] = _get_key(sre_encrypt.group(1))

    return config


def parse_header(data, filename):
    '''Parse and verify scratchpad header'''

    # Check minimum length.
    if len(data) < SCRATCHPAD_MIN_LENGTH:
        raise ValueError("file too short: %s" % filename)

    # Check maximum length.
    if len(data) > SCRATCHPAD_MAX_LENGTH:
        raise ValueError("file too long: %s" % filename)

    # Check combi scratchpad tag.
    if data[:16] != SCRATCHPAD_V1_TAG:
        raise ValueError("no valid scratchpad tag found: %s" % filename)

    # Unpack scratchpad header fields.
    length, crc, otapseq, pad, bltype, blstatus = struct.unpack("<LH2B2L",
                                                                data[16:32])

    # Verify data length.
    length_indata = len(data) - 32
    if length != length_indata:
        raise ValueError("header data length mismatch (%d vs. %d bytes): %s" %
                         (length, length_indata, filename))

    # Verify CRC.
    crc_indata = crc16_ccitt(data[32:])
    if crc != crc_indata:
        raise ValueError("header CRC mismatch (0x%04x vs. 0x%04x): %s" %
                         (crc, crc_indata, filename))

    scratchpad = {}
    scratchpad["length"] = length
    scratchpad["crc"] = crc
    scratchpad["otapseq"] = otapseq
    scratchpad["pad"] = pad
    scratchpad["bltype"] = bltype
    scratchpad["blstatus"] = blstatus
    scratchpad["data"] = data[32:]

    return scratchpad


def print_header_info(header):
    '''Print scratchpad header information'''

    print("")
    print("Type: Combi scratchpad")
    print("Compressed data length: %d" % header["length"])
    print("Compressed data CRC: 0x%04x" % header["crc"])
    print("OTAP sequence number: %d" % header["otapseq"])
    print("Pad field: 0x%02x" % header["pad"])
    print("Type field: 0x%08x" % header["bltype"])
    print("Status field: 0x%08x" % header["blstatus"])
    print("")


def parse_secure(data, filename):
    '''Parse security-related headers'''

    # Check minimum length.
    if len(data) < 32:
        raise ValueError("file too short: %s" % filename)

    # Unpack security-related header fields.
    cmac, icb = struct.unpack("<16s16s", data[:32])

    secure = {}
    secure["length"] = 32
    secure["cmac"] = cmac
    secure["icb"] = icb

    return secure


def print_secure_info(header):
    '''Print security-related header information'''

    def _bytes_to_hex(str):
        return " ".join(["%02x" % n for n in bytearray(str)])

    print("Security header length: %d" % header["length"])
    print("CMAC / OMAC1: %s" % _bytes_to_hex(header["cmac"]))
    print("Initial Counter Block: %s" % _bytes_to_hex(header["icb"]))
    print("")


def parse_files(data, filename):
    '''Parse file headers'''

    if len(data) < 16:
        raise ValueError("file too short: %s" % filename)

    pos = 0
    indata_length = len(data)
    files = []

    while pos < indata_length:
        if (pos + 16) > indata_length:
            raise ValueError("extra data after files: %s" % filename)

        (areaid, length, ver_major, ver_minor,
         ver_maint, ver_devel, pad) = struct.unpack("<2L4BL",
                                                    data[pos : (pos + 16)])

        f = {}
        f["areaid"] = areaid
        f["length"] = length
        f["ver_major"] = ver_major
        f["ver_minor"] = ver_minor
        f["ver_maint"] = ver_maint
        f["ver_devel"] = ver_devel
        f["pad"] = pad

        # Skip over header.
        pos += 16

        if pos + length > indata_length:
            raise ValueError("not enough data for file: %s" % filename)

        f["data"] = data[pos:(pos + length)]

        files.append(f)

        # Skip over compressed data.
        pos += length

    return files


def print_file_info(files):
    '''Print file information'''

    n = 0

    for f in files:
        print("File %d" % n)
        print("Area ID: 0x%08x" % f["areaid"])
        print("Compressed length: %d" % f["length"])
        print("Version: %d.%d.%d.%d" % (f["ver_major"], f["ver_minor"],
                                        f["ver_maint"], f["ver_devel"]))
        print("Pad field: %d" % f["pad"])
        print("")

        n += 1


def authenticate(cmac, icb, data, keys, quiet, key_name = None):
    '''Authenticate file data.'''

    if len(keys) == 0:
        if not quiet:
            print("No keys, data not authenticated")
        return []

    def _test_key(cmac, icb, data, auth_key):
        '''Try to authenticate ICB, file headers and file data with a key.'''

        # CMAC.new() only accepts bytes, not bytearray.
        auth_key = bytes(auth_key)

        # Create a CMAC / OMAC1 object.
        cobj = CMAC.new(auth_key, ciphermod = AES)

        # Calculate CMAC / OMAC1 over ICB, file headers and file data.
        cobj.update(icb)
        cobj.update(data)

        # Succeeded?
        return cmac == cobj.digest()

    matching_keys = []

    if key_name != None:
        # Try a named key only.
        if _test_key(cmac, icb, data, keys[key_name]["auth"]):
            matching_keys.append(key_name)

        if len(matching_keys) == 0:
            raise ValueError("failed authentication with key: %s" % key_name)
        elif not quiet:
            print("Authentication with key succeeded: %s" % key_name)
    else:
        # Try all keys and return a list of keys that worked.
        for key_name in keys:
            if _test_key(cmac, icb, data, keys[key_name]["auth"]):
                # Matching key found, add it to list.
                matching_keys.append(key_name)

        if len(matching_keys) == 0:
            raise ValueError("authentication failed")
        elif not quiet:
            print("Authentication succeeded with the following "
                  "keys: %s" % " ".join(matching_keys))

    return matching_keys


def decrypt(cipher_key, icb, files, prefix):
    '''Decrypt file data.'''

    # AES.new() only accepts bytes, not bytearray.
    cipher_key = bytes(cipher_key)

    # Create a fast counter for AES cipher.
    icb0, icb1, icb2, icb3 = struct.unpack("<4L", icb)
    ctr = Counter.new(128, little_endian = True,
                      allow_wraparound = True,
                      initial_value = (icb3 << 96) | (icb2 << 64) |
                                      (icb1 << 32) | icb0)

    # Create an AES Counter (CTR) mode cipher.
    cipher = AES.new(cipher_key, AES.MODE_CTR, counter = ctr)

    # Decrypt, decompress and write scratchpad file data to files.
    for file_num in range(len(files)):
        # Get scratchpad file data.
        data = files[file_num]["data"]

        # Decrypt data.
        data = cipher.decrypt(data)

        # Decompress data. Cannot use zlib.decompress(),
        # because the Adler-32 checksum is missing.
        dec_obj = zlib.decompressobj()

        # Feed zlib header first to decompressor.
        dec_obj.decompress(b"\x78\x9c")

        # Feed compressed data to decompressor, except the last 15 bytes.
        tail_in = data[-15:]
        data = dec_obj.decompress(data[:-15])

        # Since the Adler-32 checksum is missing and we don't know how
        # many bytes of padding there are (between 0 and 15 bytes), the
        # last bytes need to be fed to the decompressor one by one.
        tail_out = bytearray()
        for b in tail_in:
            try:
                compr_bytes = bytearray(dec_obj.unconsumed_tail)
                compr_bytes.append(b)
                # dec_obj.decompress() only accepts bytes, not bytearray.
                tail_out.extend(dec_obj.decompress(bytes(compr_bytes)))
            except zlib.error:
                # Adler-32 check failed, so all compressed data
                # must have been processed by now.
                break
        data += tail_out

        # Write data to a file.
        with open("%s%04d" % (prefix, file_num), "wb") as file:
            file.write(data)


def main():
    '''Main program'''

    # Determine program name, for error messages.
    pgmname = os.path.split(sys.argv[0])[-1]

    # Create a parser for parsing the command line and printing error messages.
    parser = create_argument_parser(pgmname)

    try:
        # Parse command line arguments.
        args = parser.parse_args()

        if args.configfile:
            # Load keys from configuration file.
            config = read_config(args.configfile)

            if args.keyname != None and args.keyname not in config["keys"]:
                raise ValueError("key not found in configuration file: %s" %
                                 args.keyname)
        elif args.keyname:
            raise ValueError("key given without configuration file")
        else:
            # No configuration file, so no keys.
            config = {"keys": {}}
    except (ValueError, IOError, OSError) as exc:
        sys.stdout.write("%s: %s\n" % (pgmname, exc))
        return 1

    try:
        filename = args.infile

        with open(filename, "rb") as infile:
            data = infile.read(SCRATCHPAD_MAX_LENGTH + 1)

        scratchpad_header = parse_header(data, filename)

        data = scratchpad_header["data"]

        if not args.quiet:
            print_header_info(scratchpad_header)

        secure_header = parse_secure(data, filename)

        if not args.quiet:
            print_secure_info(secure_header)

        files = parse_files(data[secure_header["length"]:], filename)

        if not args.quiet:
            print_file_info(files)

        auth_result = authenticate(secure_header["cmac"],
                                   secure_header["icb"],
                                   data[secure_header["length"]:],
                                   config["keys"], args.quiet, args.keyname)

        if args.dump and len(auth_result) > 0:
            decrypt(config["keys"][auth_result[0]]["encrypt"],
                    secure_header["icb"], files, args.dump)

        if not args.quiet:
            print("")
    except (ValueError, IOError) as exc:
        sys.stdout.write("%s: %s\n" % (pgmname, exc))
        return 1

    return 0


# Run main.
if __name__ == "__main__":
    sys.exit(main())
