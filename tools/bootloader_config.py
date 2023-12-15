#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import argparse
import binascii

from Crypto.PublicKey import ECC  # For checking the private key format

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

# Maximum number of memory areas
MAX_NUM_MEMORY_AREAS = 16

# Original, lower limit for the maximum number of memory areas, used
# with OMAC1 / CMAC authentication, for backward compatibility
MAX_NUM_MEMORY_AREAS_LEGACY = 8

# Size of a memory area header
MEMORY_AREA_HEADER_SIZE = 16

# Size of CMAC / OMAC1 authentication key in bytes
SIZE_OMAC1_KEY_IN_BYTES = 16

# Size of AES-128 encryption key in bytes
SIZE_AES128_KEY_IN_BYTES = 16

# Maximum number of key pairs
MAX_NUM_KEYS = 8


# Number of bytes reserved for bootloader settings
# The size of settings is hardcoded to be 1KB.
# In next version of bootloader, the setting areas should be all aligned to last kB of bootloader area
# It is not the case at the moment as some platforms have it at 14kB
BOOTLOADER_SETTINGS_NUM_BYTES = 0x400


class FlashDesc(object):
    """Flash description object

    An object to describe flash memory

    """
    def __init__(self, length, eraseblock):
        """Flash descriptor creation

        Args:
            length: length of the flash in bytes
            eraseblock: minimum block size for erase operation
        """
        self.length = length
        self.eraseblock = eraseblock

    def __repr__(self):
        return "<Flash len:%d eraseblock:%d>" % \
               (self.length, self.eraseblock)

    def __str__(self):
        return self.__repr__()

class AesConfig(object):
    """AES configuration description object

    An object to describe AES specific configuration

    """
    def __init__(self, aes_little_endian):
        """Little endian descriptor creation

        Args:
            aes_little_endian: AES CTR endianess
        """
        self.aes_little_endian = aes_little_endian

    def __repr__(self):
        return "<Little endian:%s>" % \
               (self.aes_little_endian)

    def __str__(self):
        return self.__repr__()

class AreaDesc(object):
    """Area description object

    An object to describe a Wirepas area in flash
    """
    # Flag definition
    FLAG_STORE_VERSION_NUMBER = 0x00000001
    FLAG_IN_EXTERNAL_FLASH = 0x00000002

    _FLAG_MASK = 0x00000003

    # Type definition
    BOOTLOADER_TYPE = 0
    STACK_TYPE = 1
    APP_TYPE = 2
    PERSISTENT_TYPE = 3
    SCRATCHPAD_TYPE = 4
    USER_TYPE = 5
    MODEMFW_TYPE = 6

    def __init__(self, area_id, address, length, flags, area_type):
        """Area descriptor creation

        Args:
            area_id: id of the area
            address: initial of address in Flash
            length: length of the area in bytes
            flags: flags of the area
            area_type: type of the area
        """
        self.id = area_id
        self.address = address
        self.length = length
        self.start = self.address
        self.end = self.start + self.length - 1

        if (flags | AreaDesc._FLAG_MASK) != AreaDesc._FLAG_MASK:
            raise ValueError("wrong flags for area id=%s", id)

        self.flags = flags
        if area_type < AreaDesc.BOOTLOADER_TYPE or area_type > AreaDesc.MODEMFW_TYPE:
            raise ValueError("wrong area type for area id=%s", id)
        self.type = area_type

    def is_scratchpad_type(self):
        """Is this area dedicated for scratchpad
        """
        return self.type == AreaDesc.SCRATCHPAD_TYPE

    def is_bootloader_type(self):
        """Is this area dedicated for bootloader
        """
        return self.type == AreaDesc.BOOTLOADER_TYPE

    def is_stack_type(self):
        """Is this area dedicated for Wirepas stack
        """
        return self.type == AreaDesc.STACK_TYPE

    def is_persistent_type(self):
        """Is this area dedicated for Wirepas persistent
        """
        return self.type == AreaDesc.PERSISTENT_TYPE

    def is_app_type(self):
        """Is this area dedicated for application
        """
        return self.type == AreaDesc.APP_TYPE

    def is_internal(self):
        """Is this area located in internal flash
        """
        return (self.flags & AreaDesc.FLAG_IN_EXTERNAL_FLASH) == 0

    def has_bl_info_header(self):
        """Does this area contains bootloader info header
        """
        return (self.flags & AreaDesc.FLAG_STORE_VERSION_NUMBER) != 0

    def __repr__(self):
        return "<Area id:0x%x address:0x%x len:%d type:%s>" \
               % (self.id,
                  self.address,
                  self.length,
                  self.type)

    def __str__(self):
        return self.__repr__()


