# You can add more sources here and libraries if needed
SRCS +=$(SRCS_PATH)app.c
SRCS +=$(SRCS_PATH)fifo.c
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



SHARED_APP_CONFIG=yes
CFLAGS += -DSHARED_APP_CONFIG_MAX_FILTER=2

ifeq ($(enable_legacy_appcfg),yes)
CFLAGS += -DENABLE_LEGACY_APPCFG
endif

