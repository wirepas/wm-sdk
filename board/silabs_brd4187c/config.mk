# Mcu of the board
MCU_FAMILY=efr
MCU=efr32
MCU_SUB=xg24
MCU_MEM_VAR=xxxxf1536
MCU_RAM_VAR=256
# Run internal tests with the minumum memory configuration
MCU_MEM_VAR=xxxxf1024
MCU_RAM_VAR=128
# Which radio to use
radio=efr32xg24

# Hardware capabilities of the board
## Is 32kHz crystal mounted on the board.
board_hw_crystal_32k=yes
## Is DCDC used on this board.  Radio is powered using dcdc
board_hw_dcdc=yes
## HFXO crystal characteristics
## https://github.com/SiliconLabs/gecko_sdk/blob/gsdk_4.2/hardware/board/config/brd4186c_brd4001a/sl_device_init_hfxo_config.h
board_hw_hfxo_ctune=87

## LFXO crystal characteristics
## https://github.com/SiliconLabs/gecko_sdk/blob/gsdk_4.2/hardware/board/config/brd4186c_brd4001a/sl_device_init_lfxo_config.h
board_hw_lfxo_ctune=36

board_hw_lfxo_gain=1

# Set path to custom power table here, if set, the custom power
# table will be set during application startup, and the stack will use that
# instead of the default one.
# For convenience, an example of a +20dBm power table is provided, uncomment the
# line below or use radio_power_table=20 setting as make argument to use that
# instead of the default +8dBm power table.
# NOTE! +20 dBm power level is usable only where US FCC regulation applies
#RADIO_CUSTOM_POWER_TABLE=mcu/efr/efr32/hal/radio/radio_power_table_efr32xg24_20dBm.h
ifneq ("$(radio_power_table)", "")
    RADIO_CUSTOM_POWER_TABLE=mcu/efr/efr32/hal/radio/radio_power_table_efr32xg24_$(radio_power_table)dbm.h
endif
