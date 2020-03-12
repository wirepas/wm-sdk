# You can add more sources here if needed

SRCS +=$(SRCS_PATH)app_settings.c
SRCS +=$(SRCS_PATH)ble_beacon.c
SRCS +=$(SRCS_PATH)measurements.c
SRCS +=$(SRCS_PATH)scheduler.c
SRCS +=$(SRCS_PATH)overhaul.c
SRCS +=$(SRCS_PATH)event.c
INCLUDES +=-I$(SRCS_PATH)

# Define default network settings
CFLAGS += -DCONF_NETWORK_ADDRESS=$(default_network_address)
CFLAGS += -DCONF_NETWORK_CHANNEL=$(default_network_channel)
CFLAGS += -DCONF_ROLE=$(default_role)
CFLAGS += -DCONF_ROLE_FLAG=$(default_role_flag)
CFLAGS += -DCONF_DEVICE_MODE=$(default_device_mode)
CFLAGS += -DCONF_DEVICE_CLASS=$(default_device_class)
CFLAGS += -DCONF_BLEBEACON_SETUP=$(default_BleBeacon_setup)
CFLAGS += -DCONF_BLEBEACON_SELECTION=$(default_BleBeacon_selection)
CFLAGS += -DCONF_VOLTAGE_REPORT=$(default_voltage_report)
CFLAGS += -DCONF_SCAN_PERIOD_S=$(default_scan_period_s)
CFLAGS += -DCONF_USE_LEGACY_MEASUREMENT=$(use_legacy_measurement)



ifeq ($(use_persistent_memory),yes)
 CFLAGS += -DUSE_PERSISTENT_MEMORY
 # Use persistent memeory
 HAL_PERSISTENT_MEMORY=yes
endif

ifeq ($(use_legacy_measurement),yes)
CFLAGS += -DUSE_3BYTE_NODE_ADDRESS
endif

APP_SCHEDULER=yes
CFLAGS += -DAPP_SCHEDULER_MAX_TASKS=1
CFLAGS += -DAPP_SCHEDULER_MAX_EXEC_TIME_US=100
