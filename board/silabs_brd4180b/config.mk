# Mcu of the board
MCU_FAMILY=efr
MCU=efr32
MCU_SUB=xg21
MCU_MEM_VAR=xxxxf1024

# Which radio to use
radio=efr32xg21

# Hardware capabilities of the board
## Is 32kHz crystal mounted on the board.
board_hw_crystal_32k=yes
## Is DCDC used on this board. (must be no for efr32xg21).
board_hw_dcdc=no
## HFXO crystal characteristics
board_hw_hfxo_ctune=129
## LFXO crystal characteristics
board_hw_lfxo_ctune=79
board_hw_lfxo_gain=1

# Uncomment, and set path to custom power table here, if set, the custom power
# table will be set during application startup, and the stack will use that
# instead of the default one.
# For convenience, an example of a +20dBm power table is provided, uncomment the
# line below to use that instead of the default +10dBm power table
# NOTE! +20 dBm power level is usable only where US FCC regulation applies
#RADIO_CUSTOM_POWER_TABLE=mcu/efr32/hal/radio/radio_power_table_efr32xg21_20dBm.h
