# This application use HAL for buttons and leds
HAL_BUTTON=yes
HAL_LED=yes

# Define default network settings
CFLAGS += -DNETWORK_ADDRESS=$(default_network_address)
CFLAGS += -DNETWORK_CHANNEL=$(default_network_channel)

# Use App Scheduler and configure it
APP_SCHEDULER=yes
CFLAGS += -DAPP_SCHEDULER_MAX_TASKS=4
CFLAGS += -DAPP_SCHEDULER_MAX_EXEC_TIME_US=100
