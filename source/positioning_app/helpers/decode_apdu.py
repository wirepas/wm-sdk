# Wirepas Oy 2018

import struct
import binascii
import argparse


def cmdline():
    """ Adds argument options. """

    parser = argparse.ArgumentParser(description='Wirepas Positioning App Config Generator',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    parser.add_argument('--hexstring',
                        default=None,
                        type=str,
                        help='hexadecimal string.')

    parser.add_argument('--terminal_log',
                        default=None,
                        type=str,
                        help='reads hextrings from a file.')

    # ignores unknonw arguments
    _args, _ = parser.parse_known_args()

    return _args


def chunker(seq, size):
    """
        Splits a sequence in multiple parts

        Args:
            seq ([]) : an array
            size (int) : length of each array part

        Returns:
            array ([]) : a chunk of SEQ with given SIZE
    """
    return (seq[pos:pos + size] for pos in range(0, len(seq), size))


def decode_hex_str(hex):
    """
        Splits a sequence in multiple parts

        Args:
            seq ([]) : an array
            size (int) : length of each array part

        Returns:
            array ([]) : a chunk of SEQ with given SIZE
    """
    payload = bytes.fromhex(hex.replace(' ', '').strip(' '))
    return decode_bytes(payload)


def decode_bytes(apdu):
    """
        Takes a payload in bytes and decodes its contents

        The contents are assumed to follow the positioning definitions:

        Header:
            message sequence number (2 bytes)

        RSS field:
            type | length | address (3 bytes) | rssi

        Known types:
            RSS : 0
    """

    apdu_len = len(apdu)

    format_header = struct.Struct('<H B B')
    format_meas = struct.Struct('<B B B B')

    # get the first 4 bytes
    header = format_header.unpack(apdu[0:4])
    body = apdu[4:]
    sequence = header[0]
    msg_type = header[1]
    payload_len = header[2]

    if len(body) % 4:
        print('invalid payload {0}'.format(len(body) / 4))
        return None

    count = 0
    measurements = list()
    for chunk in chunker(body, format_meas.size):
        if len(body) < 4:
            continue
        values = format_meas.unpack(chunk)

        addr = 0
        addr = values[0]
        addr = addr | (values[1] << 8)
        addr = addr | (values[2] << 16)
        rss = values[-1] * -0.5
        d = {
            'address': addr,
            'rss': rss,
        }
        measurements.append(d)

    dapdu = dict(sequence=sequence,
                 type=msg_type,
                 length=payload_len,
                 nb_measurements=len(measurements),
                 measurements=measurements)

    return dapdu


if __name__ == '__main__':
    """
        Main loop

        It accepts either a hex string through the terminal or a Wirepas
        terminal log (see example_log attached)

        Command line example:
        $ run example: decode_apdu.py --hexstring '0a 00 00 01  7b 00 00 d3 '

        Wirepas terminal log example:
        $ run decode_apdu.py --terminal_log './example_wirepas_terminal.log'

        The decoded payload will be printed to the terminal
    """
    args = cmdline()

    if args.hexstring:
        hexstring = args.hexstring.replace(
            '0x', ' ').replace(' ', '').strip(' ')
        payload = decode_bytes(bytes.fromhex(hexstring))
        print(payload)

    elif args.terminal_log:
        key = '....{...'
        with open(args.terminal_log, 'r') as file:
            for line in file.readlines():
                if key in line:
                    line = line.split()
                    hexstring = ''.join(line[line.index(key) + 1:])
                    payload = decode_bytes(bytes.fromhex(hexstring))
                    print(payload)
