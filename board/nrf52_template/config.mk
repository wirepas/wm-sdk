# This definition describes processor architecture. 
# Valid value: nrf52 for nRF52 based boards
MCU=nrf52

# This describes the sub-architecture of the processor. Allowed values are
# following: 832,840
MCU_SUB=832

# This describes whether module contains @ref radio_fem.h "Radio FEM" module.
# Optional.  
# USE_FEM=yes

# This describes the radio profiles to choose (only valid if MCU_SUB=840)
# Allowed values are 0x0000000D (max power to 4dm), 0x0000000E (max power to 8dbm)
# See @ref app_lib_system_protocol_profile_e "Radio profile" for full list
# mac_profileid=0x0000000E

