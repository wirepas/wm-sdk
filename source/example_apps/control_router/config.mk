# Boards compatible with this app 
TARGET_BOARDS := silabs_brd4312a pca10056 silabs_brd4181b promistel_rpi_hat silabs_brd4184a silabs_brd4254a pca10100 pca10090_nrf52840 pca10059 ublox_b204 silabs_brd4210a nrf52_template pca10112 pan1780 nrf52832_mdk_v2 ruuvitag silabs_brd4180b tbsense2 bgm220-ek4314a silabs_brd4253a pca10040 efr32_template wuerth_261101102 mdbt50q_rx 
#
# Network default settings configuration
#

# If this section is removed, node has to be configured in
# a different way
default_network_address ?= 0xB689E6
default_network_channel ?= 3
# !HIGHLY RECOMMENDED! : To enable security keys please un-comment the lines below and fill with a
#                        randomly generated authentication & encryption keys (exactly 16 bytes)
#default_network_cipher_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??
#default_network_authen_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??

#
# App specific configuration
#

# Define a specific application area_id
app_specific_area_id=0x8030B3

# App version
app_major=$(sdk_major)
app_minor=$(sdk_minor)
app_maintenance=$(sdk_maintenance)
app_development=1