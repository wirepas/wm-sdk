# Mcu of the board
MCU_FAMILY=efr
MCU=efr32
MCU_SUB=xg24
MCU_MEM_VAR=xxxxf1536
MCU_RAM_VAR=256

# Which radio to use
radio=efr32xg24

# Hardware capabilities of the board
## Is 32kHz crystal mounted on the board.
board_hw_crystal_32k=yes
## Is DCDC used on this board. 
board_hw_dcdc=yes
## HFXO crystal characteristics
## https://github.com/SiliconLabs/gecko_sdk/blob/gsdk_4.2/hardware/board/config/brd2601a/sl_device_init_hfxo_config.h
board_hw_hfxo_ctune=140
## LFXO crystal characteristics
## https://github.com/SiliconLabs/gecko_sdk/blob/gsdk_4.2/hardware/board/config/brd2601a/sl_device_init_lfxo_config.h
board_hw_lfxo_ctune=63
board_hw_lfxo_gain=1