class BootloaderAreaDesc(AreaDesc):
    """Bootloader area description object

    An object to describe a Wirepas bootloader area in flash
    """
    def __init__(self, area_id, address, length, flags, settings, key_type):
        """Bootloader Area descriptor creation

        Args:
            area_id: id of the area
            address: initial of address in Flash
            length: length of the area in bytes
            flags: flags of the area
            settings: address of the setting area relative to beginning of area
            key_type: authentication and encryption key type (from KeyDesc) or None
        """
        if (flags & AreaDesc.FLAG_STORE_VERSION_NUMBER) or (flags & AreaDesc.FLAG_IN_EXTERNAL_FLASH):
            raise ValueError("bootloader can only be internal without header")

        super(BootloaderAreaDesc, self).__init__(
            area_id, address, length, flags, AreaDesc.BOOTLOADER_TYPE)
        self.settings = settings
        if self.settings is None:
            raise ValueError("missing settings key in a bootloader area")

        if key_type not in (None,
                            KeyDesc.KEY_TYPE_OMAC1_AES128CTR,
                            KeyDesc.KEY_TYPE_SHA256_ECDSA_P256_AES128CTR):
            raise ValueError("invalid key type")
        self.key_type = key_type

    def set_key_type(self, key_type):
        if key_type not in (KeyDesc.KEY_TYPE_OMAC1_AES128CTR,
                            KeyDesc.KEY_TYPE_SHA256_ECDSA_P256_AES128CTR):
            raise ValueError("invalid key type")
        self.key_type = key_type

    def get_settings_start_address(self):
        """Get absolute setting start address
        """
        return self.address + self.settings

    def get_settings_end_address(self):
        """Get absolute settings area end address
        """
        return self.get_settings_start_address() + BOOTLOADER_SETTINGS_NUM_BYTES

    def get_key_start_address(self):
        """Get absolute keys area start address
        """

        address = self.get_settings_start_address()
        if self.key_type is None:
            raise ValueError("no key type set")
        elif self.key_type == KeyDesc.KEY_TYPE_SHA256_ECDSA_P256_AES128CTR:
            # With ECDSA P-256 authentication, support for 16 areas was added
            # and the keys had to be moved up in memory
            address += MAX_NUM_MEMORY_AREAS * MEMORY_AREA_HEADER_SIZE
        else:
            # With CMAC / OMAC1 authentication, use the original start
            # address for keys, for backward compatibility
            address += MAX_NUM_MEMORY_AREAS_LEGACY * MEMORY_AREA_HEADER_SIZE
        return address

    def __repr__(self):
        return "<Bootloader area id:0x%x address:0x%x len:%d settings=0x%x>" \
               % (self.id,
                  self.address,
                  self.length,
                  self.settings)

    def __str__(self):
        return self.__repr__()


