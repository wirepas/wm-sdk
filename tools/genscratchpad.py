#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# genscratchpad.py - A tool to generate encrypted scratchpad contents
#
# Requires:
#   - Python 2 v2.7 or newer (uses argparse, hextool.py)
#     or Python 3 v3.2 or newer
#   - PyCryptodome v3.0 or newer
#   - hextool.py in the same directory as this file

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

# Import hextool.py, but do not write a .pyc file for it.
dont_write_bytecode = sys.dont_write_bytecode
sys.dont_write_bytecode = True
import hextool
sys.dont_write_bytecode = dont_write_bytecode
del dont_write_bytecode

from bootloader_config import BootloaderConfig


# Python 2 and Python 3 support

try:
    # Python 3
    import configparser
    config_parser_tweaks = {"comment_prefixes": ("#", ";"),
                            "inline_comment_prefixes": (";", )}
except ImportError:
    # Python 2
    import ConfigParser as configparser
    config_parser_tweaks = {}

try:
    # Python 2
    xrange
except NameError:
    # Python 3
    xrange = range


# Constants

# Set to True to enable tests. Enable only one. See utils/aes.c for details.
# Results are written to the output file.
AES_TEST = False
CMAC_TEST = False

# Offset from start of area for \ref bl_info_header_t, in bytes.
BL_INFO_HEADER_OFFSET = 16

# Data block length, in bytes
BLOCK_LENGTH = 16

# Magic 16-byte string for locating a combi scratchpad in Flash
SCRATCHPAD_V1_TAG = b"SCR1\232\223\060\202\331\353\012\374\061\041\343\067"

# Maximum number of bytes in an input file
MAX_NUM_BYTES_PER_FILE = (8 * 1024 * 1024)  # Eight megabytes

# Input data for testing AES and CMAC
test_data = bytearray([
    0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
    0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
    0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
    0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
    0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11
    ])

# Initial Counter Block for AES CTR test
test_icb = bytearray([
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
    ])


# Classes

class InFile(object):
    '''An input file'''

    def __init__(self, file_spec = None, data = None,
                 version = (0, 0, 0, 0)):
        if file_spec == None and data == None:
            raise ValueError("no data given")
        elif file_spec != None and data != None:
            raise ValueError("file_spec and data are mutually exclusive")

        # Set default values.
        self.area_id = 0x00000000
        self.version = version
        self.compressed = False
        self.encrypted = False

        if data:
            # Data given, make a copy of it.
            self.data = bytearray(data)
            self.raw_data = bytearray(data)
        else:
            # File specification given, parse it.
            version, self.area_id, filename = self.parse_file_spec(file_spec)
            self.version = parse_version(version, self.version[3])

            # Read data from file.
            memory = hextool.Memory()
            hextool.load_intel_hex(memory, filename = filename)

            if memory.num_ranges == 0:
                # No data found in file.
                raise ValueError("file contains no data: '%s'" % filename)
            elif (memory.max_address -
                  memory.min_address > MAX_NUM_BYTES_PER_FILE):
                raise ValueError("file too big: '%s'" % filename)

            # Convert Memory object to a flat bytearray.
            self.data = memory[memory.min_address:memory.max_address]
            self.raw_data = memory[memory.min_address:memory.max_address]

        # Create a file header for uncompressed, unencrypted data.
        self.header = make_file_header(self.area_id, len(self.data),
                                       *self.version)

    def compress(self):
        if self.compressed:
            raise ValueError("data already compressed")

        # OPTIMIZE: Avoid extra conversions between bytearrays and bytes.
        # Compress data. zlib.compress() does not accept
        # bytearrays, so convert bytearray to bytes.
        compressed_data = bytearray(zlib.compress(bytes(self.data), 9))

        # Determine compressed data length without
        # zlib header and Adler-32 checksum.
        comp_data_len = len(compressed_data) - 2 - 4

        # Pad size to a multiple of block length.
        num_blocks = (comp_data_len + BLOCK_LENGTH - 1) // BLOCK_LENGTH

        # Mark data as compressed.
        self.compressed = True

        # Create a bytearray object for compressed data.
        self.data = bytearray(num_blocks * BLOCK_LENGTH)

        # Copy compressed data to bytearray, but leave
        # out zlib header and Adler-32 checksum.
        self.data[:comp_data_len] = compressed_data[2:-4]

        # Update the file header now that data length has changed.
        self.header = make_file_header(self.area_id, len(self.data),
                                       *self.version)

    def encrypt(self, cipher):
        if self.encrypted:
            raise ValueError("data already encrypted")

        # OPTIMIZE: Avoid extra conversions between bytearrays and bytes.
        # Cipher does not accept bytearrays, so convert to bytes.
        self.data = bytearray(cipher.encrypt(bytes(self.data)))

        # Mark data as encrypted.
        self.encrypted = True

    def parse_file_spec(self, file_spec):
        '''
        Parse input file specification.

        file_spec   version:area_id:filename or file.conf:area_id:filename
        '''

        try:
            # Read version, memory area ID and file name.
            version, area_id, filename = file_spec.split(":", 2)

            if version.endswith(".conf"):
                # conf file provided, version must be read inside
                # version is a file name
                cp = configparser.ConfigParser()
                cp.read(version)

                extracted_version = None

                for section in cp.sections():
                    for option in cp.options(section):
                        if option == "version":
                            extracted_version = cp.get(section, option)
                if extracted_version is None:
                    raise ValueError("invalid configuration file format, cannot find version:"
                                     "'%s'" % version)

                # Override version
                version = extracted_version

            area_id = int(area_id, 0)
            if area_id < 0 or area_id > 4294967295:
                raise ValueError
        except ValueError:
            raise ValueError("invalid input file specification: "
                             "'%s'" % file_spec)

        return version, area_id, filename


