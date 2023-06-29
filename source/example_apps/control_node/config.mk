# Boards compatible with this app 
TARGET_BOARDS := nrf52832_mdk_v2 bgm220-ek4314a silabs_brd4254a pca10090 pca10059 silabs_brd4184a wuerth_261101102 pca10090_nrf52840 promistel_rpi_hat ruuvitag silabs_brd2601b silabs_brd4180b efr32_template mdbt50q_rx nrf52_template pca10153 pca10040 silabs_brd4312a silabs_brd4253a silabs_brd4187c silabs_brd2703a pca10056 pan1780 silabs_brd4181b pca10100 tbsense2 ublox_b204 pca10112 silabs_brd4210a 
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
app_specific_area_id=0x8030B2

# Control node app default settings
# The diagnostic period is low for more reactivity during tests.
# A value of 12 hours is recommended for production.
default_diag_period_ms=60000
default_packet_ttl_ms=250

# App version
app_major=$(sdk_major)
app_minor=$(sdk_minor)
app_maintenance=$(sdk_maintenance)
app_development=1