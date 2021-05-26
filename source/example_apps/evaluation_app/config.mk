# Boards compatible with this app 
TARGET_BOARDS := pca10040 pca10100 pca10056 ruuvitag tbsense2 
# Boards compatible with this app
#
# Network default settings configuration
#

# If this section is removed, node has to be configured in
# a different way
default_network_address ?= 0xD8D42B
default_network_channel ?= 9
# !HIGHLY RECOMMENDED! : To enable security keys please un-comment the lines below and fill with a
#                        randomly generated authentication & encryption keys (exactly 16 bytes)
#default_network_cipher_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??
#default_network_authen_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??

#
# App specific configuration
#

# Define a specific application area_id
app_specific_area_id=0x80597B

#define node operating mode (i.e low-energy or low-latency : 0 => low-energy / 1 => low-latency)
default_operating_mode ?= 0

# App version
app_major=1
app_minor=0
app_maintenance=0
app_development=0
