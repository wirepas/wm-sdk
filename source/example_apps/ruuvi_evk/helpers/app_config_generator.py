# Wirepas Oy 2019

import yaml
import struct
import binascii
import argparse


class Command(object):
    """
    Command

    This class takes a command representation and build a hexadecimal and
    binary representation of itself.

    It assumes the command format is passed according to the struct's
    module definition in:

    https://docs.python.org/3.5/library/struct.html
    """

    def __init__(self, name, definition):
        super(Command, self).__init__()
        self.name = name
        self.type = definition['type']
        self.length = definition['length']
        self.fields = definition['fields']
        self.format = definition['format']
        self._bin = None
        self._hex = None
        self.build()

    def build(self):
        """
        Builds itself according to the parameters defined in the YAML file.

        This method updates two attributes:
            self._bin: binary representation
            self._hex: hexadecimal representation

        Returns:
            command(str): hexadecimal representation as string.
        """
        payload = [self.type, self.length]
        for k, v in self.fields.items():
            payload.append(v)

        self._bin = struct.pack(self.format, *payload)
        self._hex = binascii.b2a_hex(self._bin)

        return self._hex.decode()

    def rebuild(self):
        """ alias for build """
        return self.build()

    def hex(self):
        """ returns its hex representation as a str"""
        return self._hex.decode()

    def bin(self):
        """ returns its binary representation"""
        return self._bin

    def __str__(self):
        """ defines how it prints itself"""
        return '{0}'.format(self.hex())


class AppConfig(object):
    """
    AppConfig

    Reads a yaml file with the command definition in use with the
    positioning application.

    The yaml file contains a list of all commands prepended with the
    version.

    version: 1
    commands:
        enable_sensors:
          type: 1
          length: 6
          fields:
            temperature_enable: 1
            humidity_enable: 1
            presure_enable: 1
            accel_x_enable: 1
            accel_x_enable: 1
            accel_x_enable: 1
          format: '<BBBBBBBB'

        update_rate:
          type: 2
          length: 2
          fields:
            interval: 60
          format: '<BBH'

    The version number can be passed on the terminal to allow an
    assertion that you are indeed providing the expected format.

    By default the current version is 1.

    This class handles the creation of overhaul commands and provides
    methods to generate specific appconfig payloads.
    """

    def __init__(self, filepath, check_version=1):

        super(AppConfig, self).__init__()

        with open(filepath, 'r') as stream:
            obj = yaml.safe_load(stream)
            if check_version == obj['version']:
                self.version = obj['version']
                self.commands = obj['commands']
                try:
                    self.device_classes = obj['device_classes']
                except KeyError:
                    self.device_classes = None
            else:
                raise KeyError("version mismatch in YAML.")

    def get_command(self, name):
        """
        Returns a Command object.

        Args:
            name(str): command name

        Returns:
            command(obj): a built Command object
        """
        return Command(name, self.commands[name])

    def appconfig(self, device_class=None, names=None):
        """
        Returns an appconfig payload matching for the desired set of commands.

        Args:
            names([str]): list os commands(if none, all known are taken)

        Returns:
            command(obj): a
        """
        if names is None:
            names = self.commands

        if device_class is None:
            obj = ''
        else:
            try:
                obj = '{0:x} '.format(self.device_classes[
                                      device_class.upper()])
            except KeyError:
                obj = '{0:x} '.format(self.device_classes[
                                      device_class.lower()])

        for name in names:
            command = Command(name, self.commands[name])
            obj = obj + \
                '{0} : {1} => {2} \n\n'.format(
                    name, command.fields, str(command))
        obj.strip()
        return obj


def cmdline():
    """ Adds argument options. """

    parser = argparse.ArgumentParser(description='Wirepas Positioning App Config Generator',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('--filepath',
                        default='appconfig.yaml',
                        action='store',
                        type=str,
                        help='yaml file with command definition.')

    parser.add_argument('--version',
                        default=1,
                        action='store',
                        type=int,
                        help='force a check against the file version.')

    # ignores unknonw arguments
    _args, _ = parser.parse_known_args()

    return _args


def example():
    args = cmdline()
    appconfig = AppConfig(filepath=args.filepath, check_version=args.version)

    print('Wirepas Oy 2018\n')
    print('Appconfig generation helper - set your values on the yaml.\n')
    print('>> Appconfig to set parameters AppConfig commands ({0})'
          ' for default device class\n'.format(args.filepath))
    print('{0}\n'.format(appconfig.appconfig()))


if __name__ == '__main__':
    """ executes main. """
    example()
