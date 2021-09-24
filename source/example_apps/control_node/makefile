# You can add more sources here and libraries if needed
SRCS +=
INCLUDES +=
LIBS +=
LDFLAGS +=

# Example:
# SRCS += $(SRCS_PATH)my_driver.c
# LIBS += my_lib.a
# include math.h lib
# LDFLAGS += -lm

# Control node app default settings (set in config.mk)
CFLAGS += -DCONF_DIAG_PERIOD_MS=$(default_diag_period_ms)
CFLAGS += -DCONF_PKT_TTL_MS=$(default_packet_ttl_ms)

# This application use HAL for buttons
HAL_BUTTON=yes

APP_SCHEDULER=yes
APP_SCHEDULER_TASKS=1

# Enable Control library
CONTROL_NODE=yes

# Uncomment to enable debug logs (has no effect on board without serial port)
# Note: This is disabled by default to avoid overconsumption.
# APP_PRINTING=yes
