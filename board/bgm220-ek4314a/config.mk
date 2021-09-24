# Mcu of the board
MCU_FAMILY=efr
MCU=efr32
MCU_SUB=xg22
MCU_MEM_VAR=xxxxf512

# Radio of the module on the board
radio=bgm220pc22hna

# Hardware capabilities of the board
## Is 32kHz crystal mounted on the board.
board_hw_crystal_32k=yes
## Is DCDC used on this board (must be yes for efr32xg22).
board_hw_dcdc=yes
## HFXO crystal characteristics
board_hw_hfxo_ctune=120
## LFXO crystal characteristics
board_hw_lfxo_ctune=37
board_hw_lfxo_gain=1
