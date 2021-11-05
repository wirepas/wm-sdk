# This definition describes mcu family.
MCU_FAMILY=efr

# This definition describes processor architecture.
# Valid value: efr32 for EFR32 based boards
MCU=efr32

# This describes the sub-architecture of the processor. Allowed values are
# following: xg12,xg21,xg22
MCU_SUB=xg12

# This definitions describes the processor memory variant. It is only used when
# <code>@ref config_mk_mcu "MCU"</code> is <code>efr32</code>.
# Valid values are following:
# - <code>pxxxf512</code> For 512 kB flash memory variant (xg12)
# - <code>xxxxf512</code> For 512 kB flash memory variant (xg22)
# - <code>pxxxf1024</code> For 1024 kB flash memory variant (xg12)
# - <code>xxxxf1024</code> For 1024 kB flash memory variant (xg21)
MCU_MEM_VAR=pxxxf1024

# Which radio version to use (different stack binary)
# Must be set only for xg21 and xg22 only
# Allowed values are following for xg21: efr32xg21, bgm210pa22jia
# Allowed values are following for xg22: efr32xg22, bgm220pc22hna
# radio=

# This describes the hardware capabilities of the board
# (this is used to customize the hardware service of the bootloader).
## Is 32kHz crystal mounted on the board? default:yes, possible values:yes, no
board_hw_crystal_32k=yes
## Is DCDC to be enabled on the board? default:yes, possible values:yes, no
## (it replaces MCU_NO_DCDC define in ./board/bootlaoder/early_init_efr32.c)
board_hw_dcdc=yes
## HFXO crystal characteristics
board_hw_hfxo_ctune=322
## LFXO crystal characteristics
board_hw_lfxo_ctune=68
board_hw_lfxo_gain=2
