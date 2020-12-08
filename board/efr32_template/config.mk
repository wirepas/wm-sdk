# This definition describes processor architecture.
# Valid value: efr32 for EFR32 based boards
MCU=efr32

# This describes the sub-architecture of the processor. Allowed values are
# following: xg12,xg13
MCU_SUB=xg12

# This definitions describes the processor memory variant. It is only used when
# <code>@ref config_mk_mcu "MCU"</code> is <code>efr32</code>.
# Valid values are following:
# - <code>pxxxf512</code> For 512 kB flash memory variant
# - <code>pxxxf1024</code> For 1024 kB flash memory variant
MCU_MEM_VAR=pxxxf1024

# This describes the hardware capabilities of the board
# (this is used to customize the hardware service of the bootloader).
## Is 32kHz crystal mounted on the board? default:yes, possible values:yes, no
board_hw_crystal_32k=yes
## Is DCDC to be enabled on the board? default:yes, possible values:yes, no
## (it replaces MCU_NO_DCDC define in ./board/bootlaoder/early_init_efr32.c)
board_hw_dcdc=yes