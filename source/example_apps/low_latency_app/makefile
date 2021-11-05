# You can add more sources here and libraries if needed
SRCS +=
INCLUDES +=
LIBS +=
LDFLAGS +=

# Example:
# SRCS += $(SRCS_PATH)my_driver.c
# LIBS += my_lib.a
# include math.h lib
#LDFLAGS += -lm

# Define default network settings
CFLAGS += -DNETWORK_ADDRESS=$(default_network_address)
CFLAGS += -DNETWORK_CHANNEL=$(default_network_channel)

ifneq ($(default_operating_mode),0)
    CFLAGS+= -DENABLE_LOW_LATENCY_MODE
endif

# Use Shared Data
SHARED_DATA=yes

# Use App Scheduler and configure it
APP_SCHEDULER=yes
APP_SCHEDULER_TASKS=1

APP_PRINTING=yes
# This application use HAL for buttons and leds
HAL_BUTTON=yes
HAL_LED=yes