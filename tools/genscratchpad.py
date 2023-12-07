#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# genscratchpad.py - A tool to generate encrypted scratchpad contents
#
# Requires:
#   - Python 2 v2.7 or newer (uses argparse, hextool.py)
#     or Python 3 v3.2 or newer
#   - PyCryptodome v3.0 or newer
#   - hextool.py and bootloader_config.py in the same directory as this file

import sys
import os
import zlib
import struct
import argparse
import textwrap
import hextool

from Crypto.Hash import CMAC, SHA256
from Crypto.Cipher import AES
from Crypto.PublicKey import ECC
from Crypto.Signature import DSS
from Crypto.Util import Counter
from Crypto.Random import get_random_bytes


from bootloader_config import BootloaderConfig, KeyDesc
from socket import htonl

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

class InFile(object):
    '''An input file'''

    # Maximum number of bytes in an input file
    MAX_NUM_BYTES_PER_FILE = (1024 * 1024)  # One megabyte

    # Data block length, in bytes
    BLOCK_LENGTH = 16

    def __init__(self, file_spec = None, data = None,
                 version = (0, 0, 0, 0), area_id = 0x00000000,
                 compressible = None, encryptable = None):
        if file_spec == None and data == None:
            raise ValueError("no data given")
        elif file_spec != None and data != None:
            raise ValueError("file_spec and data are mutually exclusive")

        # Set default values.
        self.area_id = area_id
        self.version = version
        self.compressible = compressible
        self.encryptable = encryptable
        self.filename = None
        self.compressed = False
        self.encrypted = False
        self.special_pad_field = False  # Must be False for bootloader to work

        if data:
            # Data given, make a copy of it.
            self.data = bytearray(data)
            self.raw_data = bytearray(data)
        else:
            # File specification given, parse it.
            version, self.area_id, filename = self.parse_file_spec(file_spec)
            self.version = self.parse_version(version)
            self.filename = filename

            # determine what to do based on the file extension
            filename_base, filename_ext = os.path.splitext(filename)
            if filename_ext == '.hex':
                # Read data from file.
                memory = hextool.Memory()
                hextool.load_intel_hex(memory, filename = filename)

                if memory.num_ranges == 0:
                    # No data found in file.
                    raise ValueError("file contains no data: '%s'" % filename)
                elif (memory.max_address -
                    memory.min_address > self.MAX_NUM_BYTES_PER_FILE):
                    raise ValueError("file too big: '%s'" % filename)

                # Convert Memory object to a flat bytearray.
                self.data = memory[memory.min_address:memory.max_address]
                self.raw_data = memory[memory.min_address:memory.max_address]
                if self.compressible is None:
                    self.compressible = True
                if self.encryptable is None:
                    self.encryptable = True
            elif filename_ext == '.cbor':
                memory = hextool.Memory()
                hextool.load_binary(memory, filename = filename)
                self.data = memory[0:memory.num_bytes]
                self.raw_data = memory[0:memory.num_bytes]
                if self.compressible is None:
                    self.compressible = False
                if self.encryptable is None:
                    self.encryptable = False
                self.special_pad_field = True  # Scratchpad is not for bootloader
            else:
                raise ValueError("unsupported file extension: '%s'" % filename)

        # Create a file header for uncompressed, unencrypted data.
        self.header = self.make_file_header(self.area_id, len(self.data),
                                            *self.version)

    def compress(self):
        if self.compressed:
            raise ValueError("data already compressed")

        if self.compressible:
            # OPTIMIZE: Avoid extra conversions between bytearrays and bytes.
            # Compress data. zlib.compress() does not accept
            # bytearrays, so convert bytearray to bytes.
            compressed_data = bytearray(zlib.compress(bytes(self.data), 9))

            # Determine compressed data length without
            # zlib header and Adler-32 checksum.
            comp_data_len = len(compressed_data) - 2 - 4

            # Pad size to a multiple of block length.
            num_blocks = (
                comp_data_len + self.BLOCK_LENGTH - 1) // self.BLOCK_LENGTH

            # Create a bytearray object for compressed data.
            self.data = bytearray(num_blocks * self.BLOCK_LENGTH)

            # Copy compressed data to bytearray, but leave
            # out zlib header and Adler-32 checksum.
            self.data[:comp_data_len] = compressed_data[2:-4]
            data_length = len(self.data)
            pad = data_length - comp_data_len
        else:
            # Do not compress, but pad size to a multiple of block length.
            data_pld_len = len(self.data)
            num_blocks = (
                data_pld_len + self.BLOCK_LENGTH - 1) // self.BLOCK_LENGTH
            fill_zeros = num_blocks * self.BLOCK_LENGTH - data_pld_len
            self.data.extend(bytearray(b"\x00" * fill_zeros))
            data_length = len(self.data)
            pad = data_length - data_pld_len

        if not self.special_pad_field:
            # Pad can only be zero for scratchpads that are
            # to be processed by the bootloader
            pad = 0x00000000
        else:
            # Scratchpad is not for the bootloader and the
            # pad field has special meaning
            pass

        # Update the file header now that data length has changed.
        self.header = self.make_file_header(self.area_id, data_length,
                                            *self.version, pad)

        # Mark data as compressed.
        self.compressed = True

    def encrypt(self, cipher):
        '''Ecrypt file data

        cipher  Initialized cipher object
        '''

        if self.encrypted:
            raise ValueError("data already encrypted")

        # Encrypt data and update counter.
        enc_data = cipher.encrypt(bytes(self.data))

        if self.encryptable:
            # OPTIMIZE: Avoid extra conversions between bytearrays and bytes.
            # Cipher does not accept bytearrays, so convert to bytes.
            self.data = bytearray(enc_data)
        else:
            # File is not to be encrypted, only update counter.
            pass

        # Mark data as encrypted.
        self.encrypted = True

    @staticmethod
    def parse_file_spec(file_spec):
        '''
        Parse input file specification.

        file_spec   version:area_id:filename or file.conf:area_id:filename
        '''

        try:
            # Read version, memory area ID and file name.
            fields = file_spec.split(":", 2)
            if fields.__len__() == 3:
                version, area_id, filename = fields
            elif fields.__len__() == 2:
                # Version is not provided, probably useless (as some area do not have versioning)
                version = "0.0.0.0"
                area_id, filename = fields
            else:
                raise ValueError("invalid input file specification: "
                                 "'%s'" % file_spec)

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
                    raise ValueError("invalid configuration file format, "
                                     "cannot find version: '%s'" % version)

                # Override version
                version = extracted_version

            area_id = int(area_id, 0)
            if area_id < 0 or area_id > 4294967295:
                raise ValueError
        except ValueError:
            raise ValueError("invalid input file specification: "
                             "'%s'" % file_spec)

        return version, area_id, filename

    @staticmethod
    def parse_version(version):
        '''Parse a version number of form MINOR.MAJOR.MAINTENANCE.DEVELOPMENT.'''

        try:
            # Split to components.
            ver_components = version.strip().split(".")

            if len(ver_components) != 4:
                # Wrong number of components.
                raise ValueError

            # Convert to integers.
            ver_components = [int(n) for n in ver_components]
        except ValueError:
            raise ValueError("invalid firmware version: '%s'" % version)

        return tuple(ver_components)

    @staticmethod
    def make_file_header(areaid, length,
                         ver_major, ver_minor, ver_maint, ver_devel,
                         pad = 0x00000000):
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
            /** Padding, reserved for future use, must be < 16 */
            uint32_t pad;
        };
        '''

        if areaid < 0 or areaid > 4294967295:
            raise ValueError("memory area ID not 0 .. 4294967295")

        return struct.pack("<2L4BL", areaid, length,
                           ver_major, ver_minor, ver_maint, ver_devel, pad)


class SignatureTLVFile(object):
    '''A signature file with TLV records inside'''

    # Area ID of public key signature
    AREA_ID = 0x027C362F  # “PKSIG” in DEC Radix-50 encoding

    # Signature file format version: 1.0.0.0
    FILE_FORMAT_VERSION = (1, 0, 0, 0)

    # The only supported authentication algorithm
    AUTH_ALGO_SHA256_ECDSA_P256 = 0

    # The only supported encryption algorithm
    ENCRYPT_ALGO_AES128CTR = 0

    # Size of ECDSA P-256 signature, in bytes
    ECDSA_P256_SIGNATURE_SIZE = 64  # R and S in big-endian order

    # Supported signature file record types
    REC_TYPE_END_OF_FILE = 0x00
    REC_TYPE_HEADER = 0x01
    REC_TYPE_AUTH_ALGO = 0x02
    # PKSIG_REC_TYPE_AUTH_ALGO_PARAMS = 0x03
    REC_TYPE_ENCRYPT_ALGO = 0x04
    # PKSIG_REC_TYPE_ENCRYPT_ALGO_PARAMS = 0x05
    REC_TYPE_SIGNATURE = 0x06

    def __init__(self):
        self.records = []
        self.header = b""
        self.data = b""
        self.infile = None

    def add_auth_algo(self, algo):
        if algo != self.AUTH_ALGO_SHA256_ECDSA_P256:
            raise ValueError("invalid authentication algorithm")
        elif self.infile is not None:
            raise ValueError("data already compressed")

        # Add authentication algorithm record
        auth_algo_id = b"ES256"  # ECDSA P-256 authentication with SHA-256 hash
        auth_algo_rec = (
            struct.pack("<BB", self.REC_TYPE_AUTH_ALGO,
                        len(auth_algo_id)) + auth_algo_id)
        self.records.append(auth_algo_rec)


    def add_encrypt_algo(self, algo):
        if algo != self.ENCRYPT_ALGO_AES128CTR:
            raise ValueError("invalid encryption algorithm")
        elif self.infile is not None:
            raise ValueError("data already compressed")

        # Add encryption algorithm record
        encrypt_algo_id = b"AES128CTR"  # AES-128 in counter mode (CTR)
        encrypt_algo_rec = (
            struct.pack("<BB", self.REC_TYPE_ENCRYPT_ALGO,
                        len(encrypt_algo_id)) + encrypt_algo_id)
        self.records.append(encrypt_algo_rec)

    def add_signature(self, signature = None):
        if signature is None:
            # No signature given, use zeros
            signature = b"\x00" * self.ECDSA_P256_SIGNATURE_SIZE
        elif len(signature) != self.ECDSA_P256_SIGNATURE_SIZE:
            raise ValueError("invalid signature size")

        if self.infile is not None:
            raise ValueError("data already compressed")

        # Add signature record
        signature_rec = (
            struct.pack("<BB", self.REC_TYPE_SIGNATURE,
                        len(signature)) + signature)
        self.records.append(signature_rec)

    def compress(self):
        '''Combine records and add padding

        No actual compression happens. This just mimics the InFile() API.'''

        if self.infile is not None:
            raise ValueError("data already compressed")

        # Combine records.
        pksig_file_data = b"".join(self.records)

        # Create an InFile temporarily, so we can extract the header and data.
        self.infile = InFile(data = pksig_file_data,
                             version = self.FILE_FORMAT_VERSION,
                             area_id = self.AREA_ID,
                             compressible = False,
                             encryptable = False)

        # Just do padding.
        self.infile.compress()

    def encrypt(self, cipher):
        '''Update cipher counter and extract file header and data

        No actual encryption happens. This just mimics the InFile() API.'''

        if self.infile is None:
            raise ValueError("data not compressed yet")

        # Just update cipher counter.
        self.infile.encrypt(cipher)

        # Allow header and data to be read.
        self.data = self.infile.data
        self.header = self.infile.header


class Scratchpad(object):
    '''A class to create scratchpads'''

    # Offset from start of area for \ref bl_info_header_t, in bytes.
    BL_INFO_HEADER_OFFSET = 16

    # Size of \ref bl_scratchpad_header_t, in bytes.
    BL_INFO_HEADER_SIZE = 16

    # Size of message authentication code, in bytes.
    CMAC_SIZE = 16

    # Magic 16-byte string for locating a combi scratchpad in Flash
    SCRATCHPAD_V1_TAG = b"SCR1\232\223\060\202\331\353\012\374\061\041\343\067"

    def __init__(self, key, platform):
        self.key = key
        self.platform = platform
        self.cmac = bytearray(self.CMAC_SIZE)
        self.data = []    # List of scratchpad file header and data blocks

        self.secure_header = get_random_bytes(16)
        self.cipher = self.create_cipher()

        # Appending files is allowed until get_otap() is called
        self.can_append = True

        if key.key_type == KeyDesc.KEY_TYPE_SHA256_ECDSA_P256_AES128CTR:
            # Update cipher counter with dummy data when using ECDSA P-256
            # authentication, so that old bootloaders can properly decrypt
            # files after the ECDSA signature file.
            pksig = SignatureTLVFile()
            pksig.add_auth_algo(SignatureTLVFile.AUTH_ALGO_SHA256_ECDSA_P256)
            pksig.add_encrypt_algo(SignatureTLVFile.ENCRYPT_ALGO_AES128CTR)
            pksig.add_signature(None)  # Dummy signature
            self.append(pksig)

    def append(self, infile):
        if not self.can_append:
            raise ValueError("cannot append any more files")
        infile.compress()
        infile.encrypt(self.cipher)
        self.data.append(infile.header)
        self.data.append(infile.data)

    def crc16_ccitt(self, data, initial = 0xffff):
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

    def create_cipher(self):
        '''
        Create an AES-128 Counter (CTR) mode cipher
        with 16-byte initial counter block.
        '''

        # AES.new() only accepts bytes, not bytearray.
        cipher_key = bytes(self.key.encryption)

        # Create a fast counter for AES cipher.
        icb0, icb1, icb2, icb3 = struct.unpack("<4L", self.secure_header)

        # Arrange AES nonce (IV) according to target's AES endianness.
        if self.platform.aes_little_endian == False:
            initctr =   htonl(icb3)       | \
                        htonl(icb2) << 32 | \
                        htonl(icb1) << 64 | \
                        htonl(icb0) << 96
        else:
            initctr = (icb3 << 96) | (icb2 << 64) | (icb1 << 32) | icb0

        ctr = Counter.new(128, little_endian = self.platform.aes_little_endian,
                          allow_wraparound = True,
                          initial_value = initctr)

        # Create an AES Counter (CTR) mode cipher.
        return AES.new(cipher_key, AES.MODE_CTR, counter = ctr)

    def calculate_cmac(self):
        '''Calculate CMAC / OMAC1 tag for data.'''

        # CMAC.new() only accepts bytes, not bytearray.
        auth_key = bytes(self.key.authentication)

        # Create a CMAC / OMAC1 object.
        cobj = CMAC.new(auth_key, ciphermod = AES)

        # OPTIMIZE: Avoid extra conversions between bytearrays and bytes.
        # CMAC object does not accept bytearrays, so convert to bytes.
        cobj.update(bytes(self.secure_header))
        for data in self.data:
            cobj.update(bytes(data))

        # Calculate digest and return it as bytearray.
        self.cmac = bytearray(cobj.digest())

    def calculate_ecdsa_p256_with_sha256(self):
        '''Calculate ECDSA P-256 signature for SHA-256 hash of data.'''

        # Calculate SHA-256 hash over file headers and data.
        hobj = SHA256.new()
        for d in self.data[2:]:  # Skip over the signature file itself + header
            hobj.update(d)

        # Read ECDSA P-256 private key.
        key = ECC.import_key(self.key.authentication)
        if key.curve != "p256":
            raise ValueError("unsupported ECC curve: '%s'" % key.curve)
        elif not key.has_private():
            raise ValueError("no private key given")

        # Calculate ECDSA P-256 over the SHA-256 hash.
        signer = DSS.new(key, mode = "fips-186-3", encoding = "binary")
        signature = signer.sign(hobj)

        # Create a file containing ECDSA P-256 signature.
        pksig = SignatureTLVFile()
        pksig.add_auth_algo(SignatureTLVFile.AUTH_ALGO_SHA256_ECDSA_P256)
        pksig.add_encrypt_algo(SignatureTLVFile.ENCRYPT_ALGO_AES128CTR)
        pksig.add_signature(signature)
        pksig.compress()
        pksig.encrypt(self.cipher)  # This will unfortunately
                                    # invalidate the cipher counter.

        # Replace first file (the dummy signature file) header and data.
        self.data[0] = pksig.header
        self.data[1] = pksig.data

    def make_header(self):
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

        # Calculate length of scratchpad contents.
        length = (self.CMAC_SIZE + len(self.secure_header) +
                  sum(map(len, self.data)))

        if length % 16 != 0:
            raise ValueError("data length not multiple of 16")

        # Calculate CRC of scratchpad contents.
        crc = self.crc16_ccitt(self.cmac)
        crc = self.crc16_ccitt(self.secure_header, crc)
        for data in self.data:
            crc = self.crc16_ccitt(data, crc)

        pad = 0x00

        bltype = 0x00000000     # BL_HEADER_TYPE_FIRMWARE
        blstatus = 0xffffffff   # BL_HEADER_STATUS_NEW
        seq = 255               # Will be overwritten when scratchpad will be
                                # uploaded to sink.

        return struct.pack("<LH2B2L", length, crc, seq, pad, bltype, blstatus)


    def get_otap(self):
        '''
        Get a memory image containing a scratchpad for .otap file generation.
        '''

        # Use authentication scheme selected by key type.
        if self.key.key_type == KeyDesc.KEY_TYPE_OMAC1_AES128CTR:
            # OMAC1 / CMAC authentication
            self.calculate_cmac()
        elif self.key.key_type == KeyDesc.KEY_TYPE_SHA256_ECDSA_P256_AES128CTR:
            # ECDSA P-256 authentication with SHA-256 hash
            self.calculate_ecdsa_p256_with_sha256()

            # Clear CMAC bytes, as they are not used when ECDSA P-256
            # authentication is used.
            self.cmac = bytearray(self.CMAC_SIZE)

        # Collect all data in a list
        otap = [self.SCRATCHPAD_V1_TAG]
        otap.append(self.make_header())
        otap.append(self.cmac)
        otap.append(self.secure_header)
        otap.extend(self.data)

        # Cipher counter is invalid now, so cannot append any more files.
        self.can_append = False

        # Return a valid scratchpad
        return b"".join(otap)


# Functions

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
        description = textwrap.fill(
            "A tool to generate compressed and "
            "encrypted scratchpad contents", help_width),
        epilog =
            "values:\n"
            "  infilespec    [version|file.conf]:area_id:filename\n"
            "  area_id       0x00000000 .. 0xffffffff\n"
            "  version       major.minor.maintenance.developer, "
            "each 0 .. 255,\n"
            "  file.conf     a configuration file containing metadatas"
            " associated to the filename")
    parser.add_argument("--configfile", "-c",
        type = ini_file, action = "append", metavar = "FILE", required = True,
        help = "one or more configuration files with keys")
    parser.add_argument("--keyname", "-k",
        metavar = "NAME", default = "default",
        help = "name of key to use for encryption and authentication "
               "(default: \"%(default)s\")")
    parser.add_argument("outfile", metavar = "OUTFILE",
        help = "compressed, encrypted output scratchpad file")
    parser.add_argument("infilespec", nargs = "+", metavar = "INFILESPEC",
        help = "input file(s) in Intel HEX format to "
               "be added in scratchpad, see below")

    return parser

def main():
    '''Main program'''

    # Determine program name, for error messages.
    pgmname = os.path.split(sys.argv[0])[-1]

    # Create a parser for parsing the command line and printing error messages.
    parser = create_argument_parser(pgmname)

    try:
        # Parse command line arguments.
        args = parser.parse_args()

        # Parse configuration file.
        config = BootloaderConfig.from_ini_files(args.configfile)

        # Check the key chosen is declared
        try:
            chosenkey = config.keys[args.keyname]
        except KeyError:
            raise ValueError("key not found: '%s'" % args.keyname)

        # Check that key type is supported
        if chosenkey.key_type not in (KeyDesc.KEY_TYPE_OMAC1_AES128CTR,
                                      KeyDesc.KEY_TYPE_SHA256_ECDSA_P256_AES128CTR):
            raise ValueError("key type '%s' not supported" %
                             KeyDesc.type_to_string(chosenkey.key_type))

    except (ValueError, IOError, OSError, configparser.Error) as exc:
        sys.stdout.write("%s: %s\n" % (pgmname, exc))
        return 1

    # Create a scratchpad object
    scratchpad = Scratchpad(chosenkey, config.platform)

    try:
        # Read input files.
        for file_spec in args.infilespec:
            # Create an InFile object.
            in_file = InFile(file_spec = file_spec)
            scratchpad.append(in_file)

        # Write output file.
        with open(args.outfile, "wb") as f:
            f.write(scratchpad.get_otap())

    except (ValueError, IOError, OSError) as exc:
        sys.stdout.write("%s: %s\n" % (pgmname, exc))
        return 1

    return 0

# Run main.
if __name__ == "__main__":
    sys.exit(main())
