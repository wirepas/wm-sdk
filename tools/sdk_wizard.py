#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# sdk_wizard.py - A tool to configure initial steps in SDK
#

import sys
import argparse
import random
from bootloader_config import BootloaderConfig, KeyDesc

# Python 2 and Python 3 support
try:
    # Python 2
    input = raw_input
except NameError:
    # Python 3 (raw_input doesn't exist anymore)
    pass

message_about_bootloader_keys = """
***************************************************************************************************
* WARNING:
*     Wirepas OTAP protocol requires that all the images sent to update the network are
*     encrypted and authenticated.
*     Encryption and authentication is based on AES 128bits key.
*     This message is displayed as you don't have keys set in your environment.
*     Let's create a file to store your new custom and PRIVATE keys.
*
*     Be careful, if you loose these keys, you'll not be able to update your newtork as the
*     keys are also written to the bootloader.
*     This tool will help you setup an initial default key pair.
*     Advanced users may define multiple key pairs (like for multi-vendor networks)
***************************************************************************************************
"""

message_about_storing_keys = """
***************************************************************************************************
* WARNING:
*     You must store the generated keys in a safe place to be sure that you can always retrieve it.
***************************************************************************************************
"""


def ask_user_for_a_key(key_name):
    """Ask user to enter a key

    Args:
        key_name: name of the key

    Return: A 128 bits key as a byte array
    """
    while True:
        key_str = input("\nPlease enter %s key as 16 hexadecimal digits (A1 B2 ...) :\n" % key_name)

        key_str = key_str.replace(",", " ")
        try:
            key = bytearray.fromhex(key_str)
        except ValueError:
            print("Format for key %s is not correct" % key_name)
            continue

        if len(key) != 16:
            print("Key is not 128 bits long")
            continue

        return key


def generate_random_key(seed=None):
    """Generate a random key

    Args:
        seed: the seed to initialize the random module

    Return: A 128 bits key as a byte array
    """
    if seed is not None:
        random.seed(seed)
    key = random.getrandbits(128)
    # Convert key to bytearray
    return bytearray.fromhex("%032x" % key)


def create_bootloader_key_pair():
    """Create bootloader key pair

    Ask the user for the method to generate the keys (Manual or Randomly with passphrase)
    """
    while True:
        choice = input("How do you want to generate your keys? [M]anually, [R]andomly with passphrase?\n")
        if choice in ("R", "M"):
            break

    if choice == "R":
        passphrase = input("Please enter your passphrase?\n")
        auth = generate_random_key(passphrase)
        encryp = generate_random_key()
    elif choice == "M":
        auth = ask_user_for_a_key('authentication')
        encryp = ask_user_for_a_key('encryption')

    return auth, encryp


def main():
    """Main program

    This Wizard script allows to setup some initial steps for the SDK usage:
        - keys: create a default key pair if not present yet
    """

    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("--gen_keys",
                        help="Generate keys",
                        action='store_true')

    parser.add_argument("--out_key_file", "-o",
                        help="File to write the key file")

    args = parser.parse_args()

    if args.gen_keys:
        print(message_about_bootloader_keys)
        # Create a key pair
        auth, encryp = create_bootloader_key_pair()

        # Create a bootloader config
        key_pair = KeyDesc(auth, encryp)
        key_dic = {"default": key_pair}
        config = BootloaderConfig(None, None, key_dic, None)

        # Write it to the output file
        config.to_ini_file(args.out_key_file)

        print("\nYou new key pair was correctly written to %s" % args.out_key_file)
        print(message_about_storing_keys)


if __name__ == '__main__':
    sys.exit(main())
