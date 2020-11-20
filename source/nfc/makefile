# You can add more sources here and libraries if needed
include $(SRCS_PATH)drivers/makefile
SRCS += $(SRCS_PATH)lib/t2_emulation.c
SRCS += $(SRCS_PATH)lib/tlv.c
SRCS += $(SRCS_PATH)lib/ndef.c
INCLUDES += -I$(SRCS_PATH)lib
LIBS +=
LDFLAGS +=

# Example:
# SRCS += $(SRCS_PATH)my_driver.c
# LIBS += my_lib.a
# include math.h lib
# LDFLAGS += -lm

HAL_HW_DELAY=yes

# Use App Scheduler and configure it
APP_SCHEDULER=yes
CFLAGS += -DAPP_SCHEDULER_MAX_TASKS=4
CFLAGS += -DAPP_SCHEDULER_MAX_EXEC_TIME_US=350
