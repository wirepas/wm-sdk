#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# hextool.py - A tool for loading and saving Intel HEX and binary files
#
# This file is both a standalone tool as well as a module
# used by genscratchpad.py.
#
# See https://en.wikipedia.org/wiki/Intel_HEX for the format description.
#
# Requires:
#   - Python 2 v2.7 or newer (uses argparse and memoryview)
#     or Python 3 v3.2 or newer

import sys
import os
import math
import argparse
import textwrap


# Constants

# Block size for reading and writing binary data
BINARY_BLOCK_NUM_BYTES = (64 * 1024)  # 64 kilobytes

# Maximum Intel HEX line length
MAX_LINE_LENGTH = 256

# Default maximum file size allowed when writing binary
# files and not specifying end address explicitly
DEFAULT_BINARY_MAX_NUM_BYTES = (8 * 1024 * 1024)    # Eight megabytes

# Default maximum number of data bytes per Intel HEX Data record (i.e. a line)
DEFAULT_MAX_RECORD_NUM_BYTES = 16


# Python 2 and Python 3 support

try:
    xrange
except NameError:
    xrange = range


# Objects

class MemoryRange(object):
    '''MemoryRange(start, bytes_) -> new memory range object

    Construct an object for storing contiguous binary data at an address.

    start:  Start address of data
    bytes_: Binary data to store (copied)'''

    def __init__(self, start, bytes_):
        # Make a private copy of the data given.
        #
        # For efficiency reasons, the internal bytearray object may have more
        # bytes preallocated than are in use. self.__num_bytes contains the
        # correct length.
        self.__bytearray = bytearray(bytes_)

        # Create private attributes and use read-only properties to access.
        self.__start = start
        self.__num_bytes = len(bytes_)
        self.__end = self.__start + self.__num_bytes # One past last address

    @property
    def bytes(self):
        '''Contents as a memoryview object'''

        # Return a view to the data, without copying. The internal bytearray
        # object may be larger than the actual number of bytes stored, for
        # efficiency reasons.
        return memoryview(self.__bytearray)[:self.__num_bytes]

    @property
    def start(self):
        '''Address of first byte stored'''
        return self.__start

    @property
    def end(self):
        '''Address after last byte stored'''
        return self.__end

    @property
    def num_bytes(self):
        '''Number of bytes stored'''
        return self.__num_bytes

    def __len__(self):
        '''Return number of bytes stored'''
        return self.__num_bytes

    def __repr__(self):
        '''Return a pretty representation of bytes stored'''
        data = repr(str(self.__bytearray[:min(32, self.__num_bytes)]))[1:-1]
        if self.__num_bytes > 32:
            data = "%s..." % data
        return "%s(%d, b'%s') # %d bytes\n" % (
            type(self).__name__, self.__start, data, self.__num_bytes)

    def __getitem__(self, item):
        '''Return an index or slice as a memoryview object'''

        if isinstance(item, slice):
            # Index is a slice.
            if item.step != None:
                raise TypeError("step not supported")

            if item.start != None:
                start = item.start
            else:
                start = self.__start

            if item.stop != None:
                end = item.stop
            else:
                end = self.__end
        elif isinstance(item, int):
            # Index is an integer.
            start = item
            end = item + 1
        else:
            raise TypeError("index must be int or slice")

        if (not isinstance(start, int) or start < 0 or
            not isinstance(end, int) or end < 0):
            raise TypeError("index must be a nonnegative integer")

        if start < self.__start or end > self.__end or start > end:
            raise IndexError("index out of range")

        start -= self.__start
        end -= self.__start

        # Return a view to the data, without copying.
        return memoryview(self.__bytearray)[start:end]

    def __setitem__(self, item, value):
        '''Replace data at given range or index'''
        mv = self.__getitem__(item)
        mv[:] = value

    def extend(self, bytes_):
        '''Insert new binary data at the end of existing data'''

        new_num_bytes = len(bytes_)

        if len(self.__bytearray) < self.__num_bytes + new_num_bytes:
            # Preallocate bytes, for efficiency.
            prealloc_num_bytes = self.__num_bytes + new_num_bytes
            prealloc_num_bytes = 2 ** int(      # Double the previous size.
                math.ceil(math.log(prealloc_num_bytes) / math.log(2)))
            #print("prealloc %d" % prealloc_num_bytes) # DEBUG
            b = bytearray(prealloc_num_bytes)

            # Copy old data.
            b[:self.__num_bytes] = self.__bytearray[:]
            self.__bytearray = b

        # Insert new data.
        #print("extend %d" % new_num_bytes) # DEBUG
        self.__bytearray[self.__num_bytes:
                         (self.__num_bytes + new_num_bytes)] = bytes_[:]

        # Update end address and total number of bytes.
        self.__end += new_num_bytes
        self.__num_bytes += new_num_bytes

    def trim(self):
        '''Free preallocated bytes'''
        del self.__bytearray[self.__end:]


