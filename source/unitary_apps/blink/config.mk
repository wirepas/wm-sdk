# Boards compatible with this app 
TARGET_BOARDS := pca10059 pca10056 pca10100 pca10040 promistel_rpi_hat ruuvitag silabs_brd4254a tbsense2 silabs_brd4180b silabs_brd4184a silabs_brd4181b bgm220-ek4314a silabs_brd4210a silabs_brd4312a 
#
# Network default settings configuration
#

# If this section is removed, node has to be configured in
# a different way
default_network_address ?= 0x5A127B
default_network_channel ?= 12
# !HIGHLY RECOMMENDED! : To enable security keys please un-comment the lines below and fill with a
#                        randomly generated authentication & encryption keys (exactly 16 bytes)
#default_network_cipher_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??
#default_network_authen_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??

#
# App specific configuration
#

# Define a specific application area_id
app_specific_area_id=0x824076

# App version
app_major=1
app_minor=1
app_maintenance=0
app_development=0