# Functions

def crc16_ccitt(data, initial = 0xffff):
    '''Simple and slow version of CRC16-CCITT'''

    # OPTIMIZE: Avoid extra conversions between bytearrays and bytes.
    crc = initial
    data = bytearray(data)

    for b in data:
        crc = (crc >> 8) | ((crc & 0xff) << 8)
        crc ^= b
        crc ^= (crc & 0xf0) >> 4
        crc ^= (crc & 0x0f) << 12
        crc ^= (crc & 0xff) << 5

    return crc


def create_cipher(icb, cipher_key):
    '''
    Create an AES-128 Counter (CTR) mode cipher
    with 16-byte initial counter block.
    '''

    # AES.new() only accepts bytes, not bytearray.
    cipher_key = bytes(cipher_key)

    # Create a fast counter for AES cipher.
    icb0, icb1, icb2, icb3 = struct.unpack("<4L", icb)
    ctr = Counter.new(128, little_endian = True,
                      allow_wraparound = True,
                      initial_value = (icb3 << 96) | (icb2 << 64) |
                                      (icb1 << 32) | icb0)

    # Create an AES Counter (CTR) mode cipher.
    return AES.new(cipher_key, AES.MODE_CTR, counter = ctr)


def calculate_cmac(data_list, auth_key):
    '''Calculate CMAC / OMAC1 tag for data.'''

    # CMAC.new() only accepts bytes, not bytearray.
    auth_key = bytes(auth_key)

    # Create a CMAC / OMAC1 object.
    cobj = CMAC.new(auth_key, ciphermod = AES)

    for data in data_list:
        # OPTIMIZE: Avoid extra conversions between bytearrays and bytes.
        # CMAC object does not accept bytearrays, so convert to bytes.
         cobj.update(bytes(data))

    # Calculate digest and return it as bytearray.
    return bytearray(cobj.digest())


def parse_version(version, default_devel):
    '''Parse a version number of form MINOR.MAJOR.MAINTENANCE[.DEVELOPMENT].'''

    try:
        # Split to components.
        ver_components = version.strip().split(".")

        if len(ver_components) < 3 or len(ver_components) > 4:
            # Wrong number of components.
            raise ValueError
        elif len(ver_components) == 3:
            # No development number, use default.
            ver_components.append(default_devel)

        # Convert to integers.
        ver_components = [int(n) for n in ver_components]
    except ValueError:
        raise ValueError("invalid firmware version: '%s'" % version)

    return tuple(ver_components)


