#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Change to parent folder of this script
cd $DIR/..

# set the default profiles to copy
DEFAULT_PROFILES="regression_nrf52833 regression_efr32_24  regression_nrf52832 regression_nrf52840_24_8dbm regression_efr32_aus915 regression_efr32xg13_india865"
PROFILES=${1:-${DEFAULT_PROFILES}}
set -e

# copy files function
function cp_file {
    if [[ -a $1 ]]; then
    echo "cp $1 $2"
    cp $1 $2
    fi
}

# the function to check the result
function check_return_value {
    if [ $? -gt 0 ]; then
        echo ERROR: $*
        exit 1
    fi
}

function echo_run {
    echo "$*"
    $*
}

# the function to copy the needed stack and bootloader image
function copy_stack_to_app_interface_image {
    STACK_PROFILE=$1
    if [ ! -d  ../app-interface/image/ ]; then
        echo_run "mkdir -p app-interface/image/"
    fi
    echo_run "cp_file ../appimage/${STACK_PROFILE}_bootloader.a ../app-interface/image/"
    echo_run "cp_file ../appimage/${STACK_PROFILE}_bootloader.conf ../app-interface/image/"
    echo_run "cp_file ../appimage/${STACK_PROFILE}_wsn_stack.hex ../app-interface/image/"
    echo_run "cp_file ../appimage/${STACK_PROFILE}_wsn_stack.conf ../app-interface/image/"
}

echo "#########################Start to copy stack and bootload files#######################"
for profile in $PROFILES ; do
    echo "copy $profile to image folder "
    copy_stack_to_app_interface_image $profile
    check_return_value "***copy $profile to image folder failed"
done

cd $PWD