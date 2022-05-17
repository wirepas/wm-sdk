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