class Memory(object):
    '''Memory(address_space, overlap_ok, max_num_bytes) -> new memory object

    An object to store discontinuous memory contents

    address_space   Size of address space (i.e. one past last address), in bytes
    overlap_ok      Allow or disallow overwriting existing data
    gap_fill_byte   Fill byte value for gaps, when iterating or slicing'''

    def __init__(self, address_space = 2 ** 32, overlap_ok = False,
                 gap_fill_byte = 0x00):
        # Initialize private attributes.
        self.__top_address = address_space
        self.__memory_ranges = []   # Ordered list of MemoryRange objects
        self.__gap_fill_byte = gap_fill_byte

        # Initialize cursor.
        self.cursor = 0

        # Allow or disallow overwriting existing data.
        self.overlap_ok = overlap_ok    # Can add over already present data?

        # Invalidate cached value of max_gap.
        self.__max_gap = None

        # Invalidate cached value of num_bytes.
        self.__num_bytes = None

    def __getitem__(self, item):
        '''Return data in a given address or range as a bytearray object'''

        # Check parameters.
        if isinstance(item, slice):
            # Index is a slice.
            if item.step != None:
                raise TypeError("step not supported")

            start = item.start
            end = item.stop

            if not isinstance(start, int) or not isinstance(end, int):
                raise TypeError("index must be a nonnegative integer")

            if start > end:
                # Zero-length range
                end = start
        elif isinstance(item, int):
            # Index is an integer.
            start = item
            end = item + 1
        else:
            raise TypeError("index must be int or slice")

        if (start < 0 or end < 0 or
            start > self.__top_address or end > self.__top_address):
            raise IndexError("range outside of address space")

        # Create a new, preallocated, prefilled bytearray object for data.
        num_bytes = end - start
        bytes_ = bytearray([self.__gap_fill_byte]) * num_bytes

        # Copy data from memory ranges to the bytearray.
        for mr in self.__memory_ranges:
            if start >= mr.end:
                # This range is before the range requested, skip it.
                continue
            elif end <= mr.start:
                # No more ranges to consider, stop.
                break

            # Determine read position and length in the memory range.
            read_start = max(start, mr.start)
            read_end = min(end, mr.end)

            # Determine write position and length in the bytearray.
            write_start = read_start - start
            write_end = write_start + (read_end - read_start)

            # Copy memory range data to the bytearray.
            bytes_[write_start:write_end] = mr[read_start:read_end]

        return bytes_

    def __setitem__(self, item, value):
        '''Add a block of data to a given range, or a byte to a given address'''

        # Check parameters.
        if isinstance(item, slice):
            # Index is a slice.
            if item.step != None:
                raise TypeError("step not supported")

            if item.start != None:
                start = item.start
            else:
                start = 0

            if item.stop != None:
                end = item.stop
            else:
                end = self.__top_address

            if not isinstance(start, int) or not isinstance(end, int):
                raise TypeError("index must be a nonnegative integer")

            if start > end:
                # Zero-length range
                end = start
        elif isinstance(item, int):
            # Index is an integer.
            start = item
            end = item + 1
        else:
            raise TypeError("index must be int or slice")

        if isinstance(value, int):
            # Convert single int to a bytearray.
            value = bytearray([value])
        elif not isinstance(value, (bytes, bytearray, memoryview)):
            raise TypeError("an integer or a bytes, bytearray or "
                            "memoryview object is required")

        if (start < 0 or end < 0 or
            start > self.__top_address or end > self.__top_address):
            raise IndexError("range outside of address space")
        elif len(value) != (end - start):
            raise TypeError("range must match size of data")

        if len(value) == 0:
            # No data, nothing to do.
            return

        # Create a new MemoryRange object for data.
        new_mr = MemoryRange(start, value)

        new_ranges = []

        # Find a spot where to add new data.
        n = 0
        while True:
            if n == len(self.__memory_ranges):
                # No more ranges, append new range and stop.
                new_ranges.append(new_mr)
                break

            this_mr = self.__memory_ranges[n]

            if new_mr.end < this_mr.start:
                # New range is completely before this range, append it and stop.
                new_ranges.append(new_mr)
                break
            elif new_mr.end == this_mr.start:
                # New range adjoins in front of this range. Join ranges.
                new_mr.extend(this_mr.bytes)
            elif new_mr.start == this_mr.end:
                # New range adjoins at the end of this range. Join ranges.
                this_mr.extend(new_mr.bytes)
                new_mr = this_mr
            elif new_mr.start > this_mr.end:
                # New range is completely after this
                # range, use this range as-is.
                new_ranges.append(this_mr)
            else:
                # New range overlaps this range.
                if not self.__overlap_ok:
                    raise ValueError("overlapping data not allowed")

                left = new_mr.start - this_mr.start
                right = this_mr.end - new_mr.end

                if left <= 0 and right <= 0:
                    # New range covers this range completely. Forget this range.
                    pass
                elif left <= 0 and right > 0:
                    # New range starts before or at the same position as
                    # this range. Add tail of this range to the new range.
                    new_mr.extend(this_mr.bytes[(this_mr.num_bytes - right):])
                elif left > 0 and right <= 0:
                    # New range starts after this range and runs past the
                    # end of this range. First, replace tail of this range
                    # with data from the new range and then add remaining
                    # data from the new range to this range.
                    this_mr.bytes[left:] = (
                        new_mr.bytes[:(this_mr.num_bytes - left)])
                    this_mr.extend(new_mr.bytes[(this_mr.num_bytes - left):])
                    new_mr = this_mr
                else:
                    # New range is completely within this range. Replace
                    # data in this range with data from the new range.
                    this_mr.bytes[left:(this_mr.num_bytes - right)] = (
                        new_mr.bytes)
                    new_mr = this_mr

            # Process next range.
            n += 1

        # Copy remaining ranges as is.
        new_ranges.extend(self.__memory_ranges[n:])

        # Invalidate cached value of max_gap.
        self.__max_gap = None

        # Invalidate cached value of num_bytes.
        self.__num_bytes = None

        # Update ranges.
        self.__memory_ranges = new_ranges

        # Update cursor position, for __add__().
        self.cursor = end

    def __delitem__(self, item):
        '''Delete data from an address range'''

        # Check if item is a slice or an index.
        if isinstance(item, slice):
            if item.step != None:
                raise TypeError("step not supported in del")

            if item.start != None:
                if not isinstance(item.start, int) or item.start < 0:
                    raise TypeError(
                        "slice indices must be nonnegative integers")
                start = item.start
            else:
                start = 0

            if item.stop != None:
                if not isinstance(item.stop, int) or item.stop < 0:
                    raise TypeError(
                        "slice indices must be nonnegative integers")
                end = item.stop
            else:
                end = self.__top_address
        elif isinstance(item, int):
            start = item
            end = item + 1
        else:
            raise TypeError("index must be int or slice")

        # Check parameters.
        if start >= end:
            # Nothing to delete.
            return
        elif start >= self.__top_address or end > self.__top_address:
            raise ValueError("slice indice outside of address space")

        new_ranges = []

        # Delete given range from all ranges.
        n = 0
        while True:
            if n == len(self.__memory_ranges):
                # No more ranges, stop.
                break

            this_mr = self.__memory_ranges[n]

            if  end <= this_mr.start:
                # Range to be deleted is completely before this range, stop.
                break
            elif start >= this_mr.end:
                # Range to be deleted is completely after
                # this range, use this range as-is.
                new_ranges.append(this_mr)
            elif start <= this_mr.start and end >= this_mr.end:
                # This range is completely inside the range
                # to be deleted. Forget this range.
                pass
            else:
                # This range may need to be split in two.
                if start > this_mr.start:
                    # Keep data in the beginning of this range.
                    left = start - this_mr.start
                    new_ranges.append(MemoryRange(this_mr.start,
                                                  this_mr.bytes[:left]))

                if end < this_mr.end:
                    # Keep data at the end of this range.
                    right = this_mr.num_bytes - (this_mr.end - end)
                    new_ranges.append(MemoryRange(end,
                                                  this_mr.bytes[right:]))

            # Process next range.
            n += 1

        # Copy remaining ranges as is.
        new_ranges.extend(self.__memory_ranges[n:])

        # Invalidate cached value of max_gap.
        self.__max_gap = None

        # Invalidate cached value of num_bytes.
        self.__num_bytes = None

        # Update ranges.
        self.__memory_ranges = new_ranges

    def __add__(self, value):
        '''Add bytes, bytearray or another Memory object to this object'''

        if isinstance(value, type(self)):
            # Copy all ranges from the given Memory object.
            # Does not update cursor position.
            for mr in value.memory_ranges():
                self[mr.start:mr.end] = mr.bytes

            # Get rid of preallocated bytes in internal MemoryRange objects.
            self.trim()
        elif isinstance(value, (bytes, bytearray, memoryview)):
            # Copy bytes to current cursor position and update cursor position.
            self[self.__cursor:(self.__cursor + len(value))] = value
        elif isinstance(value, int):
            # Add a single byte to cursor position.
            self[self.__cursor] = value
        else:
            raise TypeError("unsupported operand type for +")

        return self

    def __iter__(self):
        '''Iterate memory as bytes, starting from cursor position

        Gaps return the value of gap_fill_byte. Behavior is undefined
        if data is added while the iterator is running.'''

        # TODO: Iterator
        def iterator():
            pass

        return iterator

    def __repr__(self):
        '''Return a pretty representation of bytes stored'''
        return "%s() # %d ranges, %d bytes\n" % (
            type(self).__name__, len(self.__memory_ranges), self.num_bytes)

    def memory_ranges(self):
        '''Return an iterator of MemoryRange objects'''
        return iter(self.__memory_ranges)

    @property
    def cursor(self):
        '''Cursor position for __add__() and __iter__()'''
        return self.__cursor

    @cursor.setter
    def cursor(self, address):
        # Set cursor position, for __add__() and __iter__().
        self.__cursor = address

    @property
    def overlap_ok(self):
        '''Allow or disallow writing over existing data'''
        return self.__overlap_ok

    @overlap_ok.setter
    def overlap_ok(self, overlap_ok):
        self.__overlap_ok = overlap_ok  # Can add over already present data?

    @property
    def gap_fill_byte(self):
        '''Fill byte value for gaps, when iterating or slicing'''
        return self.__gap_fill_byte

    @gap_fill_byte.setter
    def gap_fill_byte(self, gap_fill_byte):
        self.__gap_fill_byte = gap_fill_byte

    @property
    def min_address(self):
        '''Address of first byte stored'''
        if len(self.__memory_ranges) == 0:
            return None
        return self.__memory_ranges[0].start

    @property
    def max_address(self):
        '''Address after last byte stored'''
        if len(self.__memory_ranges) == 0:
            return None
        return self.__memory_ranges[-1].end

    @property
    def max_gap(self):
        '''Maximum gap of consecutive memory ranges, in bytes'''

        if self.__max_gap == None:
            # No value calculated, calculate it.
            max_gap = 0

            # Calculate maximum gap between memory ranges.
            for n in range(len(self.__memory_ranges) - 1):
                this_mr = self.__memory_ranges[n]
                next_mr = self.__memory_ranges[n + 1]

                gap = next_mr.start - this_mr.end
                if gap > max_gap:
                    max_gap = gap

            self.__max_gap = max_gap

        return self.__max_gap

    @property
    def num_bytes(self):
        '''Total number of bytes stored, not counting gaps'''

        if self.__num_bytes == None:
            # No value calculated, calculate number of bytes.
            self.__num_bytes = sum(map(len, self.__memory_ranges))

        return self.__num_bytes

    @property
    def num_ranges(self):
        '''Number of memory ranges stored'''
        return len(self.__memory_ranges)

    def trim(self):
        '''Free preallocated memory in internal buffers'''
        for mr in self.__memory_ranges:
            mr.trim()