class KeyDesc(object):
    """Key pair description object

    An object to describe a Wirepas bootloader key pair
    """

    KEY_TYPE_OMAC1_AES128CTR = 0  # Symmetric CMAC / OMAC1 authentication
    KEY_TYPE_SHA256_ECDSA_P256_AES128CTR = 1  # Private / public ECDSA P-256 authentication

    # Key type name to key type mapping
    KEY_TYPE_NAMES = {
        "OMAC1_AES128CTR": KEY_TYPE_OMAC1_AES128CTR,
        "ES256_AES128CTR": KEY_TYPE_SHA256_ECDSA_P256_AES128CTR
    }

    # Key type to key type name mapping
    KEY_TYPES = dict([(key_type, key_type_name) for
                       (key_type_name, key_type) in KEY_TYPE_NAMES.items()])

    def __init__(self, key_type, authentication, encryption, authpublic=None):
        """Create a key pair object

        Args:
            authentication (byteArray): 128-bit or 256-bit authentication key
            encryption (byteArray): 128-bit encryption key
            key_type: authentication and encryption key types, KEY_TYPE_...
        """
        if key_type == KeyDesc.KEY_TYPE_OMAC1_AES128CTR:
            if len(authentication) != SIZE_OMAC1_KEY_IN_BYTES:
                raise ValueError(f"authentication key is not 128 bits long")
            elif authpublic is not None:
                raise ValueError(f"authentication public key not supported")
        elif key_type == KeyDesc.KEY_TYPE_SHA256_ECDSA_P256_AES128CTR:
            # Private key
            privkey = None
            if authentication is not None:
                # Check that the key is in DER format and is a NIST P-256 private key
                privkey = self.__convert_and_check_key(
                    authentication, "authentication private key", True)

            # Optional public key
            pubkey = None
            if authpublic is not None:
                # Check that the key is in DER format and is a NIST P-256 public key
                pubkey = self.__convert_and_check_key(
                    authpublic, "authentication public key", False)

            if privkey is None and pubkey is None:
                # No key given
                raise ValueError("no private or public key given")
            elif privkey is not None and pubkey is not None:
                # Check that the private key produces a matching public key
                if (privkey.public_key().export_key(format='DER') !=
                    pubkey.export_key(format='DER')):
                    raise ValueError("authentication private key has a non-matching public key")
            elif privkey is None:
                # Only public key given
                authentication = authpublic
        else:
            raise ValueError("invalid key type")

        if len(encryption) != SIZE_AES128_KEY_IN_BYTES:
            raise ValueError("encryption key is not 128 bits long")

        self.key_type = key_type
        self.authentication = authentication
        self.encryption = encryption

    def __repr__(self):
        return "<Key type:%s authentication:%s encryption:%s>" % \
               (self.type_to_string(self.key_type),
                binascii.hexlify(self.authentication),
                binascii.hexlify(self.encryption))

    def __str__(self):
        return self.__repr__()

    @staticmethod
    def __convert_and_check_key(key, key_name, is_private):
        # Check that the key is in DER format and is a NIST P-256 key
        try:
            key = ECC.import_key(key)  # May raise ValueError or
                                       # ECC.UnsupportedEccFeature
        except Exception:
            raise ValueError("not a valid ECC key for %s" % key_name)

        if key.curve not in ("NIST P-256", "p256", "P-256", "prime256v1", "secp256r1"):
            raise ValueError(
                "unsupported ECC curve for %s: '%s'" % (key_name, key.curve))
        elif key.has_private() != is_private:
            priv_pub_text = is_private and "private" or "public"
            raise ValueError("not a %s key for %s" % (priv_pub_text, key_name))
        return key

    @staticmethod
    def get_public_key(private_key):
        public_key = ECC.import_key(private_key).public_key()  # May raise ValueError
        return public_key.export_key(format = "raw", compress = False)

    @staticmethod
    def parse_pem_key(key):
        try:
            key = ECC.import_key(key.strip())  # May raise ValueError or
                                               # ECC.UnsupportedEccFeature
        except Exception:
            raise ValueError("not a valid PEM key")
        return bytearray(key.export_key(format='DER'))

    @staticmethod
    def is_private_key(key):
        return ECC.import_key(key).has_private()  # May raise ValueError

    @staticmethod
    def type_to_string(key_type):
        try:
            return KeyDesc.KEY_TYPES[key_type]
        except KeyError:
            raise ValueError("invalid key type")

    @staticmethod
    def type_from_string(key_type_name):
        key_type_name = key_type_name.strip()
        try:
            return KeyDesc.KEY_TYPE_NAMES[key_type_name.upper()]
        except KeyError:
            raise ValueError("invalid key type '%s'" % key_type_name)