def ini_file(string_file):
    if not string_file.endswith(".ini"):
        raise argparse.ArgumentTypeError("invalid ini file extension %s" % string_file)
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
        description = textwrap.fill(
            "A tool to generate compressed and "
            "encrypted scratchpad contents", help_width),
        epilog =
            "values:\n"
            "  infilespec    [version|file.conf]:area_id:filename\n"
            "  area_id       0x00000000 .. 0xffffffff\n"
            "  version       major.minor.maintenance[.developer], "
            "each 0 .. 255,\n"
            "                developer defaults to "
            "OTAP sequence number if not given,\n"
            "  file.conf     a configuration file containing metadatas"
            "associated to the filename")
    parser.add_argument("--configfile", "-c",
        type=ini_file,
        action='append',
        metavar = "FILE",
        help = "a configuration file with keys and memory information "
               "(default: \"%(default)s\")")
    parser.add_argument("--bootloader", "-b",
        metavar = "FILE",
        help = "a bootloader binary file")
    parser.add_argument("--genprog", "-p",
        metavar = "FILE",
        help = "produce a programming output file in addition to "
               "scratchpad, in Intel HEX format")
    parser.add_argument("--keyname", "-k",
        metavar = "NAME", default = "default",
        help = "name of key to use for encryption and authentication "
               "(default: \"%(default)s\")")
    parser.add_argument("--otapseq", "-o", type = int, default = 1,
        metavar = "NUM",
        help = "OTAP sequence number of output file "
               "(0 to 255, default: %(default)s)")
    parser.add_argument("--type", "-t", metavar = "TYPE", type = str,
        default = "combi",
        help = "type of scratchpad to generate: combi "
               "(default: \"%(default)s\")")
    parser.add_argument("--set", "-s", action = "append", type = str,
        metavar = "SYMBOL=VALUE", default = [], dest = "symbols",
        help = "introduce a symbolic value")
    parser.add_argument("outfile", metavar = "OUTFILE",
        help = "compressed, encrypted output scratchpad file")
    parser.add_argument("infilespec", nargs = "+", metavar = "INFILESPEC",
        help = "input file(s) in Intel HEX format to "
               "be added in scratchpad, see below")

    return parser


def validate_symbol_name(name):
    '''Validate a symbol name:

    letters, digits or underscore, except no digits in the first position
    '''

    first = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz"
    rest = "0123456789" + first

    if len(name) == 0:
        return False
    elif name[0] not in first:
        return False

    for c in name[1:]:
        if c not in rest:
            return False

    return True


def parse_arguments(parser):
    '''Parse command line arguments.'''

    args = parser.parse_args()

    if args.genprog and not args.bootloader:
        raise ValueError("cannot generate programming output file "
                         "without bootloader")

    sptype = args.type.lower()

    if sptype != "combi":
        raise ValueError("invalid scratchpad type: '%s'" % args.type)

    # Convert a list of symbol definitions to a dictionary.
    symbols = {}
    for sdef in args.symbols:
        sname, svalue = sdef.split("=", 1)

        if not validate_symbol_name(sname):
            raise ValueError("invalid symbol name: '%s'" % sname)

        symbols[sname] = svalue

    args.symbols = symbols

    return args


def make_file_header(areaid, length,
                     ver_major, ver_minor, ver_maint, ver_devel):
    '''
    From bootloader.h:

    struct
    {
        /** Memory area ID of this item */
        uint32_t id;
        /** Number of compressed bytes following this header */
        uint32_t length;
        /** Firmware major version number component */
        uint8_t major;
        /** Firmware minor version number component */
        uint8_t minor;
        /** Firmware maintenance version number component */
        uint8_t maint;
        /** Firmware development version number component */
        uint8_t devel;
        /** Padding, reserved for future use, must be 0 */
        uint32_t pad;
    };
    '''

    if areaid < 0 or areaid > 4294967295:
        raise ValueError("memory area ID not 0 .. 4294967295")

    pad = 0x00000000

    return struct.pack("<2L4BL", areaid, length,
                       ver_major, ver_minor, ver_maint, ver_devel, pad)


def make_scratchpad_header(otap_seq, scratchpad_data):
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
        /** Status code from bootloader: bl_header_status_e */
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

    if otap_seq < 0 or otap_seq > 255:
        raise ValueError("sequence number not 0 .. 255")

    # Calculate length of scratchpad contents.
    length = sum(map(len, scratchpad_data))

    if length % 16 != 0:
        raise ValueError("data length not multiple of 16")

    # Calculate CRC of scratchpad contents.
    crc = 0xffff
    for data in scratchpad_data:
        crc = crc16_ccitt(data, crc)

    pad = 0x00

    bltype = 0x00000000     # BL_HEADER_TYPE_FIRMWARE
    blstatus = 0xffffffff   # BL_HEADER_STATUS_NEW

    return struct.pack("<LH2B2L", length, crc, otap_seq, pad, bltype, blstatus)