# Functions

def _open_file(fileobj, filename, flags):
    '''Return the already opened file when given, open a file when not'''

    # Does the file need to be closed afterwards?
    close = False

    if fileobj == None:
        if filename == None:
            raise ValueError("no file object or filename given")

        # No file object given, so need to open (and close) a named file.
        fileobj = open(filename, flags)
        close = True

    return fileobj, close


def _calc_intel_hex_checksum(bytes_):
    '''Calculate an Intel HEX checksum'''

    return ((sum(bytes_) ^ 0xff) + 1) & 0xff


def _parse_intel_hex_record(memory, offset, address_base, line):
    '''Parse an Intel HEX record, i.e. a line of text'''

    if len(line) == 0 or line[0] != ":":
        return ValueError("invalid record start code")

    # Remove start code, trailing whitespace and newline.
    line = line[1:].rstrip()

    try:
        # Convert hexadecimal character to bytes. Could also use
        # bytearray.fromhex() here, but it permits spaces between
        # numbers, which is not correct for proper Intel HEX records.
        data = bytearray([int(line[n:(n+2)], 16) for n
                          in range(0, len(line), 2)])
    except ValueError:
        raise ValueError("invalid characters in record data")

    # Verify length.
    if len(data) < (4 + 1) or (4 + data[0] + 1) != len(data):
        raise ValueError("invalid record length")

    # Verify checksum.
    if _calc_intel_hex_checksum(data) != 0x00:
        raise ValueError("invalid record checksum")

    record_type = data[3]
    address = address_base | (data[1] << 8) | data[2]
    data = data[4:-1]

    # Apply address offset to read data.
    address += offset

    # Hande various record types.
    if record_type == 0x00:
        # Data record
        memory[address:(address + len(data))] = data
    elif record_type == 0x01:
        # End Of File record, let the caller know that there is no more records.
        address_base = None
    elif record_type == 0x02:
        # Extended Segment Address record (i.e. address bits 20:4)
        if len(data) != 2:
            raise ValueError("invalid extended segment address record")
        address_base = ((data[0] << 8) | data[1]) << 4
    elif record_type == 0x04:
        # Extended Linear Address record (i.e. address bits 32:16)
        if len(data) != 2:
            raise ValueError("invalid linear segment address record")
        address_base = ((data[0] << 8) | data[1]) << 16
    elif record_type in (0x03, 0x05):
        # Start Segment Address or Start Linear Address record, ignore.
        pass
    else:
        # Unknown record type
        raise ValueError("unknown record type 0x%02x" % record_type)

    return address_base


