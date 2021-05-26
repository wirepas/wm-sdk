# Boards compatible with this app 
TARGET_BOARDS := pca10040 pca10100 pca10056 pca10059 

#
# Network default settings configuration
#

# If this section is removed, node has to be configured in
# a different way
default_network_address ?= 0x1234ff
default_network_channel ?= 10
# !HIGHLY RECOMMENDED! : To enable security keys please un-comment the lines below and fill with a
#                        randomly generated authentication & encryption keys (exactly 16 bytes)
#default_network_cipher_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??
#default_network_authen_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??






app_specific_area_id=0x84BEBB


# Enable also the legacy app config in addition to the shared app config
# yes/no : legacy appcfg enabled / disabled
enable_legacy_appcfg=no

# App version
app_major=5
app_minor=1
app_maintenance=0
app_development=0

