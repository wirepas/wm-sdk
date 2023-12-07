
MCU_COMMON_SRCS_PATH := $(MCU_PATH)common/

include $(MCU_COMMON_SRCS_PATH)radio/makefile

ASM_SRCS += $(MCU_COMMON_SRCS_PATH)entrypoint.s
SRCS += $(MCU_COMMON_SRCS_PATH)start.c \
        $(MCU_COMMON_SRCS_PATH)ds.c

ifeq ($(HAL_BUTTON), yes)
SRCS += $(MCU_COMMON_SRCS_PATH)button.c
endif

ifeq ($(HAL_LED), yes)
SRCS += $(MCU_COMMON_SRCS_PATH)led.c
endif

ifeq ($(HAL_GPIO), yes)
SRCS += $(MCU_COMMON_SRCS_PATH)gpio_weak.c
endif