class BootloaderConfig(object):
    """Bootloader configuration

    An object describing the bootloader configuration
    """
    def __init__(self, flash, areas, keys, platform):
        """Create a bootloader config

        Args:
            flash (FlashDesc): FlashDesc object
            areas (Dic): Dictionary of area object (name, AreaDesc)
            keys (Dic): Dictionary of key object (name, KeyDesc)
            platform (AesConfig): AES configuration object
        """
        self.flash = flash
        self.areas = areas
        self.keys = keys
        # If platform configuration is missing set default values here.
        if platform is None:
            self.platform = AesConfig(aes_little_endian=True)
        else:
            self.platform = platform

    def __check_areas(self):
        """ Check if mandatory area definitions exists
        Raise exception if it is not the case
        """

        key_type = self.get_key_type()  # Raises ValueError() if no keys
        if key_type == KeyDesc.KEY_TYPE_OMAC1_AES128CTR:
            # With CMAC / OMAC1 authentication, use the original, lower
            # limit for the maximum number of memory areas, for backward
            # compatibility
            max_num_areas = MAX_NUM_MEMORY_AREAS_LEGACY
        elif key_type == KeyDesc.KEY_TYPE_SHA256_ECDSA_P256_AES128CTR:
            # With ECDSA P-256 authentication, support for 16 areas was added
            max_num_areas = MAX_NUM_MEMORY_AREAS
        else:
            raise ValueError("invalid key type")

         # Check the number of areas
        if len(self.areas) == 0:
            raise ValueError("no memory areas in configuration")
        elif len(self.areas) > max_num_areas:
            raise ValueError("too many memory areas in configuration")

        # Check the main areas are present
        if self.get_bootloader_area() is None:
            raise ValueError("no bootloader area in configuration")

        if self.get_wirepas_stack_area() is None:
            raise ValueError("no stack area in configuration")

        if self.get_persistent_area() is None:
            raise ValueError("no persistent area in configuration")

        app_area = self.get_app_area()
        if app_area is None:
            raise ValueError("no app area in configuration")
        elif not app_area.is_internal():
            raise ValueError("app can only be internal")

    def __check_keys(self):
        """ Check if security keys exist and are of the same type
        Raise exception if it is not the case
        """
        # Check the number of keys
        if len(self.keys) == 0:
            raise ValueError("no keys in configuration")
        if len(self.keys) > MAX_NUM_KEYS:
            raise ValueError("too many keys in configuration")

        key_type = None
        for _, key in self.keys.items():
            if key_type is not None and key_type != key.key_type:
                raise ValueError("all keys must be of the same type")
            key_type = key.key_type

    def __check_area_overlap(self, area1, area2):
        """Check if two area overlap

        Args:
            area1 (AreaDesc): first area
            area2 (AreaDesc): second area

        Returns:
            True if areas overlap, false otherwise
        """
        if area1.is_internal() != area2.is_internal():
            # area1 and area2 are not in same area space
            # so cannot overlap
            return False

        if area1.start < area2.start:
            # area1 is before area2
            if area1.end < area2.start:
                # area1 ends before area2
                return False
        elif area1.start > area2.start:
            # area1 is after area2
            if area1.start > area2.end:
                # area1 starts after area2
                return False

        # Any other case is an overlap
        return True

    def check_config(self):
        """Check that the config is valid

        Raise exception if it is not the case
        """

        # Check if there is a flash description
        if self.flash is None:
            raise ValueError("no flash section in configuration")
        # Check if all areas exists
        self.__check_areas()
        # Check if security keys exists
        self.__check_keys()
        # Check that there is no area overlap
        for area1 in self.areas.keys():
            # Could be optimized as we check 2 times the same combination
            for area2 in self.areas.keys():
                if area1 == area2:
                    continue
                if self.__check_area_overlap(self.areas[area1], self.areas[area2]):
                    raise ValueError("area %s and %s overlap" % (area1, area2))

        # Check if there is plaform specific configuration availabale
        if self.platform is None:
            raise ValueError("no platform specific configuration")

    @staticmethod
    def __get_area(cp, section, overlay_dic):
        """ Gets area name and area's content from section
        """
        read_flags = int(cp.get(section, "flags"), 0)
        # real flags are just bit 0 and 1
        flags = read_flags & 0x3
        # area type is bit 2, 3, 4
        area_type = (read_flags >> 2) & 0x7

        area_id_str = cp.get(section, "id")
        if overlay_dic is not None:
            try:
                area_id = int(overlay_dic[area_id_str], 0)
            except TypeError:
                # Not a string, try as int directly
                area_id = overlay_dic[area_id_str]
            except KeyError:
                # No overlay, convert value
                area_id = int(area_id_str, 0)
        else:
            # No overlay set
            area_id = int(area_id_str, 0)

        try:
            settings = int(cp.get(section, "settings"), 0)
        except configparser.NoOptionError:
            # Settings is not defined for this area
            settings = None

        if area_type == AreaDesc.BOOTLOADER_TYPE:
            area = BootloaderAreaDesc(
                area_id=area_id,
                address=int(cp.get(section, "address"), 0),
                length=int(cp.get(section, "length"), 0),
                flags=flags,
                settings=settings,
                key_type=None  # Not known yet
            )
        else:
            area = AreaDesc(
                area_id=area_id,
                address=int(cp.get(section, "address"), 0),
                length=int(cp.get(section, "length"), 0),
                flags=flags,
                area_type=area_type)
        return area


    @classmethod
    def from_ini_files(cls, files, overlay_dic={}):
        """Parse ini files to create a bootloader config object

        Args:
            files: the ini files list to parse
            overlay_dic: dictionary containing string key to be replaced by a value

        Returns:
            BootloaderConfig object
        """

        def parse_hex_key(value):
            """Parse a key expressed in ASCII hex string

            Bytes are optionally separated by commas or spaces.
            """
            value = value.replace(",", " ")

            try:
                return bytearray.fromhex(value)
            except ValueError:
                raise ValueError("invalid hex string")

        def parse_key(value):
            """Parse a key either in PEM or ASCII hex string format"""
            if value is None:
                # Key not given
                return None

            if "BEGIN" in value:
                # Try to parse key as PEM key
                return KeyDesc.parse_pem_key(value)

            # Try to parse key as hex
            return parse_hex_key(value)

        # Dictionary of area found
        areas = {}
        # Dictionary of keys found
        keys = {}
        flash = None
        platform = None

        # Read configuration files one by one to detect section duplication
        for file in files:
            cp = configparser.RawConfigParser(
                    **config_parser_tweaks)  # Most basic parser only

            cp.read(file)

            for section in cp.sections():
                # Check if it is an area
                if section.startswith("area:"):
                    area_name = section.split(":")[1]
                    if area_name in areas.keys():
                        raise ValueError("area '%s' defined multiple times in files: %s" % (area_name, files))
                    area = cls.__get_area(cp, section, overlay_dic)
                    areas[area_name] = area
                    continue

                # Check if it is a key
                if section.startswith("key:"):
                    key_name = section.split(":")[1]
                    if key_name in keys.keys():
                        raise ValueError(
                            "key '%s' defined multiple times in files: %s" % (key_name, files))

                    try:
                        key_type = KeyDesc.type_from_string(
                            cp.get(section, "keytype"))

                        if key_type == KeyDesc.KEY_TYPE_OMAC1_AES128CTR:
                            authentication = parse_hex_key(cp.get(section, "auth"))
                            authpublic = None
                        elif key_type == KeyDesc.KEY_TYPE_SHA256_ECDSA_P256_AES128CTR:
                            authentication = parse_key(
                                cp.get(section, "authprivate", fallback = None))
                            authpublic = parse_key(
                                cp.get(section, "authpublic", fallback = None))

                        encryption = parse_hex_key(cp.get(section, "encrypt"))

                        key = KeyDesc(
                            key_type=key_type,
                            authentication=authentication,
                            encryption=encryption,
                            authpublic=authpublic)
                    except ValueError as exc:
                        raise ValueError(
                            "%s in section: '%s', files: %s" % (str(exc), section, files))
                    keys[key_name] = key
                    continue

                # Check if it is a flash section
                if section == "flash":
                    if flash is not None:
                        raise ValueError("flash section already defined")

                    flash = FlashDesc(length=int(cp.get(section, "length"), 0),
                                      eraseblock=int(cp.get(section, "eraseblock"), 0))
                    continue

                # Check if it is a platform configuration section
                if section == "platform":
                    if platform is not None:
                        raise ValueError("platform section already defined")
                    platform = AesConfig(aes_little_endian=(cp.getboolean(section, "aes_little_endian")))
                    continue
                print("Warning: unknown section %s" % section)

        return cls(flash=flash,
                   areas=areas,
                   keys=keys,
                   platform=platform)

    def to_ini_file(self, file):
        """Generate ini file from current config

        Args:
            file: file to write
        """

        def format_hex_key(value):
            """Format a binary key to an ASCII hex string

            Bytes are separated by spaces.
            """
            return "".join("{:02X} ".format(x) for x in value)

        config = configparser.ConfigParser()
        if self.flash is not None:
            config.add_section('flash')
            config.set('flash','length', str(self.flash.length))
            config.set('flash','eraseblock', str(self.flash.eraseblock))

        if self.areas is not None:
            for area_name in self.areas.keys():
                section_name = 'area:{}'.format(area_name)
                area = self.areas[area_name]
                config.add_section(section_name)
                config.set(section_name, 'id', '0x%x' % area.id)
                config.set(section_name, 'address', '0x%x' % area.address)
                config.set(section_name, 'length', str(area.length))
                config.set(section_name, 'flags', '0x%x' % (area.flags | (area.type << 2)))

                # Will fail if it is not a bootloader area
                try:
                    config.set(section_name, 'settings', '0x%x' % area.settings)
                except AttributeError:
                    pass

        for key_name in self.keys.keys():
            key = self.keys[key_name]
            section_name = 'key:{}'.format(key_name)
            config.add_section(section_name)
            config.set(section_name, 'keytype', KeyDesc.type_to_string(key.key_type))
            if key.key_type == KeyDesc.KEY_TYPE_OMAC1_AES128CTR:
                auth_name = 'auth'
            elif key.key_type == KeyDesc.KEY_TYPE_SHA256_ECDSA_P256_AES128CTR:
                if KeyDesc.is_private_key(key.authentication):
                    auth_name = 'authprivate'
                else:
                    auth_name = 'authpublic'
            config.set(section_name, auth_name, format_hex_key(key.authentication))
            config.set(section_name, 'encrypt', format_hex_key(key.encryption))

        if (self.platform) is not None:
            config.add_section('platform')
            config.set('platform', 'aes_little_endian', str(self.platform.aes_little_endian))

        with open(file, 'w') as file:
            config.write(file)

    def is_scratchpad_internal(self):
        """Tells if the scratchpad is in an internal area or not

        Returns:
            True is scratchpad is internal
        """
        scratchpad_area = self.get_scratchpad_area()
        if scratchpad_area is None:
            raise ValueError("no scratchpad area found")

        return scratchpad_area.is_internal()

    def get_app_area(self):
        """"Get the app area

        Returns:
            An AreaSpec object for app area
        """
        for area in self.areas.values():
            if area.is_app_type():
                return area

        return None

    def get_scratchpad_area(self):
        """"Get the scratchpad area

        Returns:
            An AreaSpec object for scratchpad area
        """
        for area in self.areas.values():
            if area.is_scratchpad_type():
                return area

        # No dedicated area => scratchpad is in app area
        return self.get_app_area()

    def get_bootloader_area(self):
        """Get the bootloader area

        Returns:
            A BootloaderAreaSpec object that contains the bootloader area information
        """
        for area in self.areas.values():
            if area.is_bootloader_type():
                return area

        # No bootloader area found
        return None

    def get_wirepas_stack_area(self):
        """Get the Wirepas stack area

        Returns:
            An AreaSpec object for Wirepas stack area
        """
        for area in self.areas.values():
            if area.is_stack_type():
                return area

        # No area found
        return None

    def get_persistent_area(self):
        """Get the Wirepas persistent area

        Returns:
            An AreaSpec object for persistent stack area
        """
        for area in self.areas.values():
            if area.is_persistent_type():
                return area

        # No area found
        return None

    def get_key_type(self):
        for _, key in self.keys.items():
            return key.key_type
        raise ValueError("no keys in configuration")

    def __repr__(self):
        return "<Bootloader config flash:%s areas:%s keys:%s>" % (self.flash,
                                                                  self.areas,
                                                                  self.keys)

    def __str__(self):
        return "Bootloader config is as followed:\n" \
               "\t[flash] %s \n" \
               "\t[areas] %s \n" \
               "\t[keys] %s \n" \
               "\t[platform] %s \n" \
                                % (self.flash,
                                   self.areas,
                                   self.keys,
                                   self.platform)


