#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import wget
url='https://raw.githubusercontent.com/NordicSemiconductor/nrfx/v2.5.0/'

files = [
        {"input": "nrfx.h", "output": "."},
        {"input": "mdk/nrf.h", "output": 'mdk'},
        {"input": "mdk/nrf_peripherals.h", "output": "mdk"},
        {"input": "hal/nrf_gpio.h", "output": "hal"},
        {"input": "hal/nrf_gpiote.h", "output": "hal"},
        {"input": "hal/nrf_uarte.h", "output": "hal"},
        {"input": "hal/nrf_common.h", "output": "hal"},
        {"input": "hal/nrf_timer.h", "output": "hal"},
        {"input": "drivers/nrfx_common.h", "output": "drivers"},
        {"input": "drivers/nrfx_errors.h", "output": "drivers"},
        {"input": "mdk/nrf52.h", "output": 'mdk'},
        {"input": "mdk/nrf52_bitfields.h", "output": 'mdk'},
        {"input": "mdk/nrf51_to_nrf52.h", "output": 'mdk'},
        {"input": "mdk/nrf52_name_change.h", "output": 'mdk'},
        {"input": "mdk/nrf52840.h", "output": 'mdk'},
        {"input": "mdk/nrf52833.h", "output": 'mdk'},
        {"input": "mdk/nrf52840_bitfields.h", "output": 'mdk'},
        {"input": "mdk/nrf52833_bitfields.h", "output": 'mdk'},
        {"input": "mdk/nrf51_to_nrf52840.h", "output": 'mdk'},
        {"input": "mdk/nrf52_to_nrf52840.h", "output": 'mdk'},
        {"input": "mdk/nrf52_to_nrf52833.h", "output": 'mdk'},
        {"input": "mdk/system_nrf52.h", "output": 'mdk'},
        {"input": "mdk/system_nrf52840.h", "output": 'mdk'},
        {"input": "mdk/system_nrf52833.h", "output": 'mdk'},
        {"input": "mdk/nrf52832_peripherals.h", "output": 'mdk'},
        {"input": "mdk/nrf52833_peripherals.h", "output": 'mdk'},
        {"input": "mdk/nrf52840_peripherals.h", "output": 'mdk'},
        {"input": "mdk/nrf9160_bitfields.h", "output": "mdk"},
        {"input": "mdk/nrf9160.h", "output": "mdk"},
        {"input": "mdk/system_nrf9160.h", "output": "mdk"},
        {"input": "mdk/system_nrf.h", "output": "mdk"},
        {"input": "mdk/compiler_abstraction.h", "output": "mdk"},
        {"input": "mdk/nrf9160_peripherals.h", "output": "mdk"},
        {"input": "mdk/nrf9160_name_change.h", "output": "mdk"},
        {"input": "templates/nrfx_config.h", "output": "templates"},
        {"input": "templates/nrfx_config_nrf52832.h", "output": "templates"},
        {"input": "templates/nrfx_config_nrf52833.h", "output": 'templates'},
        {"input": "templates/nrfx_config_nrf52840.h", "output": "templates"},
        {"input": "templates/nrfx_config_nrf9160.h", "output": "templates"},
        {"input": "templates/nrfx_glue.h", "output": "templates"},
    ]

for file in files:
    src = url+file['input']
    dest = file['output']
    print('\nLoading:{}, to location:{}\n'.format(src, dest))
    filename = wget.download (url=src, out=dest)

print ("\n**ALL DONE**\n")
