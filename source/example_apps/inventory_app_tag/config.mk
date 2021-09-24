# Boards compatible with this app 
TARGET_BOARDS := pca10090 silabs_brd4254a efr32_template pca10059 silabs_brd4180b pca10056 nrf52_template silabs_brd4184a silabs_brd4181b promistel_rpi_hat pca10100 ublox_b204 tbsense2 pca10112 pca10040 bgm220-ek4314a wuerth_261101102 mdbt50q_rx nrf52832_mdk_v2 ruuvitag silabs_brd4312a silabs_brd4253a 


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
app_major=5
app_minor=1
app_maintenance=0
app_development=0
