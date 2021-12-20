#Set node operating mode (i.e low-energy or low-latency)
ifneq ($(default_operating_mode),0)
    CFLAGS+= -DENABLE_LOW_LATENCY_MODE
endif

# Use Shared Data
SHARED_DATA=yes

# Use App Scheduler and configure it
APP_SCHEDULER=yes
APP_SCHEDULER_TASKS=2

APP_PRINTING=yes

SHARED_APP_CONFIG=yes
SHARED_APP_CONFIG_FILTERS=2

# This application use HAL for buttons and leds
HAL_BUTTON=yes
HAL_LED=yes
