# Boards compatible with this app 
TARGET_BOARDS := pca10112 tbsense2 ublox_b204 pca10059 pca10040 silabs_brd4210a pca10100 wuerth_261101102 nrf52832_mdk_v2 efr32_template promistel_rpi_hat silabs_brd4254a mdbt50q_rx bgm220-ek4314a silabs_brd4184a silabs_brd4312a ruuvitag silabs_brd4180b pan1780 pca10056 silabs_brd4181b silabs_brd4253a nrf52_template 
#
# Network default settings configuration
#

# If this section is removed, node has to be configured in
# a different way
default_network_address ?= 0xA1B2C3
default_network_channel ?= 2
# !HIGHLY RECOMMENDED! : To enable security keys please un-comment the lines below and fill with a
#                        randomly generated authentication & encryption keys (exactly 16 bytes)
#default_network_cipher_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??
#default_network_authen_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??

#
# App specific configuration
#

# Define a specific application area_id
app_specific_area_id=0x84BEBD

# App version
app_major=1
app_minor=0
app_maintenance=0
app_development=0
