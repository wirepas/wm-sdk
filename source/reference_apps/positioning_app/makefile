# You can add more sources here and libraries if needed

SRCS +=$(SRCS_PATH)posapp_settings.c
INCLUDES +=-I$(SRCS_PATH)

# Define default network settings
CFLAGS += -DCONF_NETWORK_ADDRESS=$(default_network_address)
CFLAGS += -DCONF_NETWORK_CHANNEL=$(default_network_channel)
CFLAGS += -DCONF_ROLE=$(default_role)
CFLAGS += -DCONF_ROLE_FLAG=$(default_role_flag)
CFLAGS += -DPOSLIB_DEVICE_CLASS=$(default_poslib_device_class)
CFLAGS += -DPOSLIB_DEVICE_MODE=$(default_poslib_device_mode)
CFLAGS += -DPOSLIB_BLEBEACON_SETUP=$(default_BleBeacon_setup)
CFLAGS += -DPOSLIB_BLEBEACON_SELECTION=$(default_BleBeacon_selection)
CFLAGS += -DCONF_VOLTAGE_REPORT=$(default_voltage_report)
CFLAGS += -DCONF_USE_LEGACY_MEASUREMENT=$(use_legacy_measurement)
CFLAGS += -DPOSLIB_UPDATE_PERIOD_S=$(default_poslib_update_period_s)
CFLAGS += -DPOSLIB_BLETX_INTERVAL_S=$(default_bletx_interval_s)
CFLAGS += -DPOSLIB_BLE_PERIOD_OFFLINE_S=$(default_after_offline_update_s)
CFLAGS += -DPOSLIB_BLETX_POWER=$(default_bletx_power)
CFLAGS += -DPOSLIB_UPDATE_PERIOD_DYNAMIC_S=$(default_dynamic_update_period_s)
CFLAGS += -DPOSLIB_UPDATE_PERIOD_OFFLINE_S=$(default_poslib_update_period_offline_s)

# Enable shared libdata library
SHARED_DATA=yes
# Enable shared app_config library
SHARED_APP_CONFIG=yes
CFLAGS += -DSHARED_APP_CONFIG_MAX_FILTER=2
# Enable shared Neigbors library
SHARED_NEIGHBORS=yes
CFLAGS += -DSHARED_NEIGHBORS_MAX_CB=2
# Enabled shared Shutdown library
SHARED_SHUTDOWN=yes
CFLAGS += -DSHARED_SHUTDOWN_MAX_CB=2
# Enable Positioning library
POSITIONING=yes
# Enable HAL button
HAL_BUTTON=yes
# Enable HAL led
HAL_LED=yes

ifeq ($(use_persistent_memory),yes)
 CFLAGS += -DCONF_USE_PERSISTENT_MEMORY
 # Use persistent memeory
 HAL_PERSISTENT_MEMORY=yes
endif

ifeq ($(use_legacy_measurement),yes)
CFLAGS += -DUSE_3BYTE_NODE_ADDRESS
endif

APP_SCHEDULER=yes
CFLAGS += -DAPP_SCHEDULER_MAX_TASKS=1
CFLAGS += -DAPP_SCHEDULER_MAX_EXEC_TIME_US=500
