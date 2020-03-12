#!/usr/bin/env python
# -*- coding: utf-8 -*-

# hashfilenames.py - A tool to create file name hashes from source files
#
# Requires: Python v2.7 or newer

import sys
import os
import stat
import re
import argparse


# A list of file name suffixes that will be considered
eligible_suffixes = [".c", ".cpp", ".h", ".s", ".S", ".asm"]

# A list of files that match a given criteria
eligible_files = []


pgmname = os.path.split(sys.argv[0])[-1]


def crc16ccitt(data, initial = 0xffff):
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

def get_filename(path):
    head, tail = os.path.split(path)
    return tail or os.path.basename(head)

def scanfiles(path):
    '''Scan files and directories recursively for eligible files.'''

    global eligible_files
    global eligible_suffixes

    files_and_dirs = os.listdir(path)

    for name in files_and_dirs:
        # NOTE: Do not use os.path.join(), to keep path separators consistent.
        #       Python on Windows supports forward slashes in path names.
        full_path = "%s/%s" % (path, name)

        # Get file type (mode). Do not follow symbolic links.
        mode = os.lstat(full_path).st_mode

        if stat.S_ISDIR(mode):
            # A directory, scan it.
            scanfiles(full_path)
            continue
        elif not stat.S_ISREG(mode):
            # Not a regular file, skip it.
            continue

        # Check file name.
        for suffix in eligible_suffixes:
            if full_path.endswith(suffix):
                # Matching suffix found, stop.
                break
        else:
            # No matching suffix found, skip file.
            continue

        # Read file contents.
        with open(full_path, "r") as f:
            contents = f.read()

        # File is eligible, add it to list.
        # Remove path from file
        source_file_name = get_filename(full_path)
        eligible_files.append(source_file_name)
        # eligible_files.append(full_path)


def main():
    '''Main program'''

    global eligible_files

    # Parse command line arguments.
    parser = argparse.ArgumentParser(
        description = "A tool to create file name hashes from source file names")
    parser.add_argument("--sql", "-s", action = 'store_true',
        help = "output hashes as SQL insert statements")
    args = parser.parse_args()

    # Normalize path and only allow forward slashes.
    # Python on Windows supports forward slashes in path names.
    root = os.path.dirname(os.path.realpath(__file__))
    root = os.path.join(root, "..")
    os.chdir(root)

    # Find all eligible files.
    scanfiles('.')

    if not args.sql:
        # Print hashes and paths of eligible files.
        for name in eligible_files:
            print "0x%04x %s" % (crc16ccitt(name), name)
    else:
        # Print hashes and paths of eligible files as SQL insert statements.
        print "REPLACE INTO file_name_hashes VALUES (0, 'unknown');"
        for name in eligible_files:
            if "'" in name:
                raise ValueError("invalid character in file name: '%s'" % name)
            print "REPLACE INTO file_name_hashes VALUES (%d, '%s');" % (
                crc16ccitt(name), name)

    return 0


# Run main.
if __name__ == "__main__":
    sys.exit(main())
