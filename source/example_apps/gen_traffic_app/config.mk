# Boards compatible with this app 
TARGET_BOARDS := bgm220-ek4314a efr32_template mdbt50q_rx nrf52832_mdk_v2 nrf52_template pan1780 pca10040 pca10056 pca10059 pca10100 pca10112 promistel_rpi_hat ruuvitag silabs_brd4180b silabs_brd4181b silabs_brd4184a silabs_brd4253a silabs_brd4254a silabs_brd4312a tbsense2 ublox_b204 wuerth_261101102 
#
# Network default settings configuration
#

default_network_address ?= 0x456efc
default_network_channel ?= 2
default_network_cipher_key = 0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88
default_network_authen_key = 0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88

#
# App specific configuration
#

# Define a specific application area_id
app_specific_area_id=0x875ef6

# App version
app_major=1
app_minor=0
app_maintenance=0
app_development=0
