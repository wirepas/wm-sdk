#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# sdk_build.py - A tool to build multiple apps (or all)
#

import sys
import argparse
import subprocess
from sdk_explorer import SDKExplorer


def build_single_app(root_folder, app_name, board, app_only=False, extra_build_args=None):
    """Build a given application

    Args:
        root_folder: root_folder of sdk
        app_name: name of the app to build
        board: board to build
        full_image: True to build all final images and otap files. False to only build app binary
        xtra_build_args: list of args to append to the make cmd
    """
    build_cmd = ['make',
                 '-j7',
                 'app_name={}'.format(app_name),
                 'target_board={}'.format(board)]

    if extra_build_args is not None:
        for arg in extra_build_args:
            build_cmd.append(arg)

    if app_only:
        build_cmd.append('app_only')

    print("Building app %s for %s (app_only=%s)" % (app_name, board, app_only))

    _process = subprocess.Popen(build_cmd,
                                cwd=root_folder)
    assert(_process.wait() == 0)

def build_all_apps(root_folder, boards_subset=None, apps_subset=None, app_only=False, extra_build_args=None):
    """Build all possible apps for all boards within the defined subsets

    Args:
        root_folder: root_folder of sdk
        apps_subset: subset of apps to build
        boards_subset: subset of boards to build
        app_only: True to build app binaries. False for all final images and otap files.
        extra_build_args: list of args to append to the make cmd
    """
    sdk_matrix = SDKExplorer(root_folder).get_app_board_matrix()
    for app_name, compatible_boards in sdk_matrix.items():
        if apps_subset is not None and app_name not in apps_subset:
            continue

        for board in compatible_boards:
            if boards_subset is not None and board not in boards_subset:
                continue

            # App must be built
            build_single_app(root_folder, app_name, board, app_only, extra_build_args)


def main():
    """Main program

    This utility script alllows to build multiples apps
    """

    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("--root_folder", "-r",
                        help="Root folder of SDK",
                        default="./")

    parser.add_argument("--boards", "-b",
                        help="List of boards subset",
                        nargs='*',
                        default=None)

    parser.add_argument("--apps", "-a",
                        help="List of Apps subset",
                        nargs='*',
                        default=None)

    parser.add_argument("--app_only", "-ao",
                        action='store_true',
                        help="Should final config be checked")

    parser.add_argument("--extra_build_args", "-e",
                        help="List of extra build args",
                        nargs='*',
                        default=None)

    args = parser.parse_args()

    build_all_apps(args.root_folder, args.boards, args.apps, args.app_only, args.extra_build_args)


if __name__ == '__main__':
    sys.exit(main())
