# Mcu of the board
MCU_FAMILY=efr
MCU=efr32
MCU_SUB=xg12
MCU_MEM_VAR=pxxxf1024

# Hardware capabilities of the board
## Is 32kHz crystal mounted on the board.
board_hw_crystal_32k=yes
## Is DCDC used on this board.
board_hw_dcdc=yes
## HFXO crystal characteristics
board_hw_hfxo_ctune=322
## LFXO crystal characteristics
board_hw_lfxo_ctune=68
board_hw_lfxo_gain=2

# Uncomment, and set path to custom power table here, if set, the custom power
# table will be set during application startup, and the stack will use that
# instead of the default
# For convenience, an example of a +19dBm power table is provided, uncomment the
# line below to use that instead of the default +10dBm power table
# NOTE! +19 dBm power level is usable only where US FCC regulation applies
#RADIO_CUSTOM_POWER_TABLE=mcu/efr/efr32/hal/radio/radio_power_table_efr32xg12_19dBm.h
