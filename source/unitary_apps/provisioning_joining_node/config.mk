# Boards compatible with this app 
TARGET_BOARDS := silabs_brd4254a efr32_template pca10059 silabs_brd4180b pca10056 nrf52_template silabs_brd4184a silabs_brd4181b promistel_rpi_hat pca10100 ublox_b204 tbsense2 pca10112 pca10040 bgm220-ek4314a wuerth_261101102 silabs_brd4210a mdbt50q_rx nrf52832_mdk_v2 ruuvitag pan1780 silabs_brd4312a silabs_brd4253a 
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
