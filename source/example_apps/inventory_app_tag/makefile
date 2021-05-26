# You can add more sources here and libraries if needed
SRCS +=$(SRCS_PATH)app.c
INCLUDES +=-I$(SRCS_PATH)
LIBS +=
LDFLAGS +=

# Example:
# SRCS += $(SRCS_PATH)my_driver.c
# LIBS += my_lib.a
# include math.h lib
# LDFLAGS += -lm

# Use App Scheduler and configure it
APP_SCHEDULER=yes
CFLAGS += -DAPP_SCHEDULER_MAX_TASKS=4
CFLAGS += -DAPP_SCHEDULER_MAX_EXEC_TIME_US=100

CFLAGS += -DWITH_DIRECTED_ADVERTISER

CFLAGS += -DADVERTISER_RATE_SEC=$(default_advertiser_rate_s)
CFLAGS += -DQUEUING_TIME_MS=$(default_queuing_time_ms)



CFLAGS += -DSCAN_RAND_MS=$(default_scan_rand_ms)
CFLAGS += -DSEND_RAND_MS=$(default_send_rand_ms)