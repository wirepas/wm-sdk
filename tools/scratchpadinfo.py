#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# scratchpadinfo.py - A tool to examine scratchpad files
#
# Requires:
#   - Python 2 v2.7 or newer (uses argparse)
#     or Python 3 v3.2 or newer
#   - PyCryptodome v3.0 or newer
#   - bootloader_config.py in the same directory as this file

import sys
import os
import re
import zlib
import struct
import argparse
import textwrap

from Crypto.Hash import CMAC, SHA256
from Crypto.Cipher import AES
from Crypto.PublicKey import ECC
from Crypto.Signature import DSS
from Crypto.Util import Counter

from bootloader_config import BootloaderConfig, KeyDesc


# Constants

# Magic 16-byte string for locating a combi scratchpad in Flash
SCRATCHPAD_V1_TAG = b"SCR1\232\223\060\202\331\353\012\374\061\041\343\067"

# Minimum number of bytes in a valid combi scratchpad file
SCRATCHPAD_MIN_LENGTH = 48

# Maximum number of bytes in a combi scratchpad file
SCRATCHPAD_MAX_LENGTH = (16 * 1024 * 1024)  # 16 megabytes

# Size of scratchpad tag and header in bytes
SCRATCHPAD_TAG_HEADER_LENGTH = 32

# Size of OMAC1 / CMAC tag and secure header / Initial Counter Block in bytes
SCRATCHPAD_CMAC_ICB_LENGTH = 32

# Size of file header
SCRATCHPAD_FILE_HEADER_LENGTH = 16

# Area ID of public key signature
PKSIG_AREA_ID = 0x027C362F  # “PKSIG” in DEC Radix-50 encoding

# Signature file format version: 1.0.0.0
PKSIG_FILE_FORMAT_VERSION = (1, 0, 0, 0)

# Size of ECDSA P-256 signature, in bytes
ECDSA_P256_SIGNATURE_SIZE = 64  # R and S in big-endian order


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


def bytes_to_hex(str):
    return " ".join(["%02X" % n for n in bytearray(str)])


def ini_file(string_file):
    if not string_file.endswith(".ini"):
        raise argparse.ArgumentTypeError("invalid ini file extension '%s'" %
                                         string_file)
    return string_file


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
        type = ini_file, action = "append", metavar = "FILE", required = False,
        help = "a list of configuration files with keys (not set by default)")
    parser.add_argument("--keyname", "-k",
        metavar = "NAME",
        help = "name of key to try for authentication (try all by default)")
    parser.add_argument("--dump", "-d",
        metavar = "PREFIX",
        help = "write decrypted, decompressed data to files "
               "with a given filename prefix")
    parser.add_argument("--quiet", "-q",
        action = "store_true",
        help = "reduce verbosity")
    parser.add_argument("infile",
        metavar = "INFILE",
        help = "compressed, encrypted scratchpad file to examine")

    return parser


def parse_header(data, filename):
    '''Parse and verify scratchpad header'''

    scratchpad_tag_length = len(SCRATCHPAD_V1_TAG)

    # Check minimum length.
    if len(data) < SCRATCHPAD_MIN_LENGTH:
        raise ValueError("file too short: %s" % filename)

    # Check maximum length.
    if len(data) > SCRATCHPAD_MAX_LENGTH:
        raise ValueError("file too long: %s" % filename)

    # Check combi scratchpad tag.
    if data[:scratchpad_tag_length] != SCRATCHPAD_V1_TAG:
        raise ValueError("no valid scratchpad tag found: %s" % filename)

    # Unpack scratchpad header fields.
    length, crc, otapseq, pad, bltype, blstatus = struct.unpack(
        "<LH2B2L", data[scratchpad_tag_length:SCRATCHPAD_TAG_HEADER_LENGTH])

    # Verify data length.
    length_indata = len(data) - SCRATCHPAD_TAG_HEADER_LENGTH
    if length != length_indata:
        raise ValueError("header data length mismatch (%d vs. %d bytes): %s" %
                         (length, length_indata, filename))

    # Verify CRC.
    crc_indata = crc16_ccitt(data[SCRATCHPAD_TAG_HEADER_LENGTH:])
    if crc != crc_indata:
        raise ValueError("header CRC mismatch (0x%04x vs. 0x%04x): %s" %
                         (crc, crc_indata, filename))

    header = {}
    header["length"] = length
    header["crc"] = crc
    header["otapseq"] = otapseq
    header["pad"] = pad
    header["bltype"] = bltype
    header["blstatus"] = blstatus

    return header, SCRATCHPAD_TAG_HEADER_LENGTH


def print_header_info(header):
    '''Print scratchpad header information'''

    scratchpad_tag_length = len(SCRATCHPAD_V1_TAG)

    print("")
    print("Type: Combi scratchpad")
    print("Scratchpad tag length: %d" % scratchpad_tag_length)
    print("Scratchpad header length: %d" %
          (SCRATCHPAD_TAG_HEADER_LENGTH - scratchpad_tag_length))
    print("Compressed data length: %d" % header["length"])
    print("Compressed data CRC: 0x%04x" % header["crc"])
    print("OTAP sequence number: %d" % header["otapseq"])
    print("Pad field: 0x%02x" % header["pad"])
    print("Type field: 0x%08x" % header["bltype"])
    print("Status field: 0x%08x" % header["blstatus"])
    print("")


