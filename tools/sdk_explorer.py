#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# sdk_wizard.py - A tool to configure initial steps in SDK
#

import sys
import os
import argparse

APP_FOLDER = "source"
APP_SUBFOLDERS = ["example_apps","reference_apps","unitary_apps"]
BOARD_FOLDER = "board"


class SDKExplorer(object):
    """SDK explorer class

        A class to easily get access to application and board available from an SDK
    """
    def __init__(self, root_folder):
        """ Constructor

        Args:
            root_folder: root folder of SDK
        """
        self.root_folder = root_folder

    def get_all_supported_boards(self):
        """ Extracts all possible target boards

        Return: List of supported boards
        """
        board_folder = os.path.join(self.root_folder, BOARD_FOLDER)

        if os.path.exists(board_folder):
            # Each subfolder is considered as a board name
            all_boards = list(filter(lambda x:
                                     os.path.isdir(os.path.join(board_folder, x)) and not "_template" in x,
                                     os.listdir(board_folder)))

            return all_boards

        print("No folder %s (is root folder the right one? (-r option)" % board_folder)
        return None

    def get_all_apps(self, folder):
        """ Extracts all possible apps from source folder

        Args:
            folder: Folder to look for apps

        Return: List of supported apps
        """
        app_folder = os.path.join(self.root_folder, folder)

        if os.path.exists(app_folder):
            # Each subfolder is considered as app name
            all_apps = list(filter(lambda x:
                                   os.path.isdir(os.path.join(app_folder, x)),
                                   os.listdir(app_folder)))

            return all_apps

        print("No folder %s (is root folder the right one? (-r option)" % folder)
        return None

    def get_app_configs(self, app_name, folder):
        """ Extracts all configs for a given app

        Args:
            app_name: Application name
            folder: Folder to look for this app

        Return: List of mk config files
        """
        app_folder = os.path.join(self.root_folder, folder, app_name)
        if not os.path.exists(app_folder):
            print("App %s in %s doesn't exist" % (app_name, folder))
            return None

        # Each subfolder is considered as a board name
        return list(filter(lambda x:
                      x.endswith(".mk") and x.startswith("config"),
                      os.listdir(app_folder)))

    def get_compatible_boards_for_app(self, app_name, folder, config="config.mk"):
        """ Extracts all compatible boards for a given app

        Args:
            app_name: Application name
            folder: Folder to look for this app
            config: config file to use

        Return: List of supported boards
        """
        app_folder = os.path.join(self.root_folder, folder, app_name)
        if not os.path.exists(app_folder):
            print("App %s in %s doesn't exist" % (app_name, folder))
            return None

        config_file = os.path.join(app_folder, config)
        if not os.path.exists(config_file):
            return self.get_all_supported_boards()

        boards = []
        with open(config_file, 'r') as f:
            for line in f:
                if line.startswith("TARGET_BOARDS"):
                    boards += line.split("=")[1].split()

        if boards.__len__() == 0:
            # No target boards in config.mk, so assuming all
            return self.get_all_supported_boards()

        return boards

    def get_app_board_matrix(self):
        """ Extract a matrix of all apps and their associated boards

        Return: dictionary of dictionnary with app as first key,
                config as second key and compatible boards as values
        """
        matrix = {}

        for subfolder in APP_SUBFOLDERS:
            app_folder = os.path.join(APP_FOLDER, subfolder)
            apps = self.get_all_apps(app_folder)

            if apps is not None:
                for app in apps:
                    # Check for alternate config
                    configs = self.get_app_configs(app, app_folder)
                    matrix[app] = {}
                    for config in configs:
                        matrix[app][config] = self.get_compatible_boards_for_app(app, app_folder, config)


        return matrix


def main():
    """Main program

    Invoking directly this script allows to get some SDK information easily but the
    main purpose of SDKConfig class is to be used from other python script
    """

    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("--root_folder", "-r",
                        help="Root folder of SDK",
                        default="./")

    args = parser.parse_args()

    sdk_matrix = SDKExplorer(args.root_folder).get_app_board_matrix()
    for app_name, boards_per_config in sdk_matrix.items():
        for config, boards in boards_per_config.items():
            print("%s (%s):\n\t%s" % (app_name, config.split(".mk")[0], boards))


if __name__ == '__main__':
    sys.exit(main())
