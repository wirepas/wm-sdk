# Mcu of the board
MCU_FAMILY=nrf
MCU=nrf52
MCU_SUB=840

# Hardware capabilities of the board
## Is 32kHz crystal mounted on the board.
board_hw_crystal_32k=yes
## Is DCDC used on this board.
board_hw_dcdc=yes

# Uncomment, and set path to custom power table here, if set, the custom power
# table will be set during application startup, and the stack will use that
# instead of the default
# For convenience, an example of a +4dBm power table is provided, uncomment the
# line below to use that instead of the default +8dBm power table
#RADIO_CUSTOM_POWER_TABLE=mcu/nrf/nrf52/hal/radio/radio_power_table_nrf52840_4dBm.h
