# Boards compatible with this app 
TARGET_BOARDS := pca10040 pca10056 pca10059 pca10100 promistel_rpi_hat ruuvitag silabs_brd4254a tbsense2 
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

ifeq ($(storage),memarea)
INI_FILE=$(APP_SRCS_PATH)scratchpad_ini/scratchpad_$(MCU)$(MCU_SUB)$(MCU_MEM_VAR).ini
endif

# App version
app_major=1
app_minor=0
app_maintenance=0
app_development=0
