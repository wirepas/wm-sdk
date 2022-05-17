#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# sdk_build.py - A tool to build multiple apps (or all)
#

import sys
import argparse
import multiprocessing
import subprocess
import os
import hashlib
from sdk_explorer import SDKExplorer


def build_single_app(root_folder, app_name, board, app_only=False, config=None, extra_build_args=None):
    """Build a given application

    Args:
        root_folder: root_folder of sdk
        app_name: name of the app to build
        board: board to build
        app_only: False to build all final images and otap files. True to only build app binary
        config: which config file to take to build the app
        extra_build_args: list of args to append to the make cmd
    """
    
    _cpu_count = multiprocessing.cpu_count()
    
    build_cmd = ['make',
                 '-j{}'.format(_cpu_count),
                 'app_name={}'.format(app_name),
                 'target_board={}'.format(board)]

    if config is not None:
        build_cmd.append('app_config={}'.format(config))

    if extra_build_args is not None:
        for arg in extra_build_args:
            build_cmd.append(arg)

    if app_only:
        build_cmd.append('app_only')

    print("Build:",' '.join(build_cmd))
    _process = subprocess.Popen(build_cmd,
                                cwd=root_folder)
    assert(_process.wait() == 0)

def remove_duplicate_boards(matrix, app, dup_boards):
    """Remove duplicated boards from list. Only one board from dup_board should remain
       in initial list

    Args:
        matrix: matrix of all apps
        app: name of app
        dup_boards: duplicated boards
    """

    try:
        if app in matrix:
            all_boards = matrix[app]["config.mk"]
        else:
            # It is an app with alternate config
            app, config_suffix = app.rsplit('_config', 1)
            all_boards = matrix[app]["config"+config_suffix+".mk"]
    except KeyError:
        print("Something went wrong when removing duplicate for app ", app)
        return

    # Only one from dup_boards list should remain
    found = False
    for dup in dup_boards:
        if dup in all_boards:
            if not found:
                # One found so others can be deleted
                found = True
            else:
                all_boards.remove(dup)

def get_number_of_combination(matrix):
    count = 0
    for app, configs in matrix.items():
        for config, boards in configs.items():
            count += len(boards)
    return count

def build_all_apps(root_folder, boards_subset=None, apps_subset=None, app_only=False, extra_build_args=None, file_aliases=None, num_chunks=1, chunk_id=0):
    """Build all possible apps for all boards within the defined subsets

    Args:
        root_folder: root_folder of sdk
        apps_subset: subset of apps to build
        boards_subset: subset of boards to build
        app_only: True to build app binaries. False for all final images and otap files.
        extra_build_args: list of args to append to the make cmd
        file_aliases: file path for alias list
        num_chunks: number of chunks to generate out of all matrix combinations
        chunk_id: which chunk id to do (must be < than num_chunks)
    """
    sdk_matrix = SDKExplorer(root_folder).get_app_board_matrix()

    if file_aliases is not None:
        before = get_number_of_combination(sdk_matrix)
        # Cleanup the full list by removing aliases
        try:
            with open(file_aliases, "r") as f:
                for line in f.readlines():
                    app, boards = line.split(':')
                    duplicate_boards = boards.rstrip('\n').split(',')

                    remove_duplicate_boards(sdk_matrix, app, duplicate_boards)

        except FileNotFoundError:
            print("Cannot open file for aliases")

        after = get_number_of_combination(sdk_matrix)
        print("Duplicate removal from ", before, " to ", after, " combinations")

    if chunk_id >= num_chunks:
        raise ValueError("num_chunks must be < chunk_id")

    for app_name, configs in sorted(sdk_matrix.items())[chunk_id::num_chunks]:
        if apps_subset is not None and app_name not in apps_subset:
            continue

        # Bootloader updater cannot be built with app_only (ie no access to bootlader firmware)
        if app_only and app_name == "bootloader_updater":
            continue

        # Check different configs
        for config, compatible_boards in configs.items():
            if config == "config.mk":
                config = None
            else:
                config = config.rsplit(".", 1)[0]
            for board in compatible_boards:
                if boards_subset is not None and board not in boards_subset:
                    continue


                # App must be built
                build_single_app(root_folder, app_name, board, app_only=app_only, config=config, extra_build_args=extra_build_args)

def generate_alias_matrix(root_folder, file):
    """

    Args:
        root_folder: root_folder of sdk
    """
    aliases_dic = {}
    build_folder = os.path.join(root_folder, 'build')
    for board in os.listdir(build_folder):

        board_folder = os.path.join(build_folder, board)
        for app in os.listdir(board_folder):
            # Make app hex sha256 to compare
            app_folder = os.path.join(board_folder, app)
            app_hex = os.path.join(app_folder, "{}.hex".format(app))
            try:
                with open(app_hex, 'rb') as f:
                    file_hash = hashlib.sha256(f.read()).hexdigest()
            except FileNotFoundError:
                print("Cannot find app: app_hex")
                continue

            try:
                aliases_dic[file_hash].append((board, app))
            except KeyError:
                # New digest => add entry
                aliases_dic[file_hash] = [(board, app)]

    all = 0
    with open(file, "w") as f:
        for key, value in aliases_dic.items():
            #print(key, ' -> ', value)
            all += len(value)

            if len(value) == 1:
                continue

            alias = "{}:".format(value[0][1])
            for v in value:
                alias += ("{},".format(v[0]))

            print(alias)
            alias += "\n"
            f.write(alias)

    print("Before: ", all, " After: ", len(aliases_dic))

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
                        help="Build only for apps")

    parser.add_argument("--extra_build_args", "-e",
                        help="List of extra build args",
                        nargs='*',
                        default=None)

    parser.add_argument("--alias_apps", "-aa",
                        help="File with list of duplicated app/board combination (alias app)",
                        default=None)

    parser.add_argument("--generate_alias_list", "-gal",
                        help="Generate the alias list",
                        default=None)

    parser.add_argument("--chunk_num", "-cn",
                        help="Number of chunks to create",
                        default=1,
                        type=int)

    parser.add_argument("--chunk_id", "-ci",
                        help="Which chunk to build",
                        default=0,
                        type=int)

    args = parser.parse_args()

    build_all_apps(args.root_folder,
                   args.boards,
                   args.apps,
                   args.app_only,
                   args.extra_build_args,
                   args.alias_apps,
                   args.chunk_num,
                   args.chunk_id)

    if args.generate_alias_list != None:
        generate_alias_matrix(args.root_folder, args.generate_alias_list)


if __name__ == '__main__':
    sys.exit(main())