def parse_secure(data, offset, filename):
    '''Parse security-related headers'''

    # Check minimum length.
    if (offset + SCRATCHPAD_CMAC_ICB_LENGTH) > len(data):
        raise ValueError("file too short: %s" % filename)

    # Unpack security-related header fields.
    cmac, icb = struct.unpack(
        "<16s16s", data[offset:(offset + SCRATCHPAD_CMAC_ICB_LENGTH)])

    secure = {}
    secure["cmac"] = cmac
    secure["icb"] = icb

    return secure, SCRATCHPAD_CMAC_ICB_LENGTH


def print_secure_info(header):
    '''Print security-related header information'''

    print("Security header length: %d" % SCRATCHPAD_CMAC_ICB_LENGTH)
    print("CMAC / OMAC1: %s" % bytes_to_hex(header["cmac"]))
    print("Initial Counter Block: %s" % bytes_to_hex(header["icb"]))
    print("")


def parse_files(data, offset, filename):
    '''Parse file headers'''

    if (offset + SCRATCHPAD_FILE_HEADER_LENGTH) > len(data):
        raise ValueError("file too short: %s" % filename)

    pos = offset
    indata_length = len(data)
    files = []

    while pos < indata_length:
        if (pos + SCRATCHPAD_FILE_HEADER_LENGTH) > indata_length:
            raise ValueError("extra data after files: %s" % filename)

        (areaid, length, ver_major, ver_minor,
         ver_maint, ver_devel, pad) = struct.unpack(
            "<2L4BL", data[pos:(pos + SCRATCHPAD_FILE_HEADER_LENGTH)])

        f = {}
        f["areaid"] = areaid
        f["length"] = length
        f["ver_major"] = ver_major
        f["ver_minor"] = ver_minor
        f["ver_maint"] = ver_maint
        f["ver_devel"] = ver_devel
        f["pad"] = pad

        # Skip over header.
        pos += SCRATCHPAD_FILE_HEADER_LENGTH

        if pos + length > indata_length:
            raise ValueError("not enough data for file: %s" % filename)

        f["data"] = data[pos:(pos + length)]

        files.append(f)

        # Skip over compressed data.
        pos += length

    return files, pos


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


def parse_signature_file(files, file_offset):
    '''Parse signature file'''

    signature = None
    pksig_offset = file_offset

    # Signature file TLV prefix and total number of bytes
    tlv_prefix = b"\x02\x05ES256\x04\tAES128CTR\x06@"
    signature_file_length = 96

    if len(files) == 0:
        # No files, leave.
        pass
    elif files[0]["areaid"] != PKSIG_AREA_ID:
        # First file is not a signature file, leave.
        pass
    else:
        # Parse first file (signature file) data.
        data = files[0]["data"]

        # Compare to a known prefix.
        # TODO: Parse TLV structure.
        if (len(data) != signature_file_length or
            not data.startswith(tlv_prefix)):
            raise ValueError("could not parse signature file")

        # Extract signature.
        offset = len(tlv_prefix)
        signature = data[offset:(offset + ECDSA_P256_SIGNATURE_SIZE)]
        pksig_offset += SCRATCHPAD_FILE_HEADER_LENGTH + files[0]["length"]

    return signature, pksig_offset


def authenticate(cmac, icb, signature, data, cmac_offset, pksig_offset,
                 config, quiet, key_name = None):
    '''Authenticate file data.'''

    if config is None or len(config.keys) == 0:
        if not quiet:
            print("No keys, data not authenticated")
        return []

    def _test_key_cmac(cmac, icb, data, offset, key):
        '''Try to authenticate ICB, file headers and file data with a key.'''

        # CMAC.new() only accepts bytes, not bytearray.
        auth_key = bytes(key.authentication)

        # Create a CMAC / OMAC1 object.
        cobj = CMAC.new(auth_key, ciphermod = AES)

        # Calculate CMAC / OMAC1 over ICB, file headers and file data.
        cobj.update(icb)
        cobj.update(data[offset:])

        # Succeeded?
        return cmac == cobj.digest()

    def _test_key_pksig(signature, data, offset, key):
        '''Try to authenticate file data with a signature and a key.'''

        if not signature:
            # No signature found, leave.
            return False

        # Read ECDSA P-256 private key.
        auth_key = ECC.import_key(key.authentication)
        if auth_key.curve not in ("NIST P-256", "p256", "P-256",
                                  "prime256v1", "secp256r1"):
            raise ValueError("unsupported ECC curve: '%s'" % auth_key.curve)

        # Calculate SHA-256 hash over file headers and data.
        hobj = SHA256.new(data[offset:])
        verifier = DSS.new(auth_key, mode = "fips-186-3", encoding = "binary")

        # Verify signature.
        try:
            verifier.verify(hobj, signature)
            return True  # Succeeded.
        except ValueError:
            pass  # Failed.

        return False

    def _test_key(cmac, icb, signature, data, cmac_offset, pksig_offset, key):
        if key.key_type == KeyDesc.KEY_TYPE_OMAC1_AES128CTR:
            success = _test_key_cmac(cmac, icb, data, cmac_offset, key)
        elif key.key_type == KeyDesc.KEY_TYPE_SHA256_ECDSA_P256_AES128CTR:
            success = _test_key_pksig(signature, data, pksig_offset, key)
        else:
            raise ValueError("unsupported key type for key: '%s'" % key_name)
        return success

    matching_keys = []

    if key_name:
        # Try a named key only.
        keys_to_try = {key_name: config.keys[key_name]}
    else:
        # Try all keys.
        keys_to_try = config.keys

    for try_name in keys_to_try:
        if _test_key(cmac, icb, signature, data,
                     cmac_offset, pksig_offset, keys_to_try[try_name]):
            # Matching key found, add it to list.
            matching_keys.append(try_name)

    # Return a list of keys that worked.
    if key_name:
        if len(matching_keys) == 0:
            raise ValueError("failed authentication with key: %s" % key_name)
        elif not quiet:
            print("Authentication with key succeeded: %s" % key_name)
    else:
        if len(matching_keys) == 0:
            raise ValueError("authentication failed")
        elif not quiet:
            print("Authentication succeeded with the following "
                  "keys: %s" % " ".join(matching_keys))

    if not quiet:
        print("")

    return matching_keys


