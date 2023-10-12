#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# genhex.py - A tool to generate flashable hex files
#
# Requires:
#   - Python 2 v2.7 or newer (uses argparse, hextool.py)
#     or Python 3 v3.2 or newer
#   - hextool.py and bootloader_config.py in the same directory as this file

import sys
import os
import struct
import argparse
import textwrap
import hextool

from bootloader_config import BootloaderConfig, KeyDesc
from genscratchpad import InFile


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


# Classes

class Flashable(object):
    '''A class to create flashable objects'''

    # Offset from start of area for \ref bl_info_header_t, in bytes.
    BL_INFO_HEADER_OFFSET = 16

    def __init__(self, config, bootloader):
        self.config = config
        bl_without_config = hextool.Memory()
        hextool.load_intel_hex(bl_without_config, filename = bootloader)
        self.bootloader = self.gen_bootloader(bl_without_config)
        self.infiles = []    # List of data blocks


    def append(self, infile):
        self.infiles.append(infile)

    def make_bl_info_header(self, in_file):
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

        # This area was not generated from a scratchpad.
        scr_length = 0xFFFFFFF0
        crc = 0xFFFF
        seq = 0xFF

        # Reserved for future use
        flags = 0x00

        if in_file.area_id < 0 or in_file.area_id > 4294967295:
            raise ValueError("memory area ID not 0 .. 4294967295")

        # written_size in memory area is rounded up to erase block boundary.
        written_size = \
            (int(len(in_file.raw_data) / self.config.flash.eraseblock) + 1) * \
            self.config.flash.eraseblock

        return struct.pack("<LH2BL4BL", scr_length, crc, seq, flags,
                        in_file.area_id, in_file.version[0], in_file.version[1],
                        in_file.version[2], in_file.version[3], written_size)

    def gen_bootloader(self, bootloader):
        '''Create the bootloader image with its configuration (areas and keys).'''

        bl_area = self.config.get_bootloader_area()

        # Set authentication and encryption key type, so that memory
        # addresses for bootloader settings can be calculated.
        key_type = self.config.get_key_type()
        bl_area.set_key_type(key_type)

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
        for area in self.config.areas.values():
            data = struct.pack("<4L", area.address, area.length,
                            area.id, area.flags | (area.type << 2))
            memory += data

        # Sanity check.
        if memory.cursor > set_key_start:
            raise ValueError("error generating programming image: bootloader areas")

        # Set Memory cursor to point at the bootloader key settings.
        memory.cursor = set_key_start

        # Add keys in Flash memory image.
        for key in self.config.keys.values():
            if key_type == KeyDesc.KEY_TYPE_OMAC1_AES128CTR:
                auth_key = key.authentication
            elif key_type == KeyDesc.KEY_TYPE_SHA256_ECDSA_P256_AES128CTR:
                auth_key = KeyDesc.get_public_key(key.authentication)

                if len(auth_key) != 65 or auth_key[0] != 0x04:
                    raise ValueError("not a valid uncompressed public key")

                # Discard first byte, which indicates
                # whether the key is compressed or not
                auth_key = auth_key[1:]
            else:
                raise ValueError("invalid key type")

            memory += auth_key
            memory += key.encryption

        # Sanity check.
        if memory.cursor > set_end:
            raise ValueError("error generating programming image: bootloader keys")

        return memory

    def get_hex(self):
        '''Create a Flash memory image, ready for programming to a device.'''

        # Create a Memory object to store the Flash memory image.
        memory = hextool.Memory()

        # Add bootloader to memory image
        memory += self.bootloader

        # Add other files to memory image.
        for in_file in self.infiles:
            added = False
            for area in self.config.areas.values():
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
                        header = self.make_bl_info_header(in_file)
                        memory.cursor = area.address + self.BL_INFO_HEADER_OFFSET
                        memory.overlap_ok = True
                        memory += header
                        memory.overlap_ok = False

                    added = True

            if not added:
                raise ValueError("\nArea id not found in ini file: 0x%x\n" % in_file.area_id)

        return memory

    def get_bootloader_hex(self):
        '''Create a Bootloader memory image, with its config.'''

        # Create a Memory object to store the Flash memory image.
        memory = hextool.Memory()

        # Add bootloader to memory image
        memory += self.bootloader

        return memory


# Functions

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
            "A tool to generate flashable hex files", help_width),
        epilog =
            "values:\n"
            "  infilespec    [version|file.conf]:area_id:filename or area_id:filename\n"
            "  area_id       0x00000000 .. 0xffffffff\n"
            "  version       major.minor.maintenance.developer, "
            "each 0 .. 255,\n"
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
    parser.add_argument("outfile", metavar = "OUTFILE",
        help = "flashable hex file")
    parser.add_argument("infilespec", nargs = "*", metavar = "INFILESPEC",
        help = "input file(s) in Intel HEX format to "
               "be added in scratchpad, see below")

    return parser

def parse_arguments(parser):
    '''Parse command line arguments.'''

    args = parser.parse_args()

    if not args.bootloader:
        raise ValueError("Bootloader file is mandatory")

    return args

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
        config = BootloaderConfig.from_ini_files(args.configfile)

        # Check that config is valid
        # ValueError exception will be raised in case of invalid config
        config.check_config()

    except (ValueError, IOError, OSError, configparser.Error) as exc:
        sys.stdout.write("%s: %s\n" % (pgmname, exc))
        return 1

    flash = Flashable(config, args.bootloader)

    # Only create bootloader_with_config.hex.
    if len(args.infilespec) == 0:
        try:
            memory = flash.get_bootloader_hex()
            hextool.save_intel_hex(memory, filename = args.outfile)

        except (ValueError, IOError, OSError) as exc:
            sys.stdout.write("%s: %s\n" % (pgmname, exc))
            return 1

        return 0
    else:
        # Read input files.
        try:
            for file_spec in args.infilespec:
                # Create an InFile object.
                in_file = InFile(file_spec = file_spec)
                flash.append(in_file)

        except (ValueError, IOError, OSError) as exc:
            sys.stdout.write("%s: %s\n" % (pgmname, exc))
            return 1

        try:
            # Combine bootloader, memory area specification, keys, stack and app.
            memory = flash.get_hex()
            hextool.save_intel_hex(memory, filename = args.outfile)

        except (ValueError, IOError, OSError) as exc:
            sys.stdout.write("%s: %s\n" % (pgmname, exc))
            return 1

    return 0


# Run main.
if __name__ == "__main__":
    sys.exit(main())