def _generate_intel_hex_record(address, record_type, data):
    '''Generate an Intel HEX record, i.e. a line of text'''

    record = bytearray(4 + len(data) + 1)
    record[0] = len(data)                           # Data length in bytes
    record[1] = (address >> 8) & 0xff               # Address MSB
    record[2] = address & 0xff                      # Address LSB
    record[3] = record_type                         # Record type
    record[4:(4 + len(data))] = data                # Data bytes
    record[-1] = _calc_intel_hex_checksum(record)   # Checksum

    # Convert record to a line of text. Use a carriage return,
    # line feed pair explicitly, for maximum compatiblity.
    line = ":%s\r\n" % "".join(["%02x" % b for b in record])

    # Convert to bytes, to be able to write record to a binary file on
    # Python 3. A binary file is used to keep explicit carriage return,
    # line feed pair intact.
    return line.encode("iso-8859-1", "ignore")  # No-op in Python 2


def _write_memory_range_records(fileobj, mr, address_bits, max_record_len):
    '''Write memory range as Intel HEX records'''

    address = mr.start
    write_address = True

    while address < mr.end:
        if write_address:
            # Write an address record.
            # 20-bit Extended Segment Address records (0x02) are not supported.
            if address_bits == 16:
                # 16-bit addresses, no address record needs to be written.
                pass
            elif address_bits == 32:
                # 32-bit addresses, write an
                # Extended Linear Address record (0x04).
                data = [(address >> 24) & 0xff, (address >> 16) & 0xff]
                record = _generate_intel_hex_record(0x0000, 0x04, data)
                fileobj.write(record)
            else:
                raise ValueError("invalid address_bits")

            write_address = False

        # Determine record length. For maximum compatibility,
        # records are not allowed to pass a 64 kB boundary.
        record_len = min(max_record_len,
                         2 ** 16 - (address & 0xffff),
                         mr.end - address)
        if (address + record_len) & 0xffff == 0x0000:
            # Address wrapped around, force writing new address record.
            write_address = True

        # Write a Data record (0x00).
        data = mr[address:(address + record_len)]
        record = _generate_intel_hex_record(address & 0xffff, 0x00, data)
        fileobj.write(record)

        # Move to next address.
        address += record_len


