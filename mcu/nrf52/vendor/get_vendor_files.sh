#!/bin/bash

url=https://raw.githubusercontent.com/NordicSemiconductor/nrfx/v2.0.0/
files="
    mdk/nrf.h
    mdk/nrf52.h
    mdk/nrf52_bitfields.h
    mdk/nrf51_to_nrf52.h
    mdk/nrf52_name_change.h
    mdk/nrf52840.h
    mdk/nrf52833.h
    mdk/nrf52840_bitfields.h
    mdk/nrf52833_bitfields.h
    mdk/nrf51_to_nrf52840.h
    mdk/nrf52_to_nrf52840.h
    mdk/nrf52_to_nrf52833.h
    mdk/system_nrf52.h
    mdk/system_nrf52840.h
    mdk/system_nrf52833.h
    mdk/system_nrf.h
    mdk/compiler_abstraction.h
    hal/nrf_gpio.h
    mdk/nrf52832_peripherals.h
    mdk/nrf52833_peripherals.h
    mdk/nrf52840_peripherals.h
    "

for file in $files
    do
        if [ ! -f $file ]; then
            echo "$file does not exist, download it"
            echo Get: $url$file
            wget $url$file
        fi
    done
 

