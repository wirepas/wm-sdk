# Boards compatible with this app 
TARGET_BOARDS := silabs_brd4312a pca10056 silabs_brd4181b promistel_rpi_hat silabs_brd4184a silabs_brd4254a pca10100 pca10090_nrf52840 pca10059 ublox_b204 silabs_brd4210a nrf52_template pca10112 pan1780 nrf52832_mdk_v2 ruuvitag silabs_brd4180b tbsense2 bgm220-ek4314a silabs_brd4253a pca10040 efr32_template wuerth_261101102 mdbt50q_rx 
#
# Network default settings configuration
#

# If this section is removed, node has to be configured in
# a different way
# Network address/channel differs from proxy node.
default_network_address ?= 0xABCDE
default_network_channel ?= 5
# !HIGHLY RECOMMENDED! : To enable security keys please un-comment the lines below and fill with a
#                        randomly generated authentication & encryption keys (exactly 16 bytes)
#default_network_cipher_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??
#default_network_authen_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??

#
# App specific configuration
#

# Define a specific application area_id
app_specific_area_id=0x82f599

# UID/Key storage (chipid, memarea)
storage=memarea

# App version
# 1.0.0.0 -> 2.0.0.0: Use of default persistent area to store data
app_major=2
app_minor=0
app_maintenance=0
app_development=0
