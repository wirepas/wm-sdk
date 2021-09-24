# This definition describes mcu family.
MCU_FAMILY=nrf

# This definition describes processor architecture.
# Valid value: nrf52 for nRF52 based boards
MCU=nrf52

# This describes the sub-architecture of the processor. Allowed values are
# following: 832,840
MCU_SUB=832

# This describes the hardware capabilities of the board
# (this is used to customize the hardware service of the bootloader).
## Is 32kHz crystal mounted on the board? default:yes, possible values:yes, no
board_hw_crystal_32k=yes
## Is DCDC to be enabled on the board? default:yes, possible values:yes, no
## (it replaces BOARD_SUPPORT_DCDC define in board.h)
board_hw_dcdc=yes
