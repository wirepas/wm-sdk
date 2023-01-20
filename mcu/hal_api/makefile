INCLUDES += -I$(HAL_API_PATH)

SRCS += $(HAL_API_PATH)hal_init.c

# include drivers
include $(MCU_PATH)$(MCU_FAMILY)/makefile

ifeq ($(HAL_BUTTON), yes)
INCLUDES += -I$(HAL_API_PATH)button
endif

ifeq ($(HAL_LED), yes)
INCLUDES += -I$(HAL_API_PATH)led
endif

ifeq ($(HAL_GPIO), yes)
INCLUDES += -I$(HAL_API_PATH)gpio
endif