def load_binary(memory, fileobj = None, filename = None,
                offset = 0, overlap_ok = False):
    '''Load contents of a binary file to a Memory object

    Either an open file object (opened for reading in binary mode)
    or a filename can be given.

    This function modifies the cursor position and overlap_ok
    attributes of the Memory object.

    memory      Memory object to read data into
    fileobj     A file object for reading
    filename    A filename for reading
    offset      Offset to add to all addresses in file
    overlap_ok  Allow or disallow overwriting existing data'''

    fileobj, close = _open_file(fileobj, filename, "rb")

    # Allow or disallow overwriting existing data.
    memory.overlap_ok = overlap_ok

    # Place cursor at given address.
    memory.cursor = offset

    try:
        # Read binary file in blocks.
        while True:
            b = fileobj.read(BINARY_BLOCK_NUM_BYTES)
            if len(b) == 0:
                # End of file reached.
                break
            memory += b
    finally:
        if close:
            fileobj.close()


def load_intel_hex(memory, fileobj = None, filename = None,
                   offset = 0, overlap_ok = False):
    '''Load contents of an Intel MCS-86 Object ("Intel HEX") format file
    to a Memory object

    Either an open file object (opened for reading in text mode)
    or a filename can be given.

    This function modifies the cursor position and overlap_ok
    attributes of the Memory object.

    memory      Memory object to read data into
    fileobj     A file object for reading
    filename    A filename for reading
    offset      Offset to add to all addresses in file
    overlap_ok  Allow or disallow overwriting existing data'''

    fileobj, close = _open_file(fileobj, filename, "r")

    memory.overlap_ok = overlap_ok

    try:
        # Last seen extended address record,
        # for 20-bit and 32-bit Intel HEX files
        address_base = 0

        # Read Intel HEX records.
        while address_base != None:
            line = fileobj.readline(MAX_LINE_LENGTH)
            if line == "":
                break
            address_base = _parse_intel_hex_record(memory, offset,
                                                   address_base, line)

        # Get rid of preallocated memory in internal buffers.
        memory.trim()
    finally:
        if close:
            fileobj.close()


