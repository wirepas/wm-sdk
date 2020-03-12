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

# This describes the radio profiles to choose
# Allowed values are 0x00000009 (max power to 8dm), 0x0000000C (max power to 19dbm)
# See @ref app_lib_system_protocol_profile_e "Radio profile" for full list
# mac_profileid=0x00000009
