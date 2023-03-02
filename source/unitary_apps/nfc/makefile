# You can add more sources here and libraries if needed
include $(APP_SRCS_PATH)drivers/makefile
SRCS += $(APP_SRCS_PATH)lib/t2_emulation.c
SRCS += $(APP_SRCS_PATH)lib/tlv.c
SRCS += $(APP_SRCS_PATH)lib/ndef.c
INCLUDES += -I$(APP_SRCS_PATH)lib

HAL_HW_DELAY=yes

# Use App Scheduler and configure it
APP_SCHEDULER=yes
APP_SCHEDULER_TASKS=4
