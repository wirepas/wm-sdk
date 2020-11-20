# Boards compatible with this app 
TARGET_BOARDS := pca10040 pca10056 pca10059 pca10100 
#
# Network default settings configuration
#

# If this section is removed, node has to be configured in
# a different way
default_network_address ?= 0x2BBBA2
default_network_channel ?= 10
# !HIGHLY RECOMMENDED! : To enable security keys please un-comment the lines below and fill with a
#                        randomly generated authentication & encryption keys (exactly 16 bytes)
#default_network_cipher_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??
#default_network_authen_key ?= 0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??,0x??

app_specific_area_id=0x8010A0
default_device_class = 0xFA
default_device_mode ?= 2 #nrls tag: 1, autoscan_tag: 2, autoscan anchor: 3, opp. anchor: 4
default_role ?= 2 #headnode: 1, subnode: 2
default_role_flag ?= 0x00 # LE: 0 , LL: 0x10
default_debug_level ?= LVL_INFO
default_BleBeacon_setup ?= 0 # off: 0, always on: 1, on when offline: 2
default_BleBeacon_selection ?= 1 # Eddystone: 1, iBeacon: 2, both: 3
default_voltage_report ?= 1 # report voltage: 1, don't report voltage: 0
default_scan_period_s ?= 60 # position update rate in seconds

# Read configure values from Peristent Memory in use (yes/no)
use_persistent_memory=no

# Used to enabled sending a measurement pachet with 3 byte node address
# i.e. same format as in version 4.x
# !DON'T use it in 5.x stack version where node address is 4 byte
use_legacy_measurement=no

# App version
app_major=5
app_minor=0
app_maintenance=0
app_development=0


