# Mcu of the board
MCU_FAMILY=efr
MCU=efr32
MCU_SUB=xg13
MCU_MEM_VAR=pxxxf512

# Which radio to use
radio=efr32xg13
# Radio Configuration india865
radio_config=india865
mac_profile=subg

# Hardware capabilities of the board
## Is 32kHz crystal mounted on the board.
board_hw_crystal_32k=no
## Is DCDC used on this board.
board_hw_dcdc=yes
## HFXO crystal characteristics
board_hw_hfxo_ctune=331
## LFXO crystal characteristics
board_hw_lfxo_ctune=32
board_hw_lfxo_gain=1

# Custom power table
# if set, the custom power table will be set during application startup,
# and the stack will use that instead of the default one.
# For convenience, +12dBm power table is provided as an example.
# - mcu/efr/efr32/hal/radio/radio_power_table_efr32xg13_12dbm.h
# If predefined custom power table shall be used,
# provide 'radio_power_table=12' as build parameter.
ifneq ("$(radio_power_table)", "")
    RADIO_CUSTOM_POWER_TABLE=mcu/efr/efr32/hal/radio/radio_power_table_efr32xg13_$(radio_power_table)dbm.h
endif