def make_bl_info_header(config, scratchpad_data, in_file):
    '''
    From bootloader.h:

    typedef struct
    {
        /** Number of bytes of scratchpad, not including header and tag */
        uint32_t length;
        /** CRC16-CCITT of scratchpad, not including any header or tag bytes */
        uint16_t crc;
        /** Sequence number of data in scratchpad: \ref BL_SCRATCHPAD_MIN_SEQ .. \ref BL_SCRATCHPAD_MAX_SEQ */
        bl_scratchpad_seq_t seq;
        /** Flags are used by the stack to determine if the scratchpad must be
         *  taken into use if the connection to the sink is lost for more than
         *  one hour (MANUAL or AUTO_PROCESS). These flags are don't care for
         *  the bootloader and are not defined here.
         */
        uint8_t flags;
        /** Memory area ID of firmware */
        uint32_t id;
        /** Firmware major version number component */
        uint8_t major;
        /** Firmware minor version number component */
        uint8_t minor;
        /** Firmware maintenance version number component */
        uint8_t maint;
        /** Firmware development version number component */
        uint8_t devel;
        /** Size of the data written in the area after decrompression */
        uint32_t written_size;
    };
    '''
    # Calculate length of scratchpad contents.
    scr_length = sum(map(len, scratchpad_data))

    if scr_length % 16 != 0:
        raise ValueError("data length not multiple of 16")

    # Calculate CRC of scratchpad contents.
    crc = 0xffff
    for data in scratchpad_data:
        crc = crc16_ccitt(data, crc)

    # Sequence number can't be known in advance, set to 0xFF
    seq = 0xFF

    # Not used, set to 0x00
    flags = 0x00

    if in_file.area_id < 0 or in_file.area_id > 4294967295:
        raise ValueError("memory area ID not 0 .. 4294967295")

    # written_size in memory area is rounded up to erase block boundary.
    written_size = \
        (int(len(in_file.raw_data) / config.flash.eraseblock) + 1) * \
        config.flash.eraseblock

    return struct.pack("<LH2BL4BL", scr_length, crc, seq, flags,
                       in_file.area_id, in_file.version[0], in_file.version[1],
                       in_file.version[2], in_file.version[3], written_size)

def gen_bootloader(config, bootloader):
    '''Create the bootloader image with its configuration (areas and keys).'''

    bl_area = config.get_bootloader_area()

    bl_start = bl_area.address
    bl_max_num_bytes = bl_area.length
    bl_end = bl_start + bl_max_num_bytes
    set_start = bl_area.get_settings_start_address()
    set_key_start = bl_area.get_key_start_address()
    set_end = bl_area.get_settings_end_address()

    memory = hextool.Memory()

    if bootloader.min_address < bl_start : # DEBUG: or bootloader.max_address > bl_end:
        raise ValueError("bootloader overflows outside its area: "
                         "0x%04x .. 0x%04x, should be 0x%04x .. 0x%04x" %
                         (bootloader.min_address, bootloader.max_address,
                          bl_start, bl_end))

    # Add bootloader to the Flash memory image.
    memory.cursor = bl_start
    memory += bootloader

    # Erase keys and memory area information Flash memory image.
    del memory[set_start:set_end]

    # Set Memory cursor to point at the bootloader area settings.
    memory.cursor = set_start

    # Add memory area information in Flash memory image.
    for area in config.areas.values():
        data = struct.pack("<4L", area.address, area.length,
                           area.id, area.flags | (area.type << 2))
        memory += data

    # Sanity check.
    if memory.cursor > set_key_start:
        raise ValueError("error generating programming image: bootloader areas")

    # Set Memory cursor to point at the bootloader key settings.
    memory.cursor = set_key_start

    # Add keys in Flash memory image.
    for key in config.keys.values():
        memory += key.authentication
        memory += key.encryption

    # Sanity check.
    if memory.cursor > set_end:
        raise ValueError("error generating programming image: bootloader keys")

    return memory