def decrypt(config, key_name, icb, files, prefix, quiet):
    '''Decrypt file data.'''

    if  config is None or len(config.keys) == 0 or key_name is None:
        if not quiet:
            print("No keys, data not decrypted")
        return

    # AES.new() only accepts bytes, not bytearray.
    cipher_key = bytes(config.keys[key_name].encryption)

    # Create a fast counter for AES cipher.
    icb0, icb1, icb2, icb3 = struct.unpack("<4L", icb)
    ctr = Counter.new(128, little_endian = True,
                      allow_wraparound = True,
                      initial_value = (icb3 << 96) | (icb2 << 64) |
                                      (icb1 << 32) | icb0)

    # Create an AES Counter (CTR) mode cipher.
    cipher = AES.new(cipher_key, AES.MODE_CTR, counter = ctr)

    if not quiet:
        print("Writing files:", end = "")

    # Decrypt, decompress and write scratchpad file data to files.
    for file_num in range(len(files)):
        # Get scratchpad file data.
        data = files[file_num]["data"]

        if files[file_num]["areaid"] == PKSIG_AREA_ID:
            # Signature file, only update cipher counter.
            cipher.decrypt(data)
        else:
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
        filename = "%s%04d" % (prefix, file_num)
        with open(filename, "wb") as file:
            file.write(data)

        if not quiet:
            print(" %s" % filename, end = "")

    if not quiet:
        print("\n")  # Two new lines


def main():
    '''Main program'''

    # Determine program name, for error messages.
    pgmname = os.path.split(sys.argv[0])[-1]

    # Create a parser for parsing the command line and printing error messages.
    parser = create_argument_parser(pgmname)

    config = None

    try:
        # Parse command line arguments.
        args = parser.parse_args()

        if args.configfile:
            # Parse configuration file. Only the keys are of interest.
            config = BootloaderConfig.from_ini_files(args.configfile)

            if args.keyname and args.keyname not in config.keys:
                raise ValueError("key not found in configuration file: %s" %
                                 args.keyname)
        elif args.keyname:
            raise ValueError("key given without configuration file")
    except (ValueError, IOError, OSError) as exc:
        sys.stdout.write("%s: %s\n" % (pgmname, exc))
        return 1

    try:
        filename = args.infile

        with open(filename, "rb") as infile:
            data = infile.read(SCRATCHPAD_MAX_LENGTH + 1)

        offset = 0

        # OPTIMIZE: Use memoryview().
        scratchpad_header, skip = parse_header(data, filename)
        offset += skip

        if not args.quiet:
            print_header_info(scratchpad_header)

        secure_header, skip = parse_secure(data, offset, filename)
        offset += skip

        if not args.quiet:
            print_secure_info(secure_header)

        file_offset = offset
        files, skip = parse_files(data, file_offset, filename)
        offset += skip

        if not args.quiet:
            print_file_info(files)

        signature, pksig_offset = parse_signature_file(files, file_offset)

        # Try to authenticate scratchpad.
        auth_result = authenticate(secure_header["cmac"],
                                   secure_header["icb"],
                                   signature,
                                   data, file_offset, pksig_offset,
                                   config, args.quiet, args.keyname)

        if args.dump:
            # Decrypt and dump files. Use first matching key to decrypt.
            key_name = (len(auth_result) > 0) and auth_result[0] or None
            decrypt(config, key_name, secure_header["icb"],
                    files, args.dump, args.quiet)
    except (ValueError, IOError) as exc:
        sys.stdout.write("%s: %s\n" % (pgmname, exc))
        return 1

    return 0


# Run main.
if __name__ == "__main__":
    sys.exit(main())