def ini_file(string_file):
    """Parser for ini_file

    Args:
        string_file: a file with .ini extension
    """
    if not string_file.endswith(".ini"):
        raise argparse.ArgumentTypeError("invalid ini file extension %s" % string_file)
    return string_file


def overlay_option(string_overlay):
    """Parser for overlay

    Args:
        string_overlay: a string with format key:val
    """
    if len(string_overlay.split(':')) != 2:
        raise argparse.ArgumentTypeError("invalid overlay entry %s" % string_overlay)
    return string_overlay


def main():
    """Main program

    Used as a script, it allows to combine multiple ini file into a single one and test it if needed
    and write it as a single file
    """

    # Determine program name, for error messages
    pgmname = os.path.split(sys.argv[0])[-1]

    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("--in_file", "-i",
                        action='append',
                        type=ini_file,
                        help="List of ini files to create a config",
                        required=True)

    parser.add_argument("--out_file", "-o",
                        type=ini_file,
                        help="File to write the final config")

    parser.add_argument("--overlay", "-ol",
                        action='append',
                        type=overlay_option,
                        help="Specify an overlay entry as val:key")

    parser.add_argument("--check", "-c",
                        action='store_true',
                        help="Should final config be checked")

    parser.add_argument("--get_key_type", "-kt",
                        action='store_true',
                        help="Return key type used in config")

    try:
        args = parser.parse_args()

        # Create overlay dic
        overlay_dic = {}
        if args.overlay is not None:
            for ol in args.overlay:
                items = ol.split(":")
                overlay_dic[items[0]] = items[1]

        # Create config
        config = BootloaderConfig.from_ini_files(args.in_file, overlay_dic)

        # Check config
        if args.check:
            config.check_config()

        # Return key type used in config
        if args.get_key_type:
            print(KeyDesc.type_to_string(config.get_key_type()))
    except (ValueError, IOError, OSError, configparser.Error) as exc:
        sys.stdout.write("%s: %s\n" % (pgmname, exc))
        return 1

    # Write out config if needed
    if args.out_file:
        config.to_ini_file(args.out_file)


# Run main.
if __name__ == "__main__":
    sys.exit(main())
