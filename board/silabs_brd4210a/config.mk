# Mcu of the board
MCU_FAMILY=efr
MCU=efr32
MCU_SUB=xg23
MCU_MEM_VAR=xxxxf512

# Which radio to use
radio=efr32xg23
# Radio Configuration aus915 or india865
#radio_config=aus915
radio_config=india865
mac_profile=subg

# Hardware capabilities of the board
## Is 32kHz crystal mounted on the board.
board_hw_crystal_32k=no
## Is DCDC used on this board.
board_hw_dcdc=yes
## HFXO crystal characteristics
board_hw_hfxo_ctune=106
## LFXO crystal characteristics
board_hw_lfxo_ctune=63
board_hw_lfxo_gain=2

# Custom power table
# if set, the custom power table will be set during application startup,
# and the stack will use that instead of the default one.
# For convenience, +13dBm, +16dBm and +20dBm power tables are provided as examples.
# - mcu/efr/efr32/hal/radio/radio_power_table_efr32xg23_13dbm.h
# - mcu/efr/efr32/hal/radio/radio_power_table_efr32xg23_16dbm.h
# - mcu/efr/efr32/hal/radio/radio_power_table_efr32xg23_20dbm.h
# If predefined custom power table shall be used,
# provide radio_power_table=13/16/20 as build parameter.
ifneq ("$(radio_power_table)", "")
    RADIO_CUSTOM_POWER_TABLE=mcu/efr/efr32/hal/radio/radio_power_table_efr32xg23_$(radio_power_table)dbm.h
endif
