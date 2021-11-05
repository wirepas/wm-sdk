# Boards compatible with this app 
TARGET_BOARDS := silabs_brd4312a nrf52832_mdk_v2 tbsense2 nrf52_template pca10112 bgm220-ek4314a efr32_template silabs_brd4253a silabs_brd4180b pca10059 silabs_brd4184a ublox_b204 pca10100 mdbt50q_rx pca10056 silabs_brd4181b ruuvitag promistel_rpi_hat silabs_brd4254a pca10040 pca10090 wuerth_261101102 


#
# Network default settings configuration
#

# If this section is removed, node has to be configured in
# a different way
default_network_address ?= 0x1234FF
default_network_channel ?= 10
# !HIGHLY RECOMMENDED! : To enable security keys please un-comment the lines below and fill with a
#                        randomly generated authentication & encryption keys (exactly 16 bytes)
#default_network_cipher_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??
#default_network_authen_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??

#
# App specific configuration
#
# Define a specific application area_id
app_specific_area_id=0x84BEAA


# the rate at which advertiser packets are sent
default_advertiser_rate_s = 30

# the default queuing time [ms]
default_queuing_time_ms = 70


#scan start randomization window
default_scan_rand_ms=250
#send start randomization window
default_send_rand_ms=100

# App version
app_major=$(sdk_major)
app_minor=$(sdk_minor)
app_maintenance=$(sdk_maintenance)
app_development=1