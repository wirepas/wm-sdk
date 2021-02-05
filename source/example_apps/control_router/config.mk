# Boards compatible with this app 
TARGET_BOARDS := mdbt50q_rx nrf52832_mdk_v2 pca10040 pca10056 pca10059 pca10100 pca10112 promistel_rpi_hat ruuvitag silabs_brd4253a silabs_brd4254a tbsense2 ublox_b204 wirepas_brd4181b 
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
app_major=0
app_minor=1
app_maintenance=0
app_development=0
