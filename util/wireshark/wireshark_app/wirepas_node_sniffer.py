#!/usr/bin/env python3

import sys
import os

is_standalone = __name__ == '__main__'

import queue as Queue

if is_standalone:
    sys.path.insert(0, os.getcwd())

import signal
import struct
import threading
import time
import logging
from argparse import ArgumentParser
from serial import Serial, serialutil
from serial.tools.list_ports import comports

# List of USB PID/VID that can be used to connect to a Wirepas node
# This list can be extended if another serial to USB converter is used
USB_PID_VID = [(0x0403,0x6001),
               (0x0403,0x6015),
               (0x1366,0x0105),
               (0x1366,0x1015),
               (0x1366,0x1054),
               (0x1366,0x1055)]

class WirepasNodeSniffer(object):

    def __init__(self, connection_open_timeout=None):
        self.serial = None
        self.serial_queue = Queue.Queue()
        self.running = threading.Event()
        self.logger = logging.getLogger()

        self.dev = None
        self.channel = None
        self.dlt = None
        self.threads = []
        self.connection_open_timeout = connection_open_timeout


    def stop_sig_handler(self, *args, **kwargs):
        """
        Function responsible for closing the connection
        """
        self.logger.info("in stop_sig_handler")

        if self.running.is_set():
            self.running.clear()

            for thread in self.threads:
                try:
                    thread.join(timeout=10)
                    if thread.is_alive() is True:
                        self.logger.error("Failed to stop thread {}".format(thread.name))
                except RuntimeError:
                    pass

    @staticmethod
    def extcap_interfaces():
        """
        Wireshark-related method that returns configuration options.
        :return: string with wireshark-compatible information
        """
        res = []
        res.append("extcap {version=0.1}{display=Wirepas Node Sniffer}")
        for port in comports():
            if (port.vid, port.pid) in USB_PID_VID:
                res.append("interface {value=%s}{display=Wirepas Node Sniffer %s}" % (port.device,port.device) )

        return "\n".join(res)

    @staticmethod
    def extcap_dlts():
        """
        Wireshark-related method that returns configuration options.
        :return: string with wireshark-compatible information
        """
        res = []
        res.append("dlt {number=147}{name=USER0}{display=Wirepas Node Sniffer")

        return "\n".join(res)

    @staticmethod
    def extcap_config():
        """
        Wireshark-related method that returns configuration options.
        :return: string with wireshark-compatible information
        """
        return "\n"

    def pcap_header(self):
        """
        Returns pcap header to be written into pcap file.
        """
        # More details here: https://wiki.wireshark.org/Development/LibpcapFileFormat#global-header
        header = bytearray()
        header += struct.pack('<L', int ('a1b2c3d4', 16 ))
        header += struct.pack('<H', 2 ) # Pcap Major Version
        header += struct.pack('<H', 4 ) # Pcap Minor Version
        header += struct.pack('<I', int(0)) # Timezone
        header += struct.pack('<I', int(0)) # Accurancy of timestamps
        header += struct.pack('<L', int ('000000ff', 16 )) # Max Length of capture frame
        header += struct.pack('<L', self.dlt) # DLT
        return header

    @staticmethod
    def pcap_packet(data, timestamp):
        """
        Creates pcap packet to be saved in pcap file.
        """
        pcap = bytearray()

        caplength = len(data)

        # Pcap header from https://wiki.wireshark.org/Development/LibpcapFileFormat#record-packet-header
        pcap += struct.pack('<L', timestamp // 1000000 ) # Timestamp seconds
        pcap += struct.pack('<L', timestamp % 1000000 ) # Timestamp microseconds
        pcap += struct.pack('<L', caplength ) # Length captured
        pcap += struct.pack('<L', caplength ) # Length in frame
        # Add raw data as received on uart
        pcap += data

        return pcap

    def _read_slip_frame(self):

        SLIP_END=b'\xc0'
        SLIP_ESC=b'\xdb'
        SLIP_ESC_END=b'\xdc'
        SLIP_ESC_ESC=b'\xdd'

        escape_char = False
        message = bytearray()

        # Infinite loop until end of frame detected
        while(self.running.is_set()):
            try:
                b = self.serial.read()
            except Exception:
                # Happen when connection is stopped
                continue

            # Check first end char (0xC0)
            if b == SLIP_END:
                if len(message) != 0:
                    # Full frame received
                    break
                else:
                    # Beginning of message
                    continue

            # Slip decoding
            if escape_char:
                # Last char was escape char, so next one must be 0xdc or 0xdd
                if b == SLIP_ESC_END:
                    message+= SLIP_END
                elif b == SLIP_ESC_ESC:
                    message+= SLIP_ESC
                else:
                    self.logger.error("Wrong second escape char")
                escape_char = False

            elif b == SLIP_ESC:
                escape_char = True
            else:
                message+=b

        return message

    def serial_reader(self, dev, queue):
        """
        Thread responsible for reading from serial port and storing packets into queue.
        """

        # Data read is in Hex. Not optimal compare to binary but easy to handle
        try:
            self.serial.reset_input_buffer()
            self.serial.reset_output_buffer()

            while self.running.is_set():
                message = self._read_slip_frame()
                queue.put(self.pcap_packet(message, int(time.time()  * (10 ** 6))))

        except (serialutil.SerialException, serialutil.SerialTimeoutException) as e:
            self.logger.error("Cannot communicate with serial device: {} reason: {}".format(dev, e))
        finally:
            if self.running.is_set():
                self.stop_sig_handler()

        self.logger.info("Exiting fifo reader")

    def fifo_writer(self, fifo, queue):
        """
        Thread responsible for writing packets into pcap file/fifo from queue.
        """
        with open(fifo, 'wb', 0 ) as fh:
            fh.write(self.pcap_header())
            fh.flush()

            while self.running.is_set():
                try:
                    packet = queue.get(block=True, timeout=1)
                    try:
                        fh.write(packet)
                        fh.flush()
                    except IOError:
                        pass

                except Queue.Empty:
                    pass

        self.logger.info("Exiting fifo writer")


    def extcap_capture(self, fifo, dev):
        """
        Main method responsible for starting all other threads. In case of standalone execution this method will block
        until SIGTERM/SIGINT and/or stop_sig_handler disables the loop via self.running event.
        """

        if len(self.threads):
            raise RuntimeError("Old threads were not joined properly")

        # Try to open Serial connection first
        start_connection = time.time()
        while True:
            try:
                self.serial = Serial(dev, baudrate=115200, timeout=1, exclusive=True)
                break
            except Exception as e:
                if self.connection_open_timeout is not None and \
                    time.time() > start_connection + self.connection_open_timeout:
                    raise RuntimeError(
                        "Could not open serial connection to sniffer before timeout of {} seconds".format(
                            self.connection_open_timeout))
                time.sleep(0.5)

        packet_queue = Queue.Queue()
        self.dev = dev
        self.running.set()

        self.dlt = 147

        self.threads.append(threading.Thread(target=self.serial_reader, args=(self.dev, packet_queue), name="serial_reader", daemon=True))
        self.threads.append(threading.Thread(target=self.fifo_writer, args=(fifo, packet_queue), name="fifo_writer", daemon=True))


        for thread in self.threads:
            thread.start()

        while is_standalone and self.running.is_set():
            time.sleep(1)

        logging.info("Exiting main thread")
        self.serial.close()


    @staticmethod
    def parse_args():
        """
        Helper methods to make the standalone script work in console and wireshark.
        """
        parser = ArgumentParser(description="Extcap program for the nRF Sniffer for 802.15.4")

        parser.add_argument("--extcap-interfaces", help="Provide a list of interfaces to capture from", action="store_true")
        parser.add_argument("--extcap-interface", help="Provide the interface to capture from")
        parser.add_argument("--extcap-dlts", help="Provide a list of dlts for the given interface", action="store_true")
        parser.add_argument("--extcap-config", help="Provide a list of configurations for the given interface", action="store_true")
        parser.add_argument("--capture", help="Start the capture routine", action="store_true" )
        parser.add_argument("--fifo", help="Use together with capture to provide the fifo to dump data to")
        parser.add_argument("--extcap-capture-filter", help="Used together with capture to provide a capture filter")

        result, unknown = parser.parse_known_args()

        if result.capture and not result.extcap_interface:
            parser.error("--extcap-interface is required if --capture is present")

        return result

    def __str__(self):
        return "{} ({}) channel {}".format(type(self).__name__, self.dev, self.channel)

    def __repr__(self):
        return self.__str__()


if is_standalone:
    args = WirepasNodeSniffer.parse_args()

    logging.basicConfig(format='%(asctime)s [%(levelname)s] %(message)s', level=logging.INFO)

    sniffer_comm = WirepasNodeSniffer(2)

    if args.extcap_interfaces:
        print(sniffer_comm.extcap_interfaces())

    if args.extcap_dlts:
        print(sniffer_comm.extcap_dlts())

    if args.extcap_config:
        print(sniffer_comm.extcap_config())

    if args.capture and args.fifo:
        signal.signal(signal.SIGINT, sniffer_comm.stop_sig_handler)
        signal.signal(signal.SIGTERM, sniffer_comm.stop_sig_handler)

        try:
            sniffer_comm.extcap_capture(args.fifo, args.extcap_interface)
        except KeyboardInterrupt as e:
            sniffer_comm.stop_sig_handler()
        except RuntimeError as e:
            logging.error("Cannot open serial connection")
            exit(1)