def save_binary(memory, fileobj = None, filename = None,
                start_address = None, end_address = None,
                fill_byte = "\xff",
                max_num_bytes = DEFAULT_BINARY_MAX_NUM_BYTES,
                discard_ok = False):
    '''Save Memory object as a binary file

    Either an open file object (opened for writing in binary mode)
    or a filename can be given.

    memory          A Memory object to save
    fileobj         A file object for writing (opened in binary mode)
    filename        A filename for writing
    start_address   Address of first byte to write, None to auto-detect
    end_address     Address past last byte to write, None to auto-detect
    fill_byte       A bytes object of length 1 to use for filling gaps
    max_num_bytes   Maximum file size in bytes, if end_address is None
    discard_ok      Allow discarding data outside given address range'''

    # Use minimum address as the start address, if none given.
    if start_address == None:
        start_address = memory.min_address

    # Use maximum address as the end address, if none given.
    if end_address == None:
        end_address = memory.max_address
        num_bytes = end_address - start_address
        if num_bytes > max_num_bytes:
            raise ValueError("binary file would be too large: "
                             "%d bytes > %d bytes" % (num_bytes, max_num_bytes))

    if not discard_ok:
        # Check that there is no data before the start address.
        if memory.min_address < start_address:
            raise ValueError("data present before start address: "
                             "0x%04x < 0x%04x" % (memory.min_address,
                                                  start_address))

        # Check that there is no data after the end address.
        if memory.max_address > end_address:
            raise ValueError("data present after end address: "
                             "0x%04x > 0x%04x" % (memory.max_address,
                                                  end_address))

    if (not isinstance(fill_byte, (bytes, bytearray, memoryview)) or
        len(fill_byte) != 1):
        raise ValueError("invalid fill byte")

    fileobj, close = _open_file(fileobj, filename, "wb")

    try:
        for write_start in xrange(start_address, end_address,
                                  BINARY_BLOCK_NUM_BYTES):
            write_end = min(end_address, write_start + BINARY_BLOCK_NUM_BYTES)
            fileobj.write(memory[write_start:write_end])
    finally:
        if close:
            fileobj.close()


