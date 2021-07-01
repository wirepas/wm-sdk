# Boards compatible with this app 
TARGET_BOARDS := pca10040 pca10100 pca10056 pca10059 ruuvitag silabs_brd4180b silabs_brd4181b silabs_brd4184a 
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

app_specific_area_id ?= 0x8010A0

# Default stack setting

#headnode: 1, subnode: 2
default_role ?= 2
# LE: 0 , LL: 0x10
default_role_flag ?= 0

# Default positioning settings

# POSLIB_DEVICE_CLASS. From poslib_class_e in poslib.h
default_poslib_device_class=0xFA
# POSLIB_DEVICE_MODE. From poslib_mode_e in poslib.h
default_poslib_device_mode=2
# POSLIB_UPDATE_PERIOD_S, default update period
default_update_period_static_s=60
# POSLIB_UPDATE_PERIOD_DYNAMIC_S. Update period used with PosLib motion
default_update_period_dynamic_s=30
# POSLIB_UPDATE_PERIOD_OFFLINE_S. Autoscan mode offline scan period.
default_update_period_offline_s=0

# Default ble settings

# POSLIB_BLEBEACON_SETUP. poslib_ble_mode_e in poslib.h
default_bletx_mode=0
# BLETX_ACTIVATION_DELAY_S. BLE beacons activation delay when outside WM coverage
default_bletx_activation_delay_s=0
# POSLIB_BLEBEACON_SELECTION. From poslib_ble_type_e in poslib.h.
default_bletx_type=1
# POSLIB_BLETX_INTERVAL_S. Default update period for ble tx [milliseconds]. Needs to be [100 ... 60000]
default_bletx_interval_ms=1000
# POSLIB_BLETX_POWER. Fefault bletx power, 8 max used with all radio profiles
default_bletx_power=8

# Default voltage reporting (yes/no) 
default_voltage_report=yes

# Default logging setting
# LVL_DEBUG 4, LVL_INFO 3,LVL_WARNING ,LVL_ERROR 1, LVL_NOLOG 0
default_debug_level=3
# Read configure values from Peristent Memory in use (yes/no)
use_persistent_memory=yes

#Enable button for triggering oneshot update (yes/no) 
button_enabled=no

#Enable led notification (yes/no)
led_notification_enabled=no

#Enable motion sensor
motion_sensor=
default_motion_enabled = 1
default_motion_threshold_mg = 300
default_motion_duration_ms = 0

# App version
app_major=$(sdk_major)
app_minor=$(sdk_minor)
app_maintenance=$(sdk_maintenance)
app_development=2


