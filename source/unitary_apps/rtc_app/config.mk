# Boards compatible with this app 
TARGET_BOARDS := pca10112 pca10040 silabs_brd4253a efr32_template silabs_brd2703a ublox_b204 silabs_brd4181b bgm220-ek4314a silabs_brd4184a silabs_brd4312a wuerth_261101102 pca10056 pca10100 nrf52_template promistel_rpi_hat ruuvitag silabs_brd4210a pca20064 mdbt50q_rx pca20049 pan1780 radientum_wp_v1_0 tbsense2 silabs_brd2601b silabs_brd4158a silabs_brd4254a nrf52832_mdk_v2 silabs_brd4187c pca10153 pca10059 silabs_brd4180b 
#
# Network default settings configuration
#

# If this section is removed, node has to be configured in
# a different way
default_network_address ?= 0x239492
default_network_channel ?= 9

# !HIGHLY RECOMMENDED! : To enable security keys please un-comment the lines below and fill with a
#                        randomly generated authentication & encryption keys (exactly 16 bytes)
#default_network_cipher_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??
#default_network_authen_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??

#
# App specific configuration
#

# Define a specific application area_id
app_specific_area_id=0x80597A

# App version
app_major=$(sdk_major)
app_minor=$(sdk_minor)
app_maintenance=$(sdk_maintenance)
app_development=1