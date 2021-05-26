# Boards compatible with this app 
TARGET_BOARDS := mdbt50q_rx ublox_b204 pca10040 pca10100 pca10056 pca10059 pca10112 ruuvitag nrf52832_mdk_v2 promistel_rpi_hat tbsense2 wuerth_261101102 silabs_brd4253a silabs_brd4254a silabs_brd4180b silabs_brd4181b silabs_brd4184a 
#
# Network default settings configuration
#

# If this section is removed, node has to be configured in
# a different way
default_network_address ?= 0x12C03A
default_network_channel ?= 3
# !HIGHLY RECOMMENDED! : To enable security keys please un-comment the lines below and fill with a
#                        randomly generated authentication & encryption keys (exactly 16 bytes)
#default_network_cipher_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??
#default_network_authen_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??

#
# App specific configuration
#

# Define a specific application area_id
app_specific_area_id=0x83f161

# App version
app_major=1
app_minor=0
app_maintenance=0
app_development=0