def gen_prog_image(config, bootloader, scratchpad_header, scratchpad_data):
    '''Create a Flash memory image, ready for programming to a device.'''

    sp_area = config.get_scratchpad_area()
    sp_max_num_bytes = sp_area.length
    # scratchpad is written by the end
    sp_end = sp_area.address + sp_area.length

    # Create a Memory object to store the Flash memory image.
    memory = hextool.Memory()

    # Calculate total scratchpad length in bytes.
    sp_num_bytes = sum(map(len, scratchpad_data))

    if sp_num_bytes > sp_max_num_bytes:
        raise ValueError("scratchpad too big by %d bytes" % (sp_num_bytes -
                                                             sp_max_num_bytes))

    # Add bootloader to memory image
    memory += gen_bootloader(config, bootloader)

    # Set Memory cursor to point at the first byte
    # of scratchpad in the Flash memory image.
    memory.cursor = (sp_end - len(SCRATCHPAD_V1_TAG) -
                     len(scratchpad_header) - sp_num_bytes)

    # Add scratchpad to the Flash memory image.
    for data in scratchpad_data:
        memory += data

    # Add a scratchpad header at the end, followed by a 16-byte tag
    # for finding the scratchpad in Flash, to the Flash memory image.
    memory += scratchpad_header
    memory += SCRATCHPAD_V1_TAG

    # Sanity check.
    if memory.cursor != sp_end:
        raise ValueError("error generating programming image: scratchpad")

    return memory

def gen_prog_image_without_scratchpad(config,
                                      bootloader,
                                      scratchpad_data,
                                      in_files):
    '''Create a Flash memory image, ready for programming to a device.'''

    # Create a Memory object to store the Flash memory image.
    memory = hextool.Memory()

    # Add bootloader to memory image
    memory += gen_bootloader(config, bootloader)

    # Add other files to memory image.
    for in_file in in_files:
        for area in config.areas.values():
            # Add file only if bootloader knows this area.
            if area.id == in_file.area_id:
                memory.cursor = area.address

                # Sanity check.
                if area.length < len(in_file.raw_data):
                    raise ValueError("error generating programming image: \
                        file too big for area.")

                memory += in_file.raw_data

                # If flag is set need to add bl_info_header.
                if area.has_bl_info_header():
                    header = make_bl_info_header(config,
                                                 scratchpad_data,
                                                 in_file)
                    memory.cursor = area.address + BL_INFO_HEADER_OFFSET
                    memory.overlap_ok = True
                    memory += header
                    memory.overlap_ok = False

    return memory

def lookup_symbol(value, symbols):
    '''Get a symbolic value or use value as is'''
    return symbols.get(value, value)


def lookup_int(value, symbols):
    '''Get a symbolic or numeric value as integer'''

    value = lookup_symbol(value, symbols)

    try:
        value = int(value, 0)
    except ValueError:
        raise ValueError("invalid value: '%s'" % value)

    return value


def get_string(value):
    '''Parse a string value and remove enclosing quotes'''
    try:
        if len(value) == 0:
            return ""
        elif len(value) < 2:
            raise ValueError
        elif value[0] != '"' or value[-1] != '"':
            raise ValueError
        return value[1:-1]
    except ValueError:
        raise ValueError("invalid value: '%s'" % value)


def get_key(value):
    '''Parse an encryption or authentication key value

    Bytes are separated by commas or spaces.'''
    value = value.replace(",", " ")
    return bytearray.fromhex(value)