def save_intel_hex(memory, fileobj = None, filename = None,
                   max_record_len = DEFAULT_MAX_RECORD_NUM_BYTES):
    '''Save Memory object as an Intel MCS-86 Object ("Intel HEX") format file

    Either an open file object (opened for writing in binary mode)
    or a filename can be given.

    memory          A Memory object to save
    fileobj         A file object for writing (opened in binary mode)
    filename        A filename for writing
    max_record_len  Maximum number of bytes per data record (i.e. line)'''

    # Open file as binary, to be able to save correct line endings
    # (carriage return, line feed) correctly on all platforms.
    fileobj, close = _open_file(fileobj, filename, "wb")

    # Determine address number of bits, based on last address.
    # 20-bit Extended Segment Address records (0x02) are not supported.
    if memory.max_address == None or memory.max_address <= 2 ** 16:
        address_bits = 16   # 16-bit addresses
    else:
        address_bits = 32   # 32-bit addresses

    try:
        # No previous records, force writing an address record.
        last_end = None

        # Write memory ranges as Intel HEX records.
        for mr in memory.memory_ranges():
            _write_memory_range_records(fileobj, mr, address_bits,
                                        max_record_len)

        # Write an End Of File record (0x01).
        record = _generate_intel_hex_record(0x0000, 0x01, [])
        fileobj.write(record)
    finally:
        if close:
            fileobj.close()


def load_file(memory, filespec, overlap_ok):
    '''Load contents of a file to a Memory object, according to filespec'''

    if ":" in filespec:
        # Separate parameters and filename.
        param, filename = filespec.split(":", 1)
        param = param.lower()
    else:
        # No parameters, assume a hex file.
        filename = filespec
        param = ""

    # Default start (load) address or address offset is zero.
    offset = 0

    try:
        if param == "":
            # Default type is hex.
            file_is_hex = True
        elif param.startswith("hex"):
            if "@" in param:
                # Parse address offset for Intel HEX file.
                param, addr = param.split("@", 1)
                offset = int(addr, 0)

            # Sanity check.
            if param != "hex":
                raise ValueError

            file_is_hex = True
        elif param.startswith("bin"):
            if "@" in param:
                # Parse start (load) address of binary file.
                param, addr = param.split("@", 1)
                offset = int(addr, 0)

            # Sanity check.
            if param != "bin":
                raise ValueError

            file_is_hex = False
        else:
            raise ValueError
    except ValueError:
        raise ValueError("invalid filespec: '%s'" % filespec)

    if file_is_hex:
        load_intel_hex(memory, filename = filename, offset = offset,
                       overlap_ok = overlap_ok)
    else:
        load_binary(memory, filename = filename, offset = offset,
                    overlap_ok = overlap_ok)


