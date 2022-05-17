# Boards compatible with this app 
TARGET_BOARDS := pca10056 pca10100 tbsense2 pca10040 pan1780 
# Boards compatible with this app
#
# Network default settings configuration
#

# If this section is removed, node has to be configured in
# a different way
default_network_address ?= 0x6FCA8
default_network_channel ?= 6
# default_multicast_address ?=
# !HIGHLY RECOMMENDED! : To enable security keys please un-comment the lines below and fill with a
#                        randomly generated authentication & encryption keys (exactly 16 bytes)
#default_network_cipher_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??
#default_network_authen_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??

#
# App specific configuration
#
APP_PERSISTENT=yes

# Define a specific application area_id
app_specific_area_id=0x80597C

# App version
app_major=1
app_minor=0
app_maintenance=0
app_development=1