def main():
    '''Main program'''

    # Determine program name, for error messages.
    pgmname = os.path.split(sys.argv[0])[-1]

    # Create a parser for parsing the command line and printing error messages.
    parser = create_argument_parser(pgmname)

    try:
        # Parse command line arguments.
        args = parse_arguments(parser)

        # Parse configuration file.
        config = BootloaderConfig.from_ini_files(args.configfile, args.symbols)

        # Check that config is valid
        # ValueError exception will be raised in case of invalid config
        config.check_config()

        # Check the key chosen is declared
        try:
            chosenkey = config.keys[args.keyname]
        except KeyError:
            raise ValueError("key not found: '%s'" % args.keyname)

    except (ValueError, IOError, OSError, configparser.Error) as exc:
        sys.stdout.write("%s: %s\n" % (pgmname, exc))
        return 1

    if False:
        # DEBUG: Show config.
        for key in config.keys.items():
            print(key)

    # Create a Memory object for storing the bootloader.
    bootloader = hextool.Memory()

    if args.bootloader:
        # Read bootloader.
        hextool.load_intel_hex(bootloader, filename = args.bootloader)

    in_files = []           # List of InFile objects
                            # (header, area_id, version, data)
    scratchpad_data = []    # List of scratchpad data blocks
    ver_major, ver_minor, ver_maint, ver_devel = (0, 0, 0, 0)

    if AES_TEST:
        # Run AES test. See aes_test1() in utils/aes.c for details.
        in_files.append(InFile(data = test_data))
        scratchpad_data.append(test_icb)
    elif CMAC_TEST:
        # Run CMAC / OMAC1 test. See aes_omac1_test1()
        # in utils/aes.c for details.
        scratchpad_data.append(test_data)
    else:
        # Read input files.
        try:
            for file_spec in args.infilespec:
                # Create an InFile object.
                in_file = InFile(file_spec = file_spec,
                                 version = (0, 0, 0, args.otapseq))

                # Compress data in-place.
                in_file.compress()

                in_files.append(in_file)
        except (ValueError, IOError, OSError) as exc:
            sys.stdout.write("%s: %s\n" % (pgmname, exc))
            return 1

        # Create secure header, which is also the initial counter block (ICB).
        secure_header = get_random_bytes(16)
        scratchpad_data.append(secure_header)

    if not CMAC_TEST:
        try:
            # Create an AES Counter (CTR) mode cipher using secure
            # header as the 16-byte initial counter block (ICB).
            cipher = create_cipher(secure_header, chosenkey.encryption)

            # Encrypt each input file.
            for in_file in in_files:
                # Encrypt data in-place.
                in_file.encrypt(cipher)

                # Add file header to scratchpad data.
                scratchpad_data.append(in_file.header)

                # Add compressed, encrypted file data to scratchpad data.
                scratchpad_data.append(in_file.data)
        except ValueError as exc:
            sys.stdout.write("%s: %s\n" % (pgmname, exc))
            return 1

    # Calculate and add CMAC / OMAC1 tag.
    try:
        cmac = calculate_cmac(scratchpad_data, chosenkey.authentication)
        scratchpad_data.insert(0, cmac)
    except ValueError as exc:
        sys.stdout.write("%s: %s\n" % (pgmname, exc))
        return 1

    if not AES_TEST and not CMAC_TEST:
        # Create a scratchpad header.
        try:
            scratchpad_header = make_scratchpad_header(args.otapseq,
                                                       scratchpad_data)
        except ValueError as exc:
            sys.stdout.write("%s: %s\n" % (pgmname, exc))
            return 1
    else:
        scratchpad_header = bytes()

    # Write output file and optionally the programming image file.
    try:
        with open(args.outfile, "wb") as f:
            # A combi scratchpad file starts with a 16-byte tag.
            f.write(SCRATCHPAD_V1_TAG)

            # Combi scratchpad files have the scratchpad header in front of
            # the scratchpad contents. The firmware rearranges the data in
            # Flash memory while storing the scratchpad.
            f.write(scratchpad_header)

            # Write scratchpad contents.
            for data in scratchpad_data:
                f.write(data)

        if args.genprog == None:
            return 0

        # Combine bootloader, memory area specification, keys and scratchpad.
        file_without_scr = args.genprog.rsplit('.',1)[0] + \
                           "_without_scratchpad.hex"

        if config.is_scratchpad_internal():
            # Generate a programming image (bootloader + scratchpad file)
            # and save it as Intel HEX.
            memory = gen_prog_image(config, bootloader,
                                    scratchpad_header, scratchpad_data)
            hextool.save_intel_hex(memory, filename = args.genprog)

            # Also generate a programming image without scratchpad
            # (bl+stack+app) for debug purposes and save it as Intel HEX.
            memory = gen_prog_image_without_scratchpad(config,
                                                       bootloader,
                                                       scratchpad_data,
                                                       in_files)
            hextool.save_intel_hex(memory, filename = file_without_scr)

        else:  # config.is_scratchpad_internal():
            # Generate a programming image without scratchpad (bl+stack+app).
            # and save it as Intel HEX.
            memory = gen_prog_image_without_scratchpad(config,
                                                       bootloader,
                                                       scratchpad_data,
                                                       in_files)
            hextool.save_intel_hex(memory, filename = file_without_scr)

    except (ValueError, IOError, OSError) as exc:
        sys.stdout.write("%s: %s\n" % (pgmname, exc))
        return 1

    return 0


# Run main.
if __name__ == "__main__":
    sys.exit(main())
