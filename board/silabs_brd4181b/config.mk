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
board_hw_hfxo_ctune=133
## LFXO crystal characteristics
board_hw_lfxo_ctune=37
board_hw_lfxo_gain=1
