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

# Default stack setting

#headnode: 1, subnode: 2
default_role ?= 2
# LE: 0 , LL: 0x10
default_role_flag ?= 0

# Default positioning settings

# POSLIB_DEVICE_CLASS. From poslib_class_e in poslib.h
default_poslib_device_class=0xFA
# POSLIB_DEVICE_MODE. From poslib_mode_e in poslib.h
default_poslib_device_mode=4
# POSLIB_UPDATE_PERIOD_S, default update period
default_poslib_update_period_s=90
# POSLIB_UPDATE_PERIOD_OFFLINE_S. Autoscan mode offline scan period.
default_poslib_update_period_offline_s=0
# POSLIB_UPDATE_PERIOD_DYNAMIC_S. Update period used with PosLib motion
default_dynamic_update_period_s=180

# Default ble settings

# POSLIB_BLEBEACON_SETUP. poslib_ble_mode_e in poslib.h
default_BleBeacon_setup=0
# POSLIB_BLEUPDATE_PERIOD_OFFLINE_S. Default period (0 disabled) when ble tx after stack offline
default_after_offline_update_s=0
# POSLIB_BLEBEACON_SELECTION. From poslib_ble_type_e in poslib.h.
default_BleBeacon_selection=1
# POSLIB_BLETX_INTERVAL_S. Default update period for ble tx. Needs to be < 60
default_bletx_interval_s=30
# POSLIB_BLETX_POWER. Fefault bletx power, 8 max used with all radio profiles
default_bletx_power=8

# Default voltage reporting
default_voltage_report = 1 # report voltage: 1, don't report voltage: 0
# Default logging setting
# LVL_DEBUG 4, LVL_INFO 3,LVL_WARNING ,LVL_ERROR 1, LVL_NOLOG 0
default_debug_level=3
# Read configure values from Peristent Memory in use (yes/no)
use_persistent_memory=no

# Used to enabled sending a measurement pachet with 3 byte node address
# i.e. same format as in version 4.x
# !DON'T use it in 5.x stack version where node address is 4 byte
use_legacy_measurement=no

# App version
app_major=5
app_minor=1
app_maintenance=0
app_development=0


