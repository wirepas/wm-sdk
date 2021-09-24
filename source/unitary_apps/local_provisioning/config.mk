# Boards compatible with this app 
TARGET_BOARDS := pca10090 silabs_brd4254a efr32_template pca10059 silabs_brd4180b pca10056 nrf52_template silabs_brd4184a silabs_brd4181b promistel_rpi_hat pca10100 ublox_b204 tbsense2 pca10112 pca10040 bgm220-ek4314a wuerth_261101102 mdbt50q_rx nrf52832_mdk_v2 ruuvitag silabs_brd4312a silabs_brd4253a 
#
# Network default settings configuration
#

# If this section is removed, node has to be configured in
# a different way
# Network address/channel differs from proxy node.
network_joining_address ?= 0x7BD4E5
network_joining_channel ?= 26

#
# App specific configuration
#

# Define a specific application area_id
app_specific_area_id=0x83e789

# App version
# 1.0.0.0 -> 2.0.0.0: Use of default persistent area to store data
app_major=1
app_minor=0
app_maintenance=0
app_development=0
