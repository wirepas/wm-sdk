#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import argparse
import binascii

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
MAX_NUM_MEMORY_AREAS = 8

# Size of a memory area header
MEMORY_AREA_HEADER_SIZE = 16

# Size of a key in byte
SIZE_KEY_IN_BYTES = 16

# Maximum number of key pairs
MAX_NUM_KEYS = (1024 - MAX_NUM_MEMORY_AREAS * MEMORY_AREA_HEADER_SIZE) // (SIZE_KEY_IN_BYTES * 2)


# Number of bytes reserved for bootloader settings
# The size of settings is hardcoded to be 1KB.
# In next version of bootloader, the setting areas should be all aligned to last kB of bootloader area
# It is not the case at the moment as some platforms have it at 14kB
BOOTLOADER_SETTINGS_NUM_BYTES = 0x400


class FlashDesc(object):
    """Flash description object

    An objetc to describe a flash memory

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

    An objetc to describe a Wirepas area in flash
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
        if area_type < AreaDesc.BOOTLOADER_TYPE or area_type > AreaDesc.USER_TYPE:
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

    An objetc to describe a Wirepas bootloader area in flash
    """
    def __init__(self, area_id, address, length, flags, settings):
        """Bootloader Area descriptor creation

        Args:
            area_id: id of the area
            address: initial of address in Flash
            length: length of the area in bytes
            flags: flags of the area
            settings: address of the setting area relative to beginning of area
        """
        if (flags & AreaDesc.FLAG_STORE_VERSION_NUMBER) or (flags & AreaDesc.FLAG_IN_EXTERNAL_FLASH):
            raise ValueError("bootloader can only be internal without header")

        super(BootloaderAreaDesc, self).__init__(
            area_id, address, length, flags, AreaDesc.BOOTLOADER_TYPE)
        self.settings = settings
        if self.settings is None:
            raise ValueError("missing settings key in a bootloader area")

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
        return self.get_settings_start_address() + MAX_NUM_MEMORY_AREAS * MEMORY_AREA_HEADER_SIZE

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

    An objetc to describe a Wirepas bootloader key pair
    """
    def __init__(self, authentication, encryption):
        """Create a key pair object

        Args:
            authentication (byteArray): 128bits authentication key
            encryption (byteArray): 128bits encryption key
        """
        if len(authentication) != SIZE_KEY_IN_BYTES:
            raise ValueError("authentication key is not 128 bits long")

        if len(encryption) != SIZE_KEY_IN_BYTES:
            raise ValueError("encryption key is not 128 bits long")

        self.authentication = authentication
        self.encryption = encryption

    def __repr__(self):
        return "<Key authentication:%s encryption:%s>" % \
               (binascii.hexlify(self.authentication),
                binascii.hexlify(self.encryption))

    def __str__(self):
        return self.__repr__()


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
         # Check the number of areas
        if len(self.areas) == 0:
            raise ValueError("no memory areas in configuration")
        elif len(self.areas) > MAX_NUM_MEMORY_AREAS:
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
        """ Check if security keys exists
        Raise exception if it is not the case
        """
        # Check the number of keys
        if len(self.keys) == 0:
            raise ValueError("no keys in configuration")
        if len(self.keys) > MAX_NUM_KEYS:
            raise ValueError("too many keys in configuration")

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
                settings=settings
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

        def parse_key(value):
            """Parse an encryption or authentication key value

            Bytes are separated by commas or spaces.

            """
            value = value.replace(",", " ")
            return bytearray.fromhex(value)

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
                        raise ValueError("key'%s' defined multiple times in files: %s" % (key_name, files))

                    key = KeyDesc(authentication=parse_key(cp.get(section, "auth")),
                                  encryption=parse_key(cp.get(section, "encrypt")))
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

        if len(self.keys) is not None:
            for key_name in self.keys.keys():
                key = self.keys[key_name]
                section_name = 'key:{}'.format(key_name)
                config.add_section(section_name)
                config.set(section_name, 'auth', "".join("{:02X} ".format(x) for x in key.authentication))
                config.set(section_name, 'encrypt', "".join("{:02X} ".format(x) for x in key.encryption))

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

    args = parser.parse_args()

    # Create overlay dic
    overlay_dic = {}
    for ol in args.overlay:
        items = ol.split(":")
        overlay_dic[items[0]] = items[1]

    # Create config
    config = BootloaderConfig.from_ini_files(args.in_file, overlay_dic)

    # Check config
    if args.check:
        config.check_config()

    # Write out config if needed
    if args.out_file:
        config.to_ini_file(args.out_file)


# Run main.
if __name__ == "__main__":
    sys.exit(main())