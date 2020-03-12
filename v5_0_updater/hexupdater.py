#!/usr/bin/env python
# -*- coding: utf-8 -*-

# hexUpdater.py - A tool to prepare an EFR32 stack hex file before v4.0 to
#                 v5.0 upgrade.
#                 The first page of stack code is moved to the end of area, to
#                 leave some space for the updater startup code.
#
# Requires:
#   - Python 2 v2.7 or newer (uses argparse, hextool.py)
#     or Python 3 v3.2 or newer
#   - hextool.py

import sys
import os
import argparse
import textwrap

FIRST_PAGE_ADDRESS = 0x3A800
PAGE_SIZE = 2048

# Import hextool.py, but do not write a .pyc file for it.
dont_write_bytecode = sys.dont_write_bytecode
sys.dont_write_bytecode = True
sys.path.append('tools/')
import hextool
sys.dont_write_bytecode = dont_write_bytecode
del dont_write_bytecode

def main():
    '''Main program'''

    # Determine program name, for error messages.
    pgmname = os.path.split(sys.argv[0])[-1]

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
        "A tool to prepare EFR32 stack hex file for v4.0 to v5.0 upgrade."))

    parser.add_argument("infilespec",
        metavar = "INFILESPEC", help = "stack hex file")
    parser.add_argument("--output", "-o",
        metavar = "OUTFILESPEC",
        help = "output file")

    try:
        args = parser.parse_args()

        memory = hextool.Memory()

        hextool.load_intel_hex(memory, filename = args.infilespec)

        # Move first page off stack just before the v3 bootloader data.
        first_page = memory[memory.min_address:memory.min_address + PAGE_SIZE]
        memory.cursor = FIRST_PAGE_ADDRESS
        memory += first_page
        del memory[memory.min_address:memory.min_address + PAGE_SIZE]

        if args.output is not None:
            # Save output file.
            hextool.save_intel_hex(memory, filename=args.output)
        else:
            hextool.save_intel_hex(memory, filename=args.infilespec)

    except (ValueError, IOError, OSError) as exc:
        sys.stdout.write("%s: %s\n" % (pgmname, exc))

    return 0


# Run main.
if __name__ == "__main__":
    sys.exit(main())
