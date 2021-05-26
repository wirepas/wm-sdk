#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# hextoarray32.py - A tool to convert an hex file to an array of 32-bit numbers

import sys
import os
import struct
import hextool

LINE_INDENT = "    "
LINE_LENGTH = 72


# Open hex file and convert it to byte array
memory = hextool.Memory()
hextool.load_intel_hex(memory, filename = sys.argv[1])
# For EFR32 delete range in special registers [0xfe04000-0xfe04200]
del memory[0xfe04000:0xfe04200]
data = memory[memory.min_address:memory.max_address]

# Open outfile and write uint32 array
with open(sys.argv[2], "w") as f:
    line = LINE_INDENT

    for pos in range(0, len(data), 4):
        if pos != 0:
            line += ", "

        if len(line) >= LINE_LENGTH:
            f.write(line)
            f.write("\n")
            line = LINE_INDENT

        line += "0x%08x" % struct.unpack("<L", data[pos : pos + 4])[0]

    f.write(line)
    f.write("\n")
