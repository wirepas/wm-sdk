# You can add more sources here and libraries if needed

SRCS +=$(SRCS_PATH)posapp_settings.c
INCLUDES +=-I$(SRCS_PATH)

# Network settings
CFLAGS += -DCONF_NETWORK_ADDRESS=$(default_network_address)
CFLAGS += -DCONF_NETWORK_CHANNEL=$(default_network_channel)
CFLAGS += -DCONF_ROLE=$(default_role)

#Device class & mode
CFLAGS += -DPOSLIB_DEVICE_CLASS=$(default_poslib_device_class)
CFLAGS += -DPOSLIB_DEVICE_MODE=$(default_poslib_device_mode)

#BLE
CFLAGS += -DPOSLIB_BLETX_MODE=$(default_bletx_mode)
CFLAGS += -DPOSLIB_BLETX_TYPE=$(default_bletx_type)
CFLAGS += -DPOSLIB_BLETX_INTERVAL_MS=$(default_bletx_interval_ms)
CFLAGS += -DPOSLIB_BLETX_ACTIVATION_DELAY_S=$(default_bletx_activation_delay_s)
CFLAGS += -DPOSLIB_BLETX_POWER=$(default_bletx_power)

#Update period
CFLAGS += -DPOSLIB_UPDATE_PERIOD_S=$(default_update_period_static_s)
CFLAGS += -DPOSLIB_UPDATE_PERIOD_DYNAMIC_S=$(default_update_period_dynamic_s)
CFLAGS += -DPOSLIB_UPDATE_PERIOD_OFFLINE_S=$(default_update_period_offline_s)

#Voltage
ifeq ($(default_voltage_report),yes)
CFLAGS += -DCONF_VOLTAGE_REPORT
HAL_VOLTAGE=yes
endif

# Mini-beacon
CFLAGS += -DPOSLIB_MBCN_ENABLED=$(default_mbcn_enabled)
CFLAGS += -DPOSLIB_MBCN_TX_INTERVAL_MS=$(default_mbcn_tx_interval_ms)

#DA
CFLAGS += -DPOSLIB_DA_ROUTING_ENABLED=$(default_da_routing_enabled)
CFLAGS += -DPOSLIB_DA_FOLLOW_NETWORK=$(default_da_follow_network)

# Enable Positioning library
POSITIONING=yes

# Enable HAL led
ifeq ($(led_notification_enabled),yes)
CFLAGS += -DCONF_USE_LED_NOTIFICATION
HAL_LED=yes
endif


# Persistent memory
ifeq ($(use_persistent_memory),yes)
CFLAGS += -DCONF_USE_PERSISTENT_MEMORY
APP_PERSISTENT=yes
endif

#Button support
ifeq ($(button_enabled),yes)
SRCS += $(SRCS_PATH)gpio_nrf52.c
CFLAGS += -DBUTTON_ENABLED
endif

APP_SCHEDULER=yes
APP_SCHEDULER_TASKS=4

# App need it to register for stop stack event
STACK_STATE_LIB=yes
STACK_STATE_CBS=1

# MOTION support
ifneq ($(motion_sensor),)
INCLUDES += -I$(SRCS_PATH)/motion/
include $(SRCS_PATH)/motion/makefile_motion.mk
endif 

