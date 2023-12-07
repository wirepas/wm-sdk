# Copyright 2023 Wirepas Ltd licensed under Apache License, Version 2.0
#
# See file LICENSE for full license details.
#
import argparse
import logging
import matplotlib
matplotlib.use('Agg')  # change matplotlib backend to work with threads
import matplotlib.pyplot as plt
import os
import pandas as pd
from threading import Lock, Thread
from time import sleep

from wirepas_mqtt_library import WirepasNetworkInterface

RTC_SOURCE_EP = 79
RTC_DEST_EP = 78

NODE_ID_KEY = "node_id"
TIMESTAMP_MS_KEY = "timestamp_ms"
TIME_DIFF_MS_KEY = "time_diff_ms"



def create_directory(filename):
    """
    Create the directory of the file if it doesn't already exists.

    Args:
        filename (str): Path and name of the file.
    """
    dirname = os.path.dirname(filename)
    if not os.path.isfile(filename):
        os.makedirs(dirname, mode=0o644, exist_ok=True)


def write_data(filename, timestamp, time_diff, node):
    """
    Add rtc data to a file with a csv format.
    The data contains in the order:
        * The real timestamp of the reception of the rtc
        * The time difference between the expected time and the real one
        * Node referencing the data

    Note: The path of the file must already exists.

    Args:
        filename (str): Name of the file to store data.
        timestamp (int): The real timestamp of the reception of the rtc
        time_diff (int): The time difference between the expected time and the real one
        node (str): Source node id referencing the data
    """
    if not os.path.isfile(filename):
        with open(filename, "w") as file:  # create the header
            file.write("node_id,timestamp_ms,time_diff_ms\n")
    with open(filename, "a") as file:  # add a line of data
        file.write(",".join(map(str, [node, timestamp, time_diff])) + "\n")


class DrawTimeDifference:
    """
    Module to draw the time difference between expected and real RTC time
    in the network.
    Draw a chart representing the observed time difference in function
    of the time for each of the node in the network.

    Launch a thread updating periodically the chart.
    """
    def __init__(self, data_filename, chart_out_file, update_period_s):
        # Name of the file storing rtc data.
        self.data_filename = data_filename
        # File chart_out_file is used to store the charts.
        self.chart_out_file = chart_out_file
        # Period in seconds to update the drawing.
        self.update_period_s = update_period_s
        self.all_data = None

        if self.data_filename is not None and os.path.isfile(self.data_filename):
            # complete the data received with the ones already inside the file
            self.all_data = pd.read_csv(self.data_filename)
            self.all_data[TIMESTAMP_MS_KEY] = pd.to_datetime(self.all_data[TIMESTAMP_MS_KEY]*1000000)
        elif self.data_filename is not None and self.data_filename != "":
            create_directory(self.data_filename)

        # Prepare the chart
        self.fig, self.ax = plt.subplots()
        plt.setp(self.fig.gca().get_xticklabels(), rotation=45,
                 horizontalalignment='right')

        if self.chart_out_file is not None and self.chart_out_file != "":
            create_directory(self.chart_out_file)

        Thread(target=self.periodic_draw, daemon=True).start()

    def add_data(self, real_rtc_time, time_diff, node_id):
        """
        Add rtc data to a global variable of type pandas.DataFrame.

        Args:
            real_rtc_time (int): The real timestamp of the reception of the rtc.
            time_diff (int): Path and name of the file to store data.
            node (str): Source node id
        """
        new_data = pd.DataFrame.from_dict(
                    {TIMESTAMP_MS_KEY: [pd.to_datetime(real_rtc_time*1000000)],
                     TIME_DIFF_MS_KEY: time_diff,
                     NODE_ID_KEY: node_id})

        if self.all_data is not None:
            self.all_data = pd.concat([self.all_data, new_data])
        else:
            self.all_data = new_data

    def run(self):
        try:
            print("Input 'q' or 'quit()' to stop the algorithmn")
            while True:
                i = input()
                if i == "q" or i == "quit()":
                    break
                else:
                    print("Command not recognized!")
        except KeyboardInterrupt:
            pass
        self.draw_rtc_difference()  # Updates the drawing before exiting

    def draw_rtc_difference(self):
        """
        Draw the rtc data from a file.
        """
        if self.data_filename is None or not os.path.isfile(self.data_filename) or self.chart_out_file is None:
            return

        # Get all the data
        if self.all_data is None or len(self.all_data) < 1:
            return

        # Draw the chart
        plt.cla()
        for label, sub_df in self.all_data.groupby(NODE_ID_KEY):
            sub_df.plot(x=TIMESTAMP_MS_KEY,
                        y=TIME_DIFF_MS_KEY,
                        xlabel="Timestamp(ms)",
                        ylabel="Time difference(ms)",
                        ax=self.ax,
                        label=label)
        self.fig.savefig(self.chart_out_file)

    def periodic_draw(self):
        while True:
            self.draw_rtc_difference()
            sleep(self.update_period_s)


def main():
    lock_data_file = Lock()

    parser = argparse.ArgumentParser(fromfile_prefix_chars='@',
                    formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument('--host', help="MQTT broker address")
    parser.add_argument(
        '--port',
        type=int,
        default=8883,
        help="MQTT broker port",
    )
    parser.add_argument(
        '--username',
        help="MQTT broker username"
    )
    parser.add_argument(
        '--password',
        help="MQTT broker password"
    )
    parser.add_argument(
        '--insecure',
        action='store_true',
        help="MQTT use unsecured connection"
    )
    parser.add_argument(
        '--data_filename',
        type=str,
        help="For each received message, the file will store the following data:\n"
             "the id of the node sending the data,\n"
             "the time of the gateway when the data message is published in the MQTT,\n"
             "and the time difference between expected rtc and the real value.\n"
             "Note: If it is not provided only the logging information "
             "of the script will be displayed on the shell.",
    )
    parser.add_argument(
        '--chart_out_file',
        type=str,
        help="File to store the chart.\n"
             "The chart displays the time difference in function of "
             "the received time of the messages and grouped by their node id."
    )
    parser.add_argument(
        '--update_period_s',
        type=int,
        default=10,
        help="Period in seconds to update the chart."
    )
    args = parser.parse_args()

    logging.basicConfig(format='%(levelname)s %(asctime)s %(message)s',
                        level=logging.INFO)

    wni = WirepasNetworkInterface(args.host,
                                  args.port,
                                  args.username,
                                  args.password,
                                  insecure=args.insecure)

    draw_time_difference = DrawTimeDifference(args.data_filename,
                                    args.chart_out_file, args.update_period_s)

    def on_data_received(data):
        estimated_rtc_time = int.from_bytes(data.data_payload, 'little') + data.travel_time_ms
        real_rtc_time = data.rx_time_ms_epoch
        time_diff = data.rx_time_ms_epoch - estimated_rtc_time
        node_id = data.source_address
        logging.info(f"RTC real value :{real_rtc_time} "
                     f"vs estimated value:{estimated_rtc_time} "
                     f"= {time_diff}ms from node {node_id}")

        if args.data_filename:
            with lock_data_file:
                write_data(args.data_filename,
                           real_rtc_time,
                           time_diff,
                           node_id)
                draw_time_difference.add_data(real_rtc_time, time_diff, node_id)

    # Register for any data
    wni.register_data_cb(on_data_received, src_ep=RTC_SOURCE_EP,
                         dst_ep=RTC_DEST_EP)

    draw_time_difference.run()


if __name__ == '__main__':
    main()
