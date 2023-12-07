# Boards compatible with this app 
TARGET_BOARDS := pan1780 wuerth_261101102 silabs_brd4158a silabs_brd2601b nrf52_template silabs_brd4312a bgm220-ek4314a nrf52832_mdk_v2 mdbt50q_rx pca20049 silabs_brd2703a efr32_template silabs_brd4254a pca10040 silabs_brd4181b silabs_brd4180b pca10059 pca10153 pca10100 silabs_brd4184a silabs_brd4253a ruuvitag radientum_wp_v1_0 silabs_brd4210a pca20064 promistel_rpi_hat ublox_b204 tbsense2 pca10056 pca10112 silabs_brd4187c 
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