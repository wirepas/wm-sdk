#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# sdk_wizard.py - A tool to configure initial steps in SDK
#

import sys
import argparse
import random
from Crypto.PublicKey import ECC  # For generating private keys

from bootloader_config import BootloaderConfig, KeyDesc


# Number of bits in symmetric keys
OMAC1_KEY_NUM_BITS = 128
AES128CTR_KEY_NUM_BITS = 128


# Python 2 and Python 3 support
try:
    # Python 2
    input = raw_input
except NameError:
    # Python 3 (raw_input doesn't exist anymore)
    pass

message_about_bootloader_keys = """
********************************************************************************
* WARNING:
*     Wirepas OTAP protocol requires that the software update image, called the
*     "scratchpad", is encrypted and authenticated. The Wirepas bootloader will
*     verify and decrypt the scratchpad before updating.
*
*     This message is displayed, because you don't have any keys present in your
*     environment. Default keys will be generated now. Advanced users may add
*     more keys later, e.g. for multi-vendor networks.
*
*     Scratchpad encryption is based on AES-128 in CTR mode, using a 128-bit
*     key.
*
*     Scratchpad authentication uses either the symmetric OMAC1 / CMAC algorithm
*     with a 128-bit key, or the public key-based ECDSA P-256 algorithm.
*     Currently only Nordic nRF9160 as well as SiLabs EFR32xG23 and xG24 support
*     ECDSA P-256 authentication.
*
*     Be careful! If you lose these keys, you'll not be able to update your
*     network. Also, anyone having access to these keys can generate valid
*     scratchpads, so it is important to not disclose these keys.
********************************************************************************
"""

message_about_storing_keys = """
********************************************************************************
* WARNING:
*     You must store the generated keys in a safe place to be sure that you can
*     always retrieve them. It is also important to not disclose these keys.
********************************************************************************
"""


def ask_user_for_a_key(key_name, required_num_bits = None):
    """Ask user to enter a key

    Args:
        key_name: name of the key

    Return: Key as a byte array
    """
    while True:
        bits_string = required_num_bits and "a %d-bit " % required_num_bits or ""
        key_str = input("\nPlease enter %s%s key as hexadecimal digits (A1 B2 ...):\n" % (bits_string, key_name))

        key_str = key_str.replace(",", " ")
        try:
            key = bytearray.fromhex(key_str)

            if required_num_bits is None:
                # Check that the given ECDSA P-256 key is valid
                KeyDesc(KeyDesc.KEY_TYPE_SHA256_ECDSA_P256_AES128CTR,
                        key,
                        b"\x00" * (AES128CTR_KEY_NUM_BITS / 8))  # Dummy key
        except ValueError:
            print("Format for key %s is not correct" % key_name)
            continue

        if len(key) == 0:
            continue

        if required_num_bits is not None and (len(key) * 8) != required_num_bits:
            print("Key is not %d bits long" % required_num_bits)
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
        key_choice = input('Generate [O]MAC1 / CMAC or [E]CDSA P-256 authentication keys?\n')
        if key_choice in ("O", "E"):
            break

    if key_choice == "O":
        keytype = KeyDesc.KEY_TYPE_OMAC1_AES128CTR
    if key_choice == "E":
        keytype = KeyDesc.KEY_TYPE_SHA256_ECDSA_P256_AES128CTR

    while True:
        choice = input("How do you want to generate your keys? [M]anually, [R]andomly with passphrase?\n")
        if choice in ("R", "M"):
            break

    if choice == "R":
        passphrase = input("Please enter your passphrase?\n")

        # Always generate a symmetric authentication key first, to update
        # random number generator state for generating the encryption key
        auth = generate_random_key(passphrase)
        encryp = generate_random_key()

        if keytype == KeyDesc.KEY_TYPE_SHA256_ECDSA_P256_AES128CTR:
            # Generate a random ECDSA P-256 key
            auth = bytearray(ECC.generate(curve='p256').export_key(format='DER'))
    elif choice == "M":
        if keytype == KeyDesc.KEY_TYPE_SHA256_ECDSA_P256_AES128CTR:
            auth = ask_user_for_a_key('authentication')  # Length varies
        else:
            auth = ask_user_for_a_key('authentication', OMAC1_KEY_NUM_BITS)
        encryp = ask_user_for_a_key('encryption', AES128CTR_KEY_NUM_BITS)

    return keytype, auth, encryp


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
        keytype, auth, encryp = create_bootloader_key_pair()

        # Create a bootloader config
        key_pair = KeyDesc(keytype, auth, encryp)
        key_dic = {"default": key_pair}
        config = BootloaderConfig(None, None, key_dic, None)
        # Hack: erase platform to avoid default one
        config.platform = None

        # Write it to the output file
        config.to_ini_file(args.out_key_file)

        print("\nYou new keys were written to %s" % args.out_key_file)
        print(message_about_storing_keys)


if __name__ == '__main__':
    sys.exit(main())