def save_file(memory, filespec, fill_byte):
    '''Save Memory object to a file, according to filespec'''

    if ":" in filespec:
        # Separate parameters and filename.
        param, filename = filespec.split(":", 1)
        param = param.lower()
    else:
        # No parameters, assume a hex file.
        filename = filespec
        param = ""

    try:
        if param == "" or param == "hex":
            # Hex files have no parameters.
            file_is_hex = True
        elif param.startswith("bin"):
            # Default is to save everything.
            start_address = memory.min_address
            end_address = memory.max_address

            # Parse start and end addresses.
            if "@" in param:
                param, addr = param.split("@")
                addr = addr.split("-")
                if addr[0] != "":
                    start_address = int(addr[0], 0)
                if addr[1] != "":
                    end_address = int(addr[1], 0)

            # Sanity check.
            if param != "bin":
                raise ValueError

            # Convert gap fill byte to a one-byte bytearray, for save_binary().
            fill_byte = bytearray([fill_byte])

            file_is_hex = False
        else:
            raise ValueError
    except ValueError:
        raise ValueError("invalid filespec: '%s'" % filespec)

    if file_is_hex:
        save_intel_hex(memory, filename = filename)
    else:
        save_binary(memory, filename = filename, start_address = start_address,
                    end_address = end_address, fill_byte = fill_byte,
                    discard_ok = True)


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
            "A tool for loading and saving Intel MCS-86 Object "
            "(\"Intel HEX\") and binary files", help_width),
        epilog =
            "values:\n"
            "  outfilespec     [outparam:]filename\n"
            "  outparam        hex (the default)\n"
            "                  bin[@[start_address]-[end_address]]\n"
            "  infilespec      [inparam:]filename\n"
            "  inparam         hex[@address_offset] (the default)\n"
            "                  bin[@start_address]\n"
            "  start_address   First address to save to a binary file,\n"
            "                  start of data by default, or address to\n"
            "                  load binary data, zero by default\n"
            "  end_address     Address after last saved byte in a\n"
            "                  binary file, end of data by default\n"
            "  address_offset  Offset to add to all addresses when\n"
            "                  reading a hex file, may be negative,\n"
            "                  zero by default\n"
            "\n"
            "  0x can be used to denote hexadecimal values, default is decimal")

    parser.add_argument("infilespec",
        metavar = "INFILESPEC", nargs = "+",
        help = "input file(s), see below")
    parser.add_argument("--output", "-o",
        metavar = "OUTFILESPEC",
        help = "output file, see below")
    parser.add_argument("--gapfill", "-g",
        metavar = "VALUE", default = "0x00",
        help = "a byte value to use to fill gaps in a binary file "
               "(default: %(default)s)")
    parser.add_argument("--delete", "-d",
        metavar = "RANGE", action = "append",
        help = "delete an address range, where RANGE is start-end, start "
               "is the first address deleted, end is the first address that "
               "is not deleted")
    # TODO: Fill
    #parser.add_argument("--fill", "-f",
    #    metavar = "FILLSPEC", action = "append",
    #    help = "fill a range of addresses with a repeating pattern, where "
    #           "PATTERN is start-end[=pattern], start is the starting address, "
    #           "end is the first address that is not filled, pattern is one or "
    #           "more bytes separated by spaces or commas, default is zero")
    parser.add_argument("--information", "-i",
        action = "store_true",
        help = "show information about memory contents")
    parser.add_argument("--overlap", "-p",
        action = "store_true",
        help = "allow overlapping data")

    try:
        args = parser.parse_args()

        # Parse gap fill option.
        try:
            fill_byte = int(args.gapfill, 0)
            if fill_byte < 0x00 or fill_byte > 0xff:
                raise ValueError
        except ValueError:
            raise ValueError("invalid gapfill value: '%s'" % args.gapfill)

        memory = Memory(overlap_ok = False)

        # Read input file or files.
        for infilespec in args.infilespec:
            load_file(memory, infilespec, args.overlap)

        # Delete memory ranges.
        if args.delete != None:
            for rng in args.delete:
                try:
                    start_address, end_address = rng.split("-")
                    start_address = int(start_address, 0)
                    end_address = int(end_address, 0)
                except (ValueError, IndexError):
                    raise ValueError("invalid address range: '%s'" % rng)

                del memory[start_address:end_address]

        # TODO: Fill memory ranges.

        if args.information:
            # Calculate address width for formatting.
            if memory.num_ranges > 0:
                mr = list(memory.memory_ranges())[-1]
                addr_width = min(4, len("%x" % mr.end))

            # Show information about memory ranges.
            for mr in memory.memory_ranges():
                print("0x%0*x - 0x%0*x: %d bytes" %
                      (addr_width, mr.start, addr_width, mr.end, mr.num_bytes))

        if args.output != None:
            # Save output file.
            save_file(memory, args.output, fill_byte)
    except (ValueError, IOError, OSError) as exc:
        sys.stdout.write("%s: %s\n" % (pgmname, exc))

    return 0


# Run main.
if __name__ == "__main__":
    sys.exit(main())
