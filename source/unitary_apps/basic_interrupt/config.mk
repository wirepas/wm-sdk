# Boards compatible with this app 
TARGET_BOARDS := pca10056 promistel_rpi_hat pca10100 ublox_b204 pca10040 
#
# Network default settings configuration
#

# If this section is removed, node has to be configured in
# a different way
default_network_address ?= 0xABCDEF
default_network_channel ?= 2
# !HIGHLY RECOMMENDED! : To enable security keys please un-comment the lines below and fill with a
#                        randomly generated authentication & encryption keys (exactly 16 bytes)
#default_network_cipher_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??
#default_network_authen_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??

#
# App specific configuration
#

# Define a specific application area_id
app_specific_area_id=0x81FD41

# App version
app_major=2
app_minor=0
app_maintenance=0
app_development=0
